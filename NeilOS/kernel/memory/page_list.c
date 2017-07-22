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
page_list_t* page_list_add(page_list_t** list, uint32_t vaddr) {
	if (!list)
		return NULL;
	
	page_list_t* t = kmalloc(sizeof(page_list_t));
	if (!t)
		return NULL;
	
	memset(t, 0, sizeof(page_list_t));
	t->vaddr = vaddr;
	t->paddr = (uint32_t)page_physical_get_four_mb(1);
	if (!t->paddr) {
		kfree(t);
		return NULL;
	}
	
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
	// TODO: come back to this and implement copy on write
	bool flags = set_multitasking_enabled(false);
	uint32_t vaddr = vm_get_next_unmapped_page(VIRTUAL_MEMORY_KERNEL);
	if (vaddr == 0) {
		set_multitasking_enabled(flags);
		return NULL;
	}
	
	page_list_t* l = page_list_add(list, vaddr);
	if (!l) {
		set_multitasking_enabled(flags);
		return NULL;
	}
	
	vm_map_page(l->vaddr, l->paddr, KERNEL_PAGE_DIRECTORY_ENTRY);
	
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
	return (page_list_get(list, vaddr, false) != NULL);
}

// Returns the page in the page list if it exists.
// If it doesn't exist and allocate=true, it will try to add it to the list and return that
page_list_t* page_list_get(page_list_t** list, uint32_t vaddr, bool allocate) {
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
	
	return page_list_add(list, vaddr);
}

// Dealloc a whole page list
void page_list_dealloc(page_list_t* list) {
	page_list_t* t = list;
	while (t) {
		page_list_t* next = t->next;
		page_physical_free((void*)t->paddr);
		kfree(t);
		t = next;
	}
}
