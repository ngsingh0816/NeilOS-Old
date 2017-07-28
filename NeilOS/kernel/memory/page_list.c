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

// Add a page to the list and return it by copying another page entry
page_list_t* page_list_add_copy(page_list_t** list, page_list_t* p, bool copy_on_write) {
	if (copy_on_write) {
		// Copy the list but point to the same address without write access
		page_list_t* l = kmalloc(sizeof(page_list_t));
		if (!l)
			return NULL;
		memcpy(l, p, sizeof(page_list_t));
		l->next = NULL;
		l->prev = NULL;
		l->owner = false;
		if ((p->permissions & MEMORY_WRITE)) {
			l->copy_on_write = true;
			p->copy_on_write = true;
			page_list_map(p);
		}
		
		page_list_t* head = *list;
		if (head) {
			l->next = head;
			head->prev = l;
		}
		*list = l;
		
		return l;
	}
	
	bool flags = set_multitasking_enabled(false);
	uint32_t vaddr = vm_get_next_unmapped_page(VIRTUAL_MEMORY_KERNEL);
	if (vaddr == 0) {
		set_multitasking_enabled(flags);
		return NULL;
	}
	
	page_list_t* l = page_list_add(list, vaddr, p->permissions);
	if (!l) {
		set_multitasking_enabled(flags);
		return NULL;
	}
	
	vm_map_page(l->vaddr, l->paddr, MEMORY_WRITE | MEMORY_KERNEL);
	memcpy((void*)l->vaddr, (void*)p->vaddr, FOUR_MB_SIZE);
	vm_unmap_page(l->vaddr);
	l->vaddr = p->vaddr;
	
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
	vm_map_page(list->vaddr, list->paddr, (list->copy_on_write ?
										   (list->permissions & ~MEMORY_WRITE) : list->permissions));
}

// Perform a copy on write
bool page_list_copy_on_write(page_list_t* list) {
	bool flags = set_multitasking_enabled(false);
	
	list->copy_on_write = false;
	void* copy = page_get_four_mb(1, VIRTUAL_MEMORY_KERNEL);
	if (!copy)
		return false;
	
	memcpy(copy, (void*)list->vaddr, FOUR_MB_SIZE);
	list->paddr = (uint32_t)vm_virtual_to_physical((uint32_t)copy);
	vm_unmap_page((uint32_t)copy);
	list->owner = true;
	
	page_list_map(list);
	
	set_multitasking_enabled(flags);
	
	return true;
}

// Dealloc a whole page list
void page_list_dealloc(page_list_t* list) {
	page_list_t* t = list;
	while (t) {
		page_list_t* next = t->next;
		if (t->owner)
			page_physical_free((void*)t->paddr);
		kfree(t);
		t = next;
	}
}
