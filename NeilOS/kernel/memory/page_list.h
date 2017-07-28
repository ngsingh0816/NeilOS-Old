//
//  page_list.h
//  NeilOS
//
//  Created by Neil Singh on 7/12/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef PAGE_LIST_H
#define PAGE_LIST_H

#include <common/types.h>
#include "memory.h"

// Linked list of memory pages
typedef struct page_list {
	struct page_list* next;
	struct page_list* prev;
	
	uint32_t paddr;
	uint32_t vaddr;
	
	uint32_t* page_table;
	uint32_t permissions;
	bool copy_on_write;
	bool owner;
} page_list_t;

// Add a page to the list and return it
page_list_t* page_list_add(page_list_t** list, uint32_t vaddr, uint32_t permissions);

// Add a page to the list and return it by copying another page entry
page_list_t* page_list_add_copy(page_list_t** list, page_list_t* p, bool copy_on_write);

// Remove a page from the list (returns false if entry does not exist)
bool page_list_remove(page_list_t** list, uint32_t vaddr);

// Returns true if the page is in the page list
bool page_list_exists(page_list_t** list, uint32_t vaddr);

// Returns the page in the page list if it exists.
// If it doesn't exist and allocate=true, it will try to add it to the list and return that
page_list_t* page_list_get(page_list_t** list, uint32_t vaddr, uint32_t permissions, bool allocate);

// Map a page into memory
void page_list_map(page_list_t* list);

// Perform a copy on write
bool page_list_copy_on_write(page_list_t* list);

// Dealloc a whole page list
void page_list_dealloc(page_list_t* list);

#endif /* PAGE_LIST_H */
