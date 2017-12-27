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
	t->lock = MUTEX_LOCKED;
	t->next = NULL;
	t->prev = NULL;
	
	page_cow_list_t* l = *list;
	page_cow_list_t* prev = NULL;
	while (l) {
		down(&l->lock);
		if (prev)
			up(&prev->lock);
		prev = l;
		l = l->next;
	}
	if (!prev) {
		*list = t;
		up(&t->lock);
	} else {
		prev->next = t;
		t->prev = prev;
		up(&prev->lock);
		up(&t->lock);
	}
	
	return true;
}

// Remove an entry from the page cow list
bool page_cow_list_remove(page_cow_list_t** list, page_list_t* l) {
	if (*list)
		down(&((*list)->lock));
	page_cow_list_t* t = *list;
	while (t) {
		if (t->entry == l) {
			if (t->prev)
				t->prev->next = t->next;
			if (t->next) {
				down(&t->next->lock);
				t->next->prev = t->prev;
				up(&t->next->lock);
			}
			
			if (*list == t) {
				*list = t->next;
				// Update all mappings
				page_cow_list_t* l = t->next;
				down(&l->lock);
				while (l) {
					if (l->entry) {
						down(&l->entry->lock);
						l->entry->linked = t->next;
						up(&l->entry->lock);
					}
					
					page_cow_list_t* prev = l;
					l = l->next;
					if (l)
						down(&l->lock);
					up(&prev->lock);
				}
			}
			
			if (t->prev)
				up(&t->prev->lock);
			up(&t->lock);
			
			kfree(t);
			
			return true;
		}
		if (t->prev)
			up(&t->prev->lock);
		page_cow_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		else
			up(&prev->lock);
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

// Add a page to the list and return it (already allocated page)
page_list_t* page_list_add_mapping_owner(page_list_t** list, uint32_t vaddr, uint32_t paddr,
										 uint32_t permissions, bool owner) {
	if (!list)
		return NULL;
	
	page_list_t* t = kmalloc(sizeof(page_list_t));
	if (!t)
		return NULL;
	
	memset(t, 0, sizeof(page_list_t));
	t->lock = MUTEX_LOCKED;
	t->vaddr = vaddr;
	t->permissions = permissions;
	t->paddr = paddr;
	t->owner = owner;
	
	if (*list)
		down(&((*list)->lock));
	page_list_t* head = *list;
	if (head) {
		t->next = head;
		head->prev = t;
	}
	*list = t;
	if (head)
		up(&head->lock);
	up(&t->lock);
	
	return t;
}

// Add a page to the list
page_list_t* page_list_add(page_list_t** list, uint32_t vaddr, uint32_t permissions) {
	uint32_t paddr = (uint32_t)page_physical_get_four_mb(1);
	if (!paddr)
		return NULL;
	
	return page_list_add_mapping_owner(list, vaddr, paddr, permissions, true);
}

page_list_t* page_list_add_mapping(page_list_t** list, uint32_t vaddr, uint32_t paddr, uint32_t permissions) {
	return page_list_add_mapping_owner(list, vaddr, paddr, permissions, false);
}

// Update (add if doesn't exist) a page to the list and return it (already allocated page)
page_list_t* page_list_update_mapping(page_list_t** list, uint32_t vaddr, uint32_t paddr, uint32_t permissions) {
	page_list_t* t = page_list_get(list, vaddr, permissions, false);
	if (!t)
		return page_list_add_mapping(list, vaddr, paddr, permissions);
	
	down(&t->lock);
	t->paddr = paddr;
	t->permissions = permissions;
	up(&t->lock);
	
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
	page_table_t* page_table = kmalloc(sizeof(page_table_t));
	if (!page_table)
		return false;
	
	page_table->pages = page_get_aligned_four_kb();
	if (!page_table->pages) {
		kfree(page_table);
		return false;
	}
	
	// Set up to point to original adressses
	for (uint32_t z = 0; z < NUM_PAGE_TABLE_ENTRIES; z++) {
		page_table->pages[z] = vm_create_page_table_entry(list->paddr + z * FOUR_KB_SIZE,
																(list->permissions & ~MEMORY_WRITE));
	}
	
	// Set no pages as owners
	memset(page_table->owners, 0, NUM_PAGE_TABLE_ENTRIES / 8);
	list->page_table = page_table;
	
	return true;
}

// Add a page to the list and return it by copying another page entry
page_list_t* page_list_add_copy(page_list_t** list, page_list_t* p) {
	// Copy the list but point to the same address without write access
	page_list_t* l = kmalloc(sizeof(page_list_t));
	if (!l)
		return NULL;

	down(&p->lock);
	memcpy(l, p, sizeof(page_list_t));
	l->lock = MUTEX_LOCKED;
	l->next = NULL;
	l->prev = NULL;
	l->owner = false;
	l->page_table = NULL;
	l->linked = NULL;
	if ((p->permissions & MEMORY_WRITE)) {
		l->copy_on_write = true;
		p->copy_on_write = true;
		
		// Set up the page table for the old list
		if (!p->page_table) {
			if (!page_list_set_up_page_table(p)) {
				kfree(l);
				up(&p->lock);
				return NULL;
			}
		}
		// Copy that page table to this new list
		if (!page_list_copy_page_table(l, p)) {
			kfree(l);
			up(&p->lock);
			return NULL;
		}
		
		// Set the linked page lists
		if (!p->linked)
			page_cow_list_add(&p->linked, p);
		page_cow_list_add(&p->linked, l);
		l->linked = p->linked;
	}
	up(&p->lock);
	
	if (*list)
		down(&((*list)->lock));
	page_list_t* head = *list;
	if (head) {
		l->next = head;
		head->prev = l;
	}
	*list = l;
	if (head)
		up(&head->lock);
	up(&l->lock);
	
	return l;
}

void page_list_dealloc_mem(page_list_t* t) {
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
}

// Remove a page from the list
bool page_list_remove(page_list_t** list, uint32_t vaddr) {
	if (!list)
		return NULL;
	
	if (*list)
		down(&((*list)->lock));
	page_list_t* t = *list;
	while (t) {
		if (t->vaddr == vaddr) {
			if (t->prev)
				t->prev->next = t->next;
			else
				*list = t->next;
			
			if (t->linked) {
				up(&t->lock);
				page_cow_list_remove(&t->linked, t);
				down(&t->lock);
			}
			
			page_list_dealloc_mem(t);
			
			if (t->prev)
				up(&t->prev->lock);
			up(&t->lock);
			
			kfree(t);
			
			return true;
		}
		if (t->prev)
			up(&t->prev->lock);
		page_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		else
			up(&prev->lock);
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
	
	if (*list)
		down(&((*list)->lock));
	page_list_t* t = *list;
	while (t) {
		if (t->vaddr == vaddr) {
			up(&t->lock);
			return t;
		}
		page_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	
	if (!allocate)
		return NULL;
	
	return page_list_add(list, vaddr, permissions);
}

void page_list_map_internal(page_list_t* list, bool preserve_context) {
	if (list->page_table) {
		vm_map_page_table(list->vaddr, vm_virtual_to_physical((uint32_t)list->page_table->pages),
						  list->page_table->pages, list->permissions);
	} else {
		vm_map_page(list->vaddr, list->paddr, (list->copy_on_write ?
											   (list->permissions & ~MEMORY_WRITE) : list->permissions), preserve_context);
	}
}

// Map a page into memory
void page_list_map(page_list_t* list, bool preserve_context) {
	down(&list->lock);
	page_list_map_internal(list, preserve_context);
	up(&list->lock);
}

void page_list_map_list(page_list_t* list, bool preserve_context) {
	page_list_t* t = list;
	while (t) {
		page_list_map_internal(t, preserve_context);
		t = t->next;
	}
}

void page_list_unmap_internal(page_list_t* list, bool preserve_context) {
	vm_unmap_page(list->vaddr, preserve_context);
}

// Unmap a page from memory (if preserve_context is set to true, then the page will be removed from being persistent
// across context switches)
void page_list_unmap(page_list_t* list, bool preserve_context) {
	down(&list->lock);
	vm_unmap_page(list->vaddr, preserve_context);
	up(&list->lock);
}

// Unmap a page list
void page_list_unmap_list(page_list_t* list, bool preserve_context) {
	if (list)
		down(&list->lock);
	page_list_t* t = list;
	while (t) {
		page_list_unmap_internal(t, preserve_context);
		
		page_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
}

// Perform a copy on write
bool page_list_copy_on_write(page_list_t* list, uint32_t address) {
	// Make a 4kb copy of this page
	void* copy = page_physical_get_aligned_four_kb(VIRTUAL_MEMORY_USER);
	if (!copy) {
		return false;
	}
	vm_lock();
	uint32_t vaddr = vm_get_next_unmapped_page(VIRTUAL_MEMORY_KERNEL);
	if (!vaddr) {
		vm_unlock();
		page_physical_free_aligned_four_kb(copy);
		return false;
	}

	uint32_t offset = (uint32_t)copy & (FOUR_MB_SIZE - 1);
	vm_map_page(vaddr, (uint32_t)copy - offset, MEMORY_WRITE | MEMORY_KERNEL, false);
	memcpy((void*)(vaddr + offset), (void*)(address & ~(FOUR_KB_SIZE - 1)), FOUR_KB_SIZE);
	vm_unmap_page(vaddr, false);
	vm_unlock();
	down(&list->lock);
	uint32_t page = (address - list->vaddr) / FOUR_KB_SIZE;
	uint32_t old_paddr = list->page_table->pages[page] & ~(FOUR_KB_SIZE - 1);
	list->page_table->pages[page] = vm_create_page_table_entry((uint32_t)copy, list->permissions);
	
	if (page_list_read_bitmap(list->page_table->owners, page)) {
		// If we already owned a page before this, look for a new owner for that page.
		// If everybody else is already an owner, deallocate it
		page_cow_list_t* t = list->linked;
		up(&list->lock);
		if (t && t->entry)
			down(&t->lock);
		bool found = false;
		while (t && t->entry) {
			if (t->entry != list && (t->entry->page_table->pages[page] & ~(FOUR_KB_SIZE - 1)) == old_paddr) {
				found = true;
				page_list_write_bitmap(t->entry->page_table->owners, page, true);
				up(&t->lock);
				break;
			}
			
			page_cow_list_t* prev = t;
			t = t->next;
			if (t && t->entry)
				down(&t->lock);
			up(&prev->lock);
		}
		if (!found)
			page_physical_free_aligned_four_kb((void*)old_paddr);
		
		down(&list->lock);
	}

	page_list_write_bitmap(list->page_table->owners, page, true);
	
	up(&list->lock);
	page_list_map(list, false);
	
	return true;
}

// Dealloc a whole page list
void page_list_dealloc(page_list_t* list) {
	page_list_t* t = list;
	while (t) {
		// Remove this from the linked page list
		if (t->linked)
			page_cow_list_remove(&t->linked, t);
		down(&t->lock);
		t->linked = NULL;
		
		page_list_dealloc_mem(t);
		
		page_list_t* prev = t;
		t = t->next;
		up(&prev->lock);
		kfree(prev);
	}
}
