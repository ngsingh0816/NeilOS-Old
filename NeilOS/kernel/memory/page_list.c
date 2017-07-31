//
//  page_list.c
//  NeilOS
//
//  Created by Neil Singh on 7/12/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "page_list.h"
#include "memory.h"
#include <common/lib.h>
#include <program/task.h>

// Helpers for cow linked list
// Add an entry to the page cow list
bool page_cow_list_add(page_cow_list_t** list, page_list_t* entry) {
	page_cow_list_t* t = kmalloc(sizeof(page_cow_list_t));
	if (!t)
		return false;
	
	t->entry = entry;
	t->next = NULL;
	t->prev = NULL;
	
	page_cow_list_t* head = *list;
	if (head) {
		t->next = head;
		head->prev = t;
	}
	*list = t;
	
	return true;
}

// Remove an entry from the page cow list
bool page_cow_list_remove(page_cow_list_t** list, page_list_t* l) {
	page_cow_list_t* t = *list;
	while (t) {
		if (t->entry == l) {
			if (t->prev)
				t->prev->next = t->next;
			if (t->next)
				t->next->prev = t->prev;
			
			if (*list == t)
				*list = t->next;
			
			kfree(t);
			
			return true;
		}
		t = t->next;
	}
	
	return false;
}

// Helper to read a bitmap
bool page_list_read_bitmap(uint32_t* bitmap, uint32_t index) {
	uint32_t page = index / (sizeof(uint32_t) * 8);
	uint32_t offset = index % (sizeof(uint32_t) * 8);
	return (bitmap[page] >> offset) & 0x1;
}

// Helper to write a bitmap
void page_list_write_bitmap(uint32_t* bitmap, uint32_t index, bool value) {
	uint32_t page = index / (sizeof(uint32_t) * 8);
	uint32_t offset = index % (sizeof(uint32_t) * 8);
	if (value)
		bitmap[page] |= (1 << offset);
	else
		bitmap[page] &= ~(1 << offset);
}

// Add a page to the list
page_list_t* page_list_add(page_list_t** list, uint32_t vaddr, uint32_t permissions) {
	if (!list)
		return NULL;
	
	page_list_t* t = kmalloc(sizeof(page_list_t));
	if (!t)
		return NULL;
	
	memset(t, 0, sizeof(page_list_t));
	t->vaddr = vaddr;
	t->permissions = permissions;
	t->paddr = (uint32_t)page_physical_get_four_mb(1);
	if (!t->paddr) {
		kfree(t);
		return NULL;
	}
	t->owner = true;
	
	page_list_t* head = *list;
	if (head) {
		t->next = head;
		head->prev = t;
	}
	*list = t;
	
	return t;
}

// Helper to copy a page table
bool page_list_copy_page_table(page_list_t* l, page_list_t* p) {
	l->page_table = kmalloc(sizeof(page_table_t));
	if (!l->page_table) {
		kfree(l);
		return false;
	}
	l->page_table->pages = page_get_aligned_four_kb();
	if (!l->page_table->pages) {
		kfree(l->page_table);
		kfree(l);
		return false;
	}
	for (uint32_t z = 0; z < NUM_PAGE_TABLE_ENTRIES; z++) {
		p->page_table->pages[z] = vm_create_page_table_entry(p->page_table->pages[z] & ~(FOUR_KB_SIZE - 1),
															 p->permissions & ~MEMORY_WRITE);
		l->page_table->pages[z] = p->page_table->pages[z];
	}
	memset(l->page_table->owners, 0, NUM_PAGE_TABLE_ENTRIES / 8);
	
	return true;
}

// Sets up the page table
bool page_list_set_up_page_table(page_list_t* list) {
	list->page_table = kmalloc(sizeof(page_table_t));
	if (!list->page_table)
		return false;
	
	list->page_table->pages = page_get_aligned_four_kb();
	if (!list->page_table->pages) {
		kfree(list->page_table);
		return false;
	}
	
	// Set up to point to original adressses
	for (uint32_t z = 0; z < NUM_PAGE_TABLE_ENTRIES; z++) {
		list->page_table->pages[z] = vm_create_page_table_entry(list->paddr + z * FOUR_KB_SIZE,
																(list->permissions & ~MEMORY_WRITE));
	}
	
	// Set no pages as owners
	memset(list->page_table->owners, 0, NUM_PAGE_TABLE_ENTRIES / 8);
	
	return true;
}

// Add a page to the list and return it by copying another page entry
page_list_t* page_list_add_copy(page_list_t** list, page_list_t* p) {
	bool flags = set_multitasking_enabled(false);
	// Copy the list but point to the same address without write access
	page_list_t* l = kmalloc(sizeof(page_list_t));
	if (!l)
		return NULL;
	memcpy(l, p, sizeof(page_list_t));
	l->next = NULL;
	l->prev = NULL;
	l->owner = false;
	l->page_table = NULL;
	l->linked = NULL;
	if ((p->permissions & MEMORY_WRITE)) {
		l->copy_on_write = true;
		p->copy_on_write = true;
		
		// Set the linked page lists
		page_cow_list_add(&l->linked, p);
		page_cow_list_t* t = p->linked;
		while (t) {
			page_cow_list_add(&l->linked, t->entry);
			page_cow_list_add(&t->entry->linked, l);
			t = t->next;
		}
		page_cow_list_add(&p->linked, l);
		
		// Set up the page table for the old list
		if (!p->page_table) {
			if (!page_list_set_up_page_table(p)) {
				page_cow_list_remove(&p->linked, l);
				kfree(l);
				return NULL;
			}
		}
		// Copy that page table to this new list
		if (!page_list_copy_page_table(l, p)) {
			page_cow_list_remove(&p->linked, l);
			kfree(l);
			return NULL;
		}
		
		page_list_map(p);
	}
	
	page_list_t* head = *list;
	if (head) {
		l->next = head;
		head->prev = l;
	}
	*list = l;
	
	set_multitasking_enabled(flags);
	
	return l;
}

