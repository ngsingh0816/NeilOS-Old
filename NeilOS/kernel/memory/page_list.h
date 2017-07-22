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

// Linked list of memory pages
typedef struct page_list {
	struct page_list* next;
	struct page_list* prev;
	
	uint32_t paddr;
	uint32_t vaddr;
} page_list_t;

// Add a page to the list and return it
page_list_t* page_list_add(page_list_t** list, uint32_t vaddr);

// Add a page to the list and return it by copying another page entry
page_list_t* page_list_add_copy(page_list_t** list, page_list_t* p, bool copy_on_write);

// Remove a page from the list (returns false if entry does not exist)
bool page_list_remove(page_list_t** list, uint32_t vaddr);

// Returns true if the page is in the page list
bool page_list_exists(page_list_t** list, uint32_t vaddr);

// Returns the page in the page list if it exists.
// If it doesn't exist and allocate=true, it will try to add it to the list and return that
page_list_t* page_list_get(page_list_t** list, uint32_t vaddr, bool allocate);

// Dealloc a whole page list
void page_list_dealloc(page_list_t* list);

#endif /* PAGE_LIST_H */
