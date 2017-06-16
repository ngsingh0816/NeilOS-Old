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

// This is a virtual and physical memory allocator (it returns virtual addresses)

// Initialize the base memory
void page_allocator_init();

// Get a number of 64KB pages (type is KERNEL, SHARED, or USER)
void* page_get(uint32_t size, uint32_t type);

// Get a number of 4MB pages
void* page_get_four_mb(uint32_t num, uint32_t type);

// Free kernel pages (virtual addresses)
void page_free(void* addr);

// Free kernel pages (physical addresses)
void physical_page_free(void* addr);

#endif /* page_allocator_h */