// Remove a page from the list
bool page_list_remove(page_list_t** list, uint32_t vaddr) {
	if (!list)
		return NULL;
	
	page_list_t* t = *list;
	while (t) {
		if (t->vaddr == vaddr) {
			if (t->prev)
				t->prev->next = t->next;
			else
				*list = t->next;
			if (t->owner)
				page_physical_free((void*)t->paddr);
			kfree(t);
			return true;
		}
		t = t->next;
	}
	
	return false;
}

// Returns true if the page is in the page list
bool page_list_exists(page_list_t** list, uint32_t vaddr) {
	return (page_list_get(list, vaddr, 0, false) != NULL);
}

// Returns the page in the page list if it exists.
// If it doesn't exist and allocate=true, it will try to add it to the list and return that
page_list_t* page_list_get(page_list_t** list, uint32_t vaddr, uint32_t permissions, bool allocate) {
	if (!list)
		return NULL;
	
	page_list_t* t = *list;
	while (t) {
		if (t->vaddr == vaddr)
			return t;
		t = t->next;
	}
	
	if (!allocate)
		return NULL;
	
	return page_list_add(list, vaddr, permissions);
}

// Map a page into memory
void page_list_map(page_list_t* list) {
	if (list->page_table) {
		vm_map_page_table(list->vaddr, vm_virtual_to_physical((uint32_t)list->page_table->pages), list->permissions);
	} else {
		vm_map_page(list->vaddr, list->paddr, (list->copy_on_write ?
										   (list->permissions & ~MEMORY_WRITE) : list->permissions));
	}
}

// Perform a copy on write
bool page_list_copy_on_write(page_list_t* list, uint32_t address) {
	// Get a four kb page and map it in then copy it
	bool flags = set_multitasking_enabled(false);
	
	// Make a 4kb copy of this page
	void* copy = page_physical_get_aligned_four_kb(VIRTUAL_MEMORY_USER);
	uint32_t vaddr = vm_get_next_unmapped_page(VIRTUAL_MEMORY_KERNEL);
	if (!vaddr) {
		page_physical_free_aligned_four_kb(copy);
		set_multitasking_enabled(flags);
		return false;
	}
	uint32_t offset = (uint32_t)copy & (FOUR_MB_SIZE - 1);
	vm_map_page(vaddr, (uint32_t)copy - offset, MEMORY_WRITE | MEMORY_KERNEL);
	memcpy((void*)(vaddr + offset), (void*)(address & ~(FOUR_KB_SIZE - 1)), FOUR_KB_SIZE);
	vm_unmap_page(vaddr);
	uint32_t page = (address - list->vaddr) / FOUR_KB_SIZE;
	uint32_t old_paddr = list->page_table->pages[page] & ~(FOUR_KB_SIZE - 1);
	list->page_table->pages[page] = vm_create_page_table_entry((uint32_t)copy, list->permissions);
	
	if (page_list_read_bitmap(list->page_table->owners, page)) {
		// If we already owned a page before this, look for a new owner for that page.
		// If everybody else is already an owner, deallocate it
		page_cow_list_t* t = list->linked;
		bool found = false;
		while (t) {
			if ((t->entry->page_table->pages[page] & ~(FOUR_KB_SIZE - 1)) == old_paddr) {
				found = true;
				page_list_write_bitmap(t->entry->page_table->owners, page, true);
				break;
			}
			t = t->next;
		}
		if (!found)
			page_physical_free_aligned_four_kb((void*)old_paddr);
	}

	page_list_write_bitmap(list->page_table->owners, page, true);
	
	page_list_map(list);
	
	set_multitasking_enabled(flags);
	
	return true;
}

// Dealloc a whole page list
void page_list_dealloc(page_list_t* list) {
	page_list_t* t = list;
	while (t) {
		page_list_t* next = t->next;
		if (t->page_table) {
			for (uint32_t z = 0; z < NUM_PAGE_TABLE_ENTRIES; z++) {
				if (page_list_read_bitmap(t->page_table->owners, z)) {
					void* addr = (void*)(t->page_table->pages[z] & ~(FOUR_KB_SIZE - 1));
					// Only free it if this wasn't part of the original paddr
					if (!((uint32_t)addr >= t->paddr && (uint32_t)addr < t->paddr + FOUR_MB_SIZE))
						page_physical_free_aligned_four_kb(addr);
				}
			}
			if (t->page_table->pages)
				page_free_aligned_four_kb(t->page_table->pages);
			kfree(t->page_table);
		}
		if (t->owner)
			page_physical_free((void*)t->paddr);
		
		// Remove this from other's linked page list
		page_cow_list_t* c = t->linked;
		while (c) {
			page_cow_list_remove(&c->entry->linked, t);
			
			page_cow_list_t* prev = c;
			c = c->next;
			kfree(prev);
		}
		
		kfree(t);
		t = next;
	}
}
