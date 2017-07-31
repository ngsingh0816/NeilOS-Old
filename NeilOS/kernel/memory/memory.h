#ifndef MEMORY_H
#define MEMORY_H

#include "common/types.h"
#include "allocation/page_allocator.h"
#include "allocation/heap.h"

/*
	Virtual Memory Map
	0GB-1GB: Kernel
	1GB-2GB: Shared Libraries
	2GB-4GB: User
 */

// Virtual Memory Allocator
#define VIRTUAL_MEMORY_KERNEL			0
#define VIRTUAL_MEMORY_SHARED			1
#define VIRTUAL_MEMORY_USER				2

// Page permissions
#define MEMORY_WRITE				(1 << 0)
#define MEMORY_KERNEL				(1 << 1)

// Common sizes
#define FOUR_KB_SIZE				4096
#define FOUR_MB_SIZE				(4096 * 1024)
#define ONE_GB_SIZE					(uint32_t)(1024 * 1024 * 1024)

#define NUM_PAGE_TABLE_ENTRIES		1024

// Gets the address of the next unmapped 4MB page of the specific type
uint32_t vm_get_next_unmapped_page(uint32_t type);

// Gets the address of the next unmapped 4MB pages of the specific type
uint32_t vm_get_next_unmapped_pages(uint32_t pages, uint32_t type);

// Maps a virtual address (4MB aligned) to a physical address (4MB aligned)
void vm_map_page(uint32_t vaddr, uint32_t paddr, uint32_t permissions);

// Maps a virtual address (4MB aligned) to a physical page table (4kb aligned)
void vm_map_page_table(uint32_t vaddr, uint32_t* page_table, uint32_t permissions);

// Create a page table entry
uint32_t vm_create_page_table_entry(uint32_t paddr, uint32_t permissions);

// Unmaps a virtual address and allows it to be used
void vm_unmap_page(uint32_t vaddr);

// Returns whether the 4MB page is mapped
bool vm_is_page_mapped(uint32_t vaddr);

// Gets the page a virtual page is mapped to
void* vm_virtual_to_physical(uint32_t vaddr);

// Gets the type for a virtual page
uint32_t vm_get_virtual_page_type(uint32_t vaddr);

// Flush the TLB
void flush_tlb();

//Sets-up paging and specific control registers to enable paging
int setup_pages();

#endif /* MEMORY_H */
