//
//  mmap_list.c
//  product
//
//  Created by Neil Singh on 1/6/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "mmap_list.h"
#include <program/task.h>
#include <syscalls/interrupt.h>

// Create a new mmap region
mmap_list_t* mmap_list_create(uint32_t start, uint32_t end, uint32_t permissions, file_descriptor_t* f,
							  uint32_t offset, bool shared) {
	mmap_list_t* t = kmalloc(sizeof(mmap_list_t));
	if (!t)
		return NULL;
	
	memset(t, 0, sizeof(mmap_list_t));
	t->start = start;
	t->end = end;
	t->permissions = permissions;
	t->shared = shared;
	t->lock = MUTEX_UNLOCKED;
	if (f) {
		t->file = f->duplicate(f);
		if (!t->file) {
			kfree(t);
			return NULL;
		}
		t->file->ref_count = 1;
		t->offset = offset;
	}
	
	return t;
}

// Link a mmap region into a list
void mmap_list_link(mmap_list_t* t, mmap_list_t** list) {
	if (*list) {
		down(&(*list)->lock);
		down(&t->lock);
		t->next = *list;
		(*list)->prev = t;
		*list = t;
		up(&t->next->lock);
		up(&t->lock);
	} else {
		down(&t->lock);
		*list = t;
		up(&t->lock);
	}
}

// Does a memory mapping exist at the specific address
mmap_list_t* mmap_list_address_exists(mmap_list_t* list, uint32_t addr) {
	return mmap_list_region_exists(list, addr, addr + 1);
}

