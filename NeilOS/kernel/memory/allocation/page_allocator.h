//
//  page_allocator.h
//  MemoryAllocator
//
//  Created by Neil Singh on 12/21/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#ifndef page_allocator_h
#define page_allocator_h

#include <common/types.h>

#define MEMORY_PAGE_SIZE		(1024 * 64)			// 64 KB

// This is a virtual and physical memory allocator (it returns virtual addresses)

// Initialize the base memory
void page_allocator_init();

// Gets a 4kb aligned 4kb virtual page
void* page_get_aligned_four_kb();

// Gets a 4kb aligned 4kb physical page (must be freed with page_free_aligned_four_kb)
void* page_physical_get_aligned_four_kb(uint32_t type);

// Frees a 4kb aligned 4kb virtual page
void page_free_aligned_four_kb(void* addr);

// Frees a 4kb aligned 4kb physical page (returns true if page directory entry is now unused)
bool page_physical_free_aligned_four_kb(void* addr);

// Get a number of 64KB pages (type is KERNEL, SHARED, or USER)
void* page_get(uint32_t size, uint32_t type);

// Get a number of 64KB physical pages
void* page_physical_get(uint32_t size);

// Get a number of 4MB pages
void* page_get_four_mb(uint32_t num, uint32_t type);

void* page_physical_get_four_mb(uint32_t num);

// Free kernel pages (virtual addresses)
void page_free(void* addr);

// Free kernel pages (physical addresses)
void page_physical_free(void* addr);

#endif /* page_allocator_h */
