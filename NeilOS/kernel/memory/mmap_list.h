//
//  mmap_list.h
//  product
//
//  Created by Neil Singh on 1/6/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef MMAP_LIST_H
#define MMAP_LIST_H

#include <common/types.h>
#include <common/concurrency/semaphore.h>
#include <syscalls/descriptor.h>

struct pcb;

// A single mmapped region
typedef struct mmap_list {
	// Start and end of region
	// Note: part of the region might be unmapped, you need to check to page_list tables of the process to be sure
	uint32_t start;
	uint32_t end;	// non-inclusive
	
	uint32_t permissions;
	file_descriptor_t* file;
	uint32_t offset;
	bool shared;
	
	mutex_t lock;
	struct mmap_list* next;
	struct mmap_list* prev;
} mmap_list_t;

// Create a new mmap region
mmap_list_t* mmap_list_create(uint32_t start, uint32_t end, uint32_t permissions, file_descriptor_t* f,
							  uint32_t offset, bool shared);

// Link a mmap region into a list
void mmap_list_link(mmap_list_t* t, mmap_list_t** list);

// Does a memory mapping exist at the specific address
mmap_list_t* mmap_list_address_exists(mmap_list_t* list, uint32_t addr);

// Does a memory mapping exist anywhere in the region
mmap_list_t* mmap_list_region_exists(mmap_list_t* list, uint32_t start, uint32_t end);

// Remove a memory mapping region (this can cause a region to be split into multiple regions or multiple deletions
// ex: [ 0x0000, 0x1000 ) removing [ 0x0200, 0x0400 ) -> [ 0x0000, 0x200 ), [ 0x400, 0x1000 ) or
// [ 0x0000, 0x1000 ), [ 0x2000, 0x3000 ) removing [ 0x800, 0x2200 ) -> [ 0x000, 0x800 ), [ 0x2200, 0x3000 ))
// Frees associated page tables if pcb != NULL
bool mmap_list_region_remove(mmap_list_t** list, uint32_t start, uint32_t end, struct pcb* pcb);

// Process a list during a page fault (return true if there was a hit)
bool mmap_list_process(mmap_list_t* list, uint32_t address, uint32_t code, struct pcb* pcb);

// Copy a list
mmap_list_t* mmap_list_copy(mmap_list_t* list);

// Sync a address region (if invalidate is true, all other mappings of the same file across all processes
// will be invalidate so that they can be reloaded with the new values)
void mmap_list_sync(mmap_list_t* list, uint32_t start, uint32_t end, bool invalidate);

// Dealloc a single list entry (and frees the associated pagelist tables too if pcb != NULL)
void mmap_list_dealloc(mmap_list_t* t, mmap_list_t** list, struct pcb* pcb);

// Dealloc a list
void mmap_list_dealloc_list(mmap_list_t* list);

#endif /* MMAP_LIST_H */