// // Does a memory mapping exist anywhere in the region (returns the a region that corresponds to it)
mmap_list_t* mmap_list_region_exists(mmap_list_t* list, uint32_t start, uint32_t end) {
	mmap_list_t* t = list;
	if (t)
		down(&t->lock);
	while (t) {
		// Compute interval intersection, then see if it is nonempty
		uint32_t s = (start < t->start) ? t->start : start;
		uint32_t e = (end < t->end) ? end : t->end;
		if (s < e) {
			up(&t->lock);
			return t;
		}
		
		mmap_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	return NULL;
}

// Helper to free page tables associated with a range
void remove_page_table_mappings(pcb_t* pcb, uint32_t start, uint32_t end) {
	// Free the associated page mappings as well by rounding the first address up to the nearest 4kb page
	// and the last address down to the nearest 4kb page.
	if ((start % FOUR_KB_SIZE) != 0)
		start = start + (FOUR_KB_SIZE - (start % FOUR_KB_SIZE));
	end = end - (end % FOUR_KB_SIZE);
	uint32_t prev_addr = 0;
	page_list_t* current_page = NULL;
	uint32_t valid_pages = 0;
	while (start < end) {
		if (prev_addr / FOUR_MB_SIZE != start / FOUR_MB_SIZE) {
			// Update the current page
			current_page = page_list_get(&pcb->page_list, start, 0, false);
			if (!current_page || (current_page && !current_page->page_table)) {
				// Nothing to unmap if it doesn't exist
				start += FOUR_MB_SIZE;
				break;
			}
			
			// Loop through to find out how many valid page mappings it has
			valid_pages = 0;
			for (uint32_t z = 0; z < NUM_PAGE_TABLE_ENTRIES; z++) {
				if (current_page->page_table->pages[z] != 0)
					valid_pages++;
			}
		}
		
		uint32_t page = (start % FOUR_MB_SIZE) / FOUR_KB_SIZE;
		down(&current_page->lock);
		if (page_list_read_bitmap(current_page->page_table->owners, page)) {
			page_physical_free((void*)current_page->page_table->pages[page]);
			page_list_write_bitmap(current_page->page_table->owners, page, false);
		}
		current_page->page_table->pages[page] = 0;
		up(&current_page->lock);
		if (--valid_pages == 0) {
			// No more valid pages in this page, so we can delete it
			page_list_dealloc_entry(current_page, &pcb->page_list);
			// Move onto the next 4mb page
			start = start + (FOUR_MB_SIZE - (start % FOUR_MB_SIZE));
			continue;
		}
		
		start += FOUR_KB_SIZE;
	}
}

// Remove a memory mapping region (this can cause a region to be split into multiple regions or multiple deletions
// ex: [ 0x0000, 0x1000 ) removing [ 0x0200, 0x0400 ) -> [ 0x0000, 0x200 ), [ 0x400, 0x1000 ) or
// [ 0x0000, 0x1000 ), [ 0x2000, 0x3000 ) removing [ 0x800, 0x2200 ) -> [ 0x000, 0x800 ), [ 0x2200, 0x3000 ))
// Frees associated page tables if pcb != NULL
bool mmap_list_region_remove(mmap_list_t** list, uint32_t start, uint32_t end, pcb_t* pcb) {
	mmap_list_t* t = NULL;
	// Loop through all the regions that this address range touches
	while ((t = mmap_list_region_exists(*list, start, end)) != NULL) {
		// We only have to create a new entry if we split this in the middle, otherwise
		// we just update the interval. If the region encompases our entrie's entire interval
		// then we can dealloc the entry completely.
		down(&t->lock);
		uint32_t s = 0, e = 0;
		if (start <= t->start && end >= t->end) {
			// All encompassing
			up(&t->lock);
			mmap_list_dealloc(t, list, NULL);
			s = t->start;
			e = t->end;
		} else if (t->start < start && t->end > end) {
			// Split in middle
			mmap_list_t* copy = mmap_list_create(t->start, start, t->permissions, t->file, t->offset, t->shared);
			if (!copy) {
				up(&t->lock);
				return false;
			}
			t->start = end;
			up(&t->lock);
			mmap_list_link(copy, list);
			s = start;
			e = end;
		} else {
			// End
			if (start <= t->start) {
				s = t->start;
				e = end;
				t->start = end;
			}
			else {
				s = start;
				e = t->end;
				t->end = start;
			}
			up(&t->lock);
		}
		if (pcb)
			remove_page_table_mappings(pcb, s, e);
	}
	
	return true;
}

// Process a list during a page fault (return true if there was a hit)
bool mmap_list_process(mmap_list_t* list, uint32_t address, uint32_t code, pcb_t* pcb) {
	// Get the mmap region that corresponds to it
	mmap_list_t* region = mmap_list_address_exists(list, address);
	if (!region)
		return false;
	
	// If we don't have permission to do what we did here then return an error anyway
	if (!region->permissions || (!(region->permissions & MEMORY_WRITE) && (code & PAGE_FAULT_WRITE_VIOLATON)))
		return false;
	
	uint32_t address_4mb_aligned = address - (address % FOUR_MB_SIZE);
	uint32_t address_4kb_aligned = address - (address % FOUR_KB_SIZE);
	page_list_t* page = page_list_get(&pcb->page_list, address_4mb_aligned, 0, false);
	if (!page || (page && !page->page_table))
		return false;
	
	// Update the corresponding 4kb mapping
	down(&page->lock);
	uint32_t offset = (address % FOUR_MB_SIZE) / FOUR_KB_SIZE;
	if (region->permissions & MEMORY_READ) {
		// Make it a read write so we can modify it
		page->page_table->pages[offset] = vm_create_page_table_entry(page->page_table->pages[offset] & ~(FOUR_KB_SIZE - 1),
																	 MEMORY_RW);
		up(&page->lock);
		page_list_map(page, false);
		down(&page->lock);
		
		// Copy over data
		if (region->file) {
			// Read in the data
			uint32_t seek_pos = region->offset + address_4kb_aligned - region->start;
			region->file->llseek(region->file, uint64_make(0, seek_pos), SEEK_SET);
			uint32_t rem = FOUR_KB_SIZE;
			int32_t num = 0;
			while ((num = region->file->read(region->file, (void*)address_4kb_aligned, rem)) > 0) {
				rem -= num;
				if (rem == 0)
					break;
			}
		} else {
			// Fill with 0's
			memset((void*)address_4kb_aligned, 0, FOUR_KB_SIZE);
		}
	}
	
	// Update the corresponding 4kb mapping to not include writing if needed
	if (!(region->permissions & MEMORY_WRITE)) {
		page->page_table->pages[offset] = vm_create_page_table_entry(page->page_table->pages[offset] & ~(FOUR_KB_SIZE - 1),
																	 region->permissions);
		up(&page->lock);
		page_list_map(page, false);
		down(&page->lock);
	}
	
	// Update the linked entries' permissions if they are shared
	page_cow_list_t* l = page->linked;
	if (l)
		down(&l->lock);
	up(&page->lock);
	while (l) {
		if (l->entry != page) {
			down(&l->entry->lock);
			if (!page_list_read_bitmap(l->entry->page_table->cow, offset)) {
				l->entry->page_table->pages[offset] =
					vm_create_page_table_entry(l->entry->page_table->pages[offset] & ~(FOUR_KB_SIZE - 1),
											   region->permissions);
			}
			up(&l->entry->lock);
		}
		
		page_cow_list_t* prev = l;
		l = l->next;
		if (l)
			down(&l->lock);
		up(&prev->lock);
	}
	
	return true;
}

// Copy a list
mmap_list_t* mmap_list_copy(mmap_list_t* list) {
	mmap_list_t* head = NULL;
	mmap_list_t* tail = NULL;
	mmap_list_t* t = list;
	if (t)
		down(&t->lock);
	while (t) {
		mmap_list_t* l = mmap_list_create(t->start, t->end, t->permissions, t->file, t->offset, t->shared);
		if (!l) {
			up(&t->lock);
			mmap_list_dealloc_list(head);
			return NULL;
		}
		
		if (!head)
			head = l;
		if (tail) {
			tail->next = l;
			l->prev = tail;
		}
		tail = l;
		
		mmap_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	
	return head;
}

// Sync a address region (if invalidate is true, all other mappings of the same file across all processes
// will be invalidate so that they can be reloaded with the new values)
void mmap_list_sync(mmap_list_t* list, uint32_t start, uint32_t end, bool invalidate) {
	mmap_list_t* t = list;
	if (t)
		down(&t->lock);
	while (t) {
		// Compute interval intersection, then see if it is nonempty
		uint32_t s = (start < t->start) ? t->start : start;
		uint32_t e = (end < t->end) ? end : t->end;
		if (s < e && t->file && t->shared && (t->permissions & MEMORY_WRITE)) {
			// Write the interval to the file
			t->file->llseek(t->file, uint64_make(0, s - t->start + t->offset), SEEK_SET);
			t->file->write(t->file, (const void*)s, e - s);
		}
		
		mmap_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	
	if (invalidate) {
		// Invalidate other mappings by just making them not present
		// TODO: come back here when theres some method of linking open files
	}
}

// Dealloc a single list entry (and frees the associated pagelist tables too if pcb != NULL)
void mmap_list_dealloc(mmap_list_t* t, mmap_list_t** list, pcb_t* pcb) {
	if (pcb) {
		mmap_list_region_remove(&pcb->user_mappings, t->start, t->end, pcb);
		return;
	}
	
	// Unlink
	if (t->prev)
		down(&t->prev->lock);
	down(&t->lock);
	if (t->next)
		down(&t->next->lock);
	if (t->prev) {
		t->prev->next = t->next;
		up(&t->prev->lock);
	}
	if (t->next) {
		t->next->prev = t->prev;
		up(&t->next->lock);
	}
	if (list && *list == t)
		*list = t->next;
	up(&t->lock);
	
	// Free data
	if (t->file)
		t->file->close(t->file);
	kfree(t);
}

// Dealloc a list
void mmap_list_dealloc_list(mmap_list_t* list) {
	// Sync the list
	mmap_list_sync(list, 0, VM_KERNEL_ADDRESS, false);
	
	mmap_list_t* t = list;
	if (t)
		down(&t->lock);
	while (t) {
		mmap_list_t* next = t->next;
		if (next)
			down(&next->lock);
		up(&t->lock);
		mmap_list_dealloc(t, NULL, NULL);
		
		t = next;
	}
}
