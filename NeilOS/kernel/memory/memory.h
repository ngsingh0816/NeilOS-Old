#ifndef MEMORY_H
#define MEMORY_H

#include "common/types.h"
#include "allocation/page_allocator.h"
#include "allocation/heap.h"

/*
	Virtual Memory Map
	0GB-3GB: User
	3GB-4GB: Kernel (0xC0000000)
 */

// Virtual Memory Allocator
#define VIRTUAL_MEMORY_KERNEL			0
#define VIRTUAL_MEMORY_USER				1
#define VM_KERNEL_ADDRESS			0xC0000000

// Page permissions
#define MEMORY_WRITE				(1 << 0)
#define MEMORY_KERNEL				(1 << 1)

// Common sizes
#define FOUR_KB_SIZE				4096
#define FOUR_MB_SIZE				(4096 * 1024)
#define ONE_GB_SIZE					(uint32_t)(1024 * 1024 * 1024)

#define NUM_PAGE_TABLE_ENTRIES		1024

// Number of kernel pages reserved
extern uint32_t num_kernel_pages_reserved;

// Lock / unlock page tables - for use with below functions
void vm_lock();
void vm_unlock();

// Gets the address of the next unmapped 4MB page of the specific type
uint32_t vm_get_next_unmapped_page(uint32_t type);

// Gets the address of the next unmapped 4MB pages of the specific type
uint32_t vm_get_next_unmapped_pages(uint32_t pages, uint32_t type);

// Maps a virtual address (4MB aligned) to a physical address (4MB aligned)
// If preserves context is set to true, the mapping will persist across context switches.
// Do not set preserves context to true for kernel pages because they already are persistent.
void vm_map_page(uint32_t vaddr, uint32_t paddr, uint32_t permissions, bool preserve_context);

// Maps a virtual address (4MB aligned) to a physical page table (4kb aligned)
void vm_map_page_table(uint32_t vaddr, uint32_t* page_table, uint32_t* page_table_vaddr, uint32_t permissions);

// Create a page table entry
uint32_t vm_create_page_table_entry(uint32_t paddr, uint32_t permissions);

// Unmaps a virtual address and allows it to be used
void vm_unmap_page(uint32_t vaddr, bool preserve_context);

// Unmaps a range of virtual addresses from [start, end)
void vm_unmap_pages(uint32_t start, uint32_t end);

// Returns whether the 4MB page is mapped
bool vm_is_page_mapped(uint32_t vaddr);

// Gets the page a virtual page is mapped to
void* vm_virtual_to_physical(uint32_t vaddr);

// Gets the type for a virtual page
uint32_t vm_get_virtual_page_type(uint32_t vaddr);

// Flush the TLB
void flush_tlb();

// Invalidate a page
void invalidate_page_address(void* vaddr);

//Sets-up paging and specific control registers to enable paging
void setup_pages();

// Complete setup of paging once the kernel has gained control
void complete_paging_setup();

#endif /* MEMORY_H */
