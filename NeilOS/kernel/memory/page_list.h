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
#include <common/concurrency/semaphore.h>
#include "memory.h"

struct page_list;

typedef struct page_cow_list {
	// Linked page_list
	struct page_list* entry;
	semaphore_t lock;
	
	struct page_cow_list* next;
	struct page_cow_list* prev;
} page_cow_list_t;

typedef struct {
	uint32_t* pages;
	// Whether this page_table owns the corresponding 4kb page
	uint32_t owners[NUM_PAGE_TABLE_ENTRIES / sizeof(uint32_t) / 8];
	// Whether or not this page should be copied on write or just referenced directly
	uint32_t cow[NUM_PAGE_TABLE_ENTRIES / sizeof(uint32_t) / 8];
} page_table_t;

// Linked list of memory pages
typedef struct page_list {
	struct page_list* next;
	struct page_list* prev;
	
	uint32_t paddr;
	uint32_t vaddr;
	
	uint32_t permissions;
	bool copy_on_write;
	page_cow_list_t* linked;
	bool owner;
	
	page_table_t* page_table;
	
	mutex_t lock;
} page_list_t;

// Helper to read a bitmap
bool page_list_read_bitmap(uint32_t* bitmap, uint32_t index);

// Helper to write a bitmap
void page_list_write_bitmap(uint32_t* bitmap, uint32_t index, bool value);

// Sets up the page table
bool page_list_set_up_page_table(page_list_t* list);

// Add a page to the list and return it
page_list_t* page_list_add(page_list_t** list, uint32_t vaddr, uint32_t permissions);

// Add a page to the list and return it (already allocated page)
page_list_t* page_list_add_mapping(page_list_t** list, uint32_t vaddr, uint32_t paddr, uint32_t permissions);

// Update (add if doesn't exist) a page to the list and return it (already allocated page)
page_list_t* page_list_update_mapping(page_list_t** list, uint32_t vaddr, uint32_t paddr, uint32_t permissions);

// Add a page to the list and return it by copying another page entry
page_list_t* page_list_add_copy(page_list_t** list, page_list_t* p);

// Remove a page from the list (returns false if entry does not exist)
bool page_list_remove(page_list_t** list, uint32_t vaddr);

// Returns true if the page is in the page list
bool page_list_exists(page_list_t** list, uint32_t vaddr);

// Returns the page in the page list if it exists.
// If it doesn't exist and allocate=true, it will try to add it to the list and return that
page_list_t* page_list_get(page_list_t** list, uint32_t vaddr, uint32_t permissions, bool allocate);

// Same as above except doesn't allocate physical memory if creating a new page
page_list_t* page_list_get_no_mem(page_list_t** list, uint32_t vaddr,
								  uint32_t permissions, bool allocate);

// Map a page into memory (if preserve_context is set to true, then the page will be persistent across context switches)
// Note: do not set preserve_context=true if the page is already contained in the process's page mappings
void page_list_map(page_list_t* list, bool preserve_context);

// Map a page list into memory
void page_list_map_list(page_list_t* list, bool preserve_context);

// Unmap a page from memory (if preserve_context is set to true, then the page will be removed from being persistent
// across context switches)
void page_list_unmap(page_list_t* list, bool preserve_context);

// Unmap a page list from memory
void page_list_unmap_list(page_list_t* list, bool preserve_context);

// Perform a copy on write
bool page_list_copy_on_write(page_list_t* list, uint32_t address);

// Dealloc a whole page list
void page_list_dealloc(page_list_t* list);

// Dealloc a single page list entry
void page_list_dealloc_entry(page_list_t* t, page_list_t** list);

#endif /* PAGE_LIST_H */
