
#include "memory.h"
#include <boot/x86_desc.h>
#include <common/lib.h>
#include <drivers/terminal/terminal.h>
#include <program/task.h>
#include "page_list.h"

#define PAGE_DIRECTORY_NUM_ENTRIES	1024
#define PAGE_TABLE_NUM_ENTRIES		1024
#define PAGE_DIRECTORY_NUM_KERNEL_ENTRIES	256

#define INITIAL_KERNEL_STACK_SIZE   (1024 * 128)

#define VRAM_START					0xB8000
#define VBE_START					0xA0000
#define VBE_END						0xB8000

// VM Allocator
#define VM_BITMAP_SIZE				(1024 / 8 / sizeof(uint32_t))			// 4GB / 4MB / 32
#define VM_USER_START				0x00									// 0GB / 4MB / 32
#define VM_KERNEL_START				0x18									// 3GB / 4MB / 32

// Special pages
#define USER_PAGE_DIRECTORY_ENTRY	0x97
#define USER_PAGE_TABLE_ENTRY		0x7
#define KERNEL_PAGE_TABLE_ENTRY		0x3
#define KERNEL_PAGE_DIRECTORY_ENTRY	0x193
#define UNUSED_PAGE					0x0
#define UNUSED_USER_PAGE			0x6

// Page values
#define PAGE_PRESENT_BIT			0x1
#define PAGE_WRITE_BIT				0x2
#define PAGE_USER_BIT				0x4
#define PAGE_DIRECTORY_BIT			0x90

uint32_t num_kernel_pages_reserved = 0;

//This is our page directory which holds all pages. It is aligned by 4 KB. It holds 1024 
//4mb pages/page tables for a total of 4gb.
uint32_t page_directory[PAGE_DIRECTORY_NUM_ENTRIES] __attribute__((aligned(FOUR_KB_SIZE)));
//Page table holds 1024 4kb pages from 0-4mb in video memory
uint32_t page_table[PAGE_TABLE_NUM_ENTRIES] __attribute__((aligned(FOUR_KB_SIZE)));
// Page table mappings for invalidation of page tables
uint16_t page_table_mappings[PAGE_DIRECTORY_NUM_ENTRIES];

// Kernel starting and ending points
extern uint32_t _kernel_start;
extern uint32_t _kernel_end;
#ifdef __APPLE__
uint32_t _kernel_start = 0;
uint32_t _kernel_end = 0;
#endif

//load cr3 register with the page directory's address
extern void load_cr3(uint32_t* directory_address);
//load cr0 to state paging is enabled and cr4 to state different sized pages
extern void load_cr0_and_cr4();
// Invalidate a particular page
extern void invalidate_page_address(void* addr);

// Bitmap for holding unmapped Virtual Memory locations
uint32_t vm_bitmap[VM_BITMAP_SIZE];

// Page directory lock
mutex_t page_directory_lock = MUTEX_UNLOCKED;

// Lock / unlock page tables - for use with below functions
void vm_lock() {
	down(&page_directory_lock);
}

void vm_unlock() {
	up(&page_directory_lock);
}

// Gets the address of the next unmapped 4MB page of the specific type
uint32_t vm_get_next_unmapped_page(uint32_t type) {
	return vm_get_next_unmapped_pages(1, type);
}

// Gets the address of the next unmapped 4MB pages of the specific type
uint32_t vm_get_next_unmapped_pages(uint32_t pages, uint32_t type) {
	// Standard linear search
	uint32_t z = VM_KERNEL_START;
	uint32_t end = VM_BITMAP_SIZE;
	if (type == VIRTUAL_MEMORY_USER) {
		z = VM_USER_START;
		end = VM_KERNEL_START;
	}
	
	static const uint32_t bits_in_uint32_t = 8 * sizeof(uint32_t);
	
	for (; z < end; z++) {
		if (vm_bitmap[z] != (uint32_t)-1) {
			uint32_t q;
			for (q = 0; q < bits_in_uint32_t; q++) {
				bool all = true;
				uint32_t n;
				for (n = 0; n < pages; n++) {
					if ((vm_bitmap[z + ((n + q) / bits_in_uint32_t)] >> ((n + q) % bits_in_uint32_t)) & 0x1) {
						all = false;
						break;
					}
				}
				if (all)
					return FOUR_MB_SIZE * (sizeof(uint32_t) * 8 * z + q);
			}
		}
	}
	
	return (uint32_t)NULL;
}

// Gets the address of the next unmapped 4MB pages of the specific type
uint32_t vm_get_next_unmapped_pages_from_back(uint32_t pages, uint32_t type) {
	// Standard linear search
	int end = VM_KERNEL_START;
	int z = VM_BITMAP_SIZE-1;
	if (type == VIRTUAL_MEMORY_USER) {
		end = 0;
		z = VM_KERNEL_START-1;
	}
	
	static const uint32_t bits_in_uint32_t = 8 * sizeof(uint32_t);
	
	for (; z >= end; z--) {
		if (vm_bitmap[z] != (uint32_t)-1) {
			for (int q = bits_in_uint32_t - 1; q >= 0; q--) {
				bool all = true;
				uint32_t n;
				for (n = 0; n < pages; n++) {
					if ((vm_bitmap[z + ((n + q) / bits_in_uint32_t)] >> ((n + q) % bits_in_uint32_t)) & 0x1) {
						all = false;
						break;
					}
				}
				if (all)
					return FOUR_MB_SIZE * (sizeof(uint32_t) * 8 * z + q);
			}
		}
	}
	
	return (uint32_t)NULL;
}

// Maps a virtual addresss (4MB aligned) to a physical address (4MB aligned)
void vm_map_page(uint32_t vaddr, uint32_t paddr, uint32_t permissions, bool preserve_context) {
	// Add this page to the current pcb so that it gets mapped in upon context switch
	if (preserve_context && current_pcb) {
		//down(&current_pcb->lock);
		page_list_update_mapping(&current_pcb->temporary_mappings, vaddr, paddr, permissions);
		//up(&current_pcb->lock);
	}
	
	// Set the page in the bitmap as mapped
	uint32_t page = vaddr / FOUR_MB_SIZE;
	uint32_t num_entries_per_bitmap = sizeof(uint32_t) * 8;
	vm_bitmap[page / num_entries_per_bitmap] |= (1 << (page % num_entries_per_bitmap));
	
	// If previous mapping was a page table, invalidate the page table
	if (!(page_directory[page] & PAGE_DIRECTORY_BIT))
		invalidate_page_address((void*)(page_table_mappings[page] * FOUR_MB_SIZE));
	
	// Map the page
	uint32_t data = PAGE_DIRECTORY_BIT;
	if (permissions & MEMORY_READ)
		data |= PAGE_PRESENT_BIT;
	if (!(permissions & MEMORY_KERNEL))
		data |= PAGE_USER_BIT;
	if (permissions & MEMORY_WRITE)
		data |= PAGE_WRITE_BIT;
	page_directory[page] = paddr | data;
		
	invalidate_page_address((void*)vaddr);
}

// Maps a virtual address (4MB aligned) to a physical page table (4kb aligned)
void vm_map_page_table(uint32_t vaddr, uint32_t* page_table, uint32_t* page_table_vaddr, uint32_t permissions) {
	uint32_t page = vaddr / FOUR_MB_SIZE;
	uint32_t num_entries_per_bitmap = sizeof(uint32_t) * 8;
	vm_bitmap[page / num_entries_per_bitmap] |= (1 << (page % num_entries_per_bitmap));
	
	// If previous mapping was a page table, invalidate the page table
	if (!(page_directory[page] & PAGE_DIRECTORY_BIT))
		invalidate_page_address((void*)(page_table_mappings[page] * FOUR_MB_SIZE));
	
	uint32_t data = 0;
	if (permissions & MEMORY_READ)
		data |= PAGE_PRESENT_BIT;
	if (!(permissions & MEMORY_KERNEL))
		data |= PAGE_USER_BIT;
	if (permissions & MEMORY_WRITE)
		data |= PAGE_WRITE_BIT;
	page_directory[page] = (uint32_t)page_table | data;
	page_table_mappings[page] = ((uint32_t)page_table_vaddr) / FOUR_MB_SIZE;
	
	invalidate_page_address((void*)vaddr);
	invalidate_page_address((void*)page_table_vaddr);
}

// Create a page table entry (address must be 4kb aligned)
uint32_t vm_create_page_table_entry(uint32_t paddr, uint32_t permissions) {
	uint32_t data = 0;
	if (permissions & MEMORY_READ)
		data |= PAGE_PRESENT_BIT;
	if (!(permissions & MEMORY_KERNEL))
		data |= PAGE_USER_BIT;
	if (permissions & MEMORY_WRITE)
		data |= PAGE_WRITE_BIT;
	return paddr | data;
}

// Unmaps a virtual address and allows it to be used
void vm_unmap_page(uint32_t vaddr, bool preserve_context) {
	// Remove this page from the current pcb so that it gets unmapped upon context switch
	if (preserve_context && current_pcb) {
		//down(&current_pcb->lock);
		page_list_remove(&current_pcb->temporary_mappings, vaddr);
		//up(&current_pcb->lock);
	}
	
	// Free it in the bitmap
	uint32_t page = vaddr / FOUR_MB_SIZE;
	uint32_t num_entries_per_bitmap = sizeof(uint32_t) * 8;
	vm_bitmap[page / num_entries_per_bitmap] &= ~(1 << (page % num_entries_per_bitmap));
	
	// If previous mapping was a page table, invalidate the page table
	if (!(page_directory[page] & PAGE_DIRECTORY_BIT))
		invalidate_page_address((void*)(page_table_mappings[page] * FOUR_MB_SIZE));
	
	// Unmap it
	page_directory[page] = UNUSED_PAGE;
	
	invalidate_page_address((void*)vaddr);
}

// Unmaps a virtual address range
void vm_unmap_pages(uint32_t start, uint32_t end) {
	// Free it in the bitmap
	uint32_t start_page = start / FOUR_MB_SIZE;
	uint32_t end_page = end / FOUR_MB_SIZE;
	uint32_t num_entries_per_bitmap = sizeof(uint32_t) * 8;

	// First and last, then inbetween
	uint32_t sp = start_page / num_entries_per_bitmap;
	uint32_t ep = end_page / num_entries_per_bitmap;
	if (sp == ep) {
		vm_bitmap[sp] &= ((1 << (start_page % num_entries_per_bitmap)) - 1) |
			(-1 ^ ((1 << (end_page % num_entries_per_bitmap)) - 1));
	} else {
		vm_bitmap[start_page / num_entries_per_bitmap] &= ((1 << (start_page % num_entries_per_bitmap)) - 1);
		vm_bitmap[end_page / num_entries_per_bitmap] &= -1 ^ ((1 << (end_page % num_entries_per_bitmap)) - 1);
		if (ep - sp >= 2)
			memset(&vm_bitmap[start_page / num_entries_per_bitmap + 1], 0, (ep - sp - 1) * sizeof(uint32_t));
	}
	
	memset(&page_directory[start_page], UNUSED_PAGE, end_page - start_page);
	/*for (unsigned int z = start_page; z < end_page; z++) {
		// If previous mapping was a page table, invalidate the page table
		if (!(page_directory[z] & PAGE_DIRECTORY_BIT))
			invalidate_page_address((void*)(page_table_mappings[z] * FOUR_MB_SIZE));
		page_directory[z] = UNUSED_PAGE;
	}*/
	
	flush_tlb();
}

// Returns whether the 4MB page is mapped
bool vm_is_page_mapped(uint32_t vaddr) {
	uint32_t page = vaddr / FOUR_MB_SIZE;
	uint32_t num_entries_per_bitmap = sizeof(uint32_t) * 8;
	
	return ((vm_bitmap[page / num_entries_per_bitmap] & (1 << (page % num_entries_per_bitmap))) != 0);
}

// Gets the page a virtual page is mapped to
void* vm_virtual_to_physical(uint32_t vaddr) {
	uint32_t page = vaddr / FOUR_MB_SIZE;
	return (void*)((page_directory[page] & ~(FOUR_MB_SIZE - 1)) + (vaddr & (FOUR_MB_SIZE - 1)));
}

// Gets the type for a virtual page
uint32_t vm_get_virtual_page_type(uint32_t vaddr) {
	if (vaddr >= VM_KERNEL_ADDRESS)
		return VIRTUAL_MEMORY_KERNEL;
	return VIRTUAL_MEMORY_USER;
}

// Flush the TLB
void flush_tlb() {
	asm volatile ("movl %cr3, %eax\n"
				  "movl %eax, %cr3");
}

// Complete setup of paging once the kernel has gained control
void complete_paging_setup() {
	// Adjust the esp
	uint32_t esp, ebp;
	asm volatile("movl %%esp, %0\n"
				 "movl %%ebp, %1\n"
				 :  "=&r"(esp), "=&r"(ebp));
	esp += VM_KERNEL_ADDRESS;
	ebp += VM_KERNEL_ADDRESS;
	asm volatile("movl %0, %%esp\n"
				 "movl %1, %%ebp\n"
				 :
				 : "r"(esp), "r"(ebp));
	// Adjust saved ebp too
	*((uint32_t*)ebp) += VM_KERNEL_ADDRESS;
		
	// Unmap lower half kernel
	vm_unmap_pages(FOUR_MB_SIZE, 4 * FOUR_MB_SIZE);
}

// Set up paging
// Note: this funciton must only use physical addresses since paging has not been set up yet
void setup_pages() {
	int i;										// For looping over page directory / table
	
	// We need physical addresses because paging haven't been set up
	uint32_t* p_page_directory = (uint32_t*)((uint32_t)page_directory - VM_KERNEL_ADDRESS);
	uint32_t* p_page_table_mappings = (uint32_t*)((uint32_t)page_table_mappings - VM_KERNEL_ADDRESS);
	uint32_t* p_vm_bitmap = (uint32_t*)((uint32_t)vm_bitmap - VM_KERNEL_ADDRESS);
	uint32_t* p_page_table = (uint32_t*)((uint32_t)page_table - VM_KERNEL_ADDRESS);
	uint32_t* p_kernel_pages = (uint32_t*)((uint32_t)&num_kernel_pages_reserved - VM_KERNEL_ADDRESS);

	// Set all pages to unused
	for (i = 0; i < PAGE_DIRECTORY_NUM_ENTRIES; i++) {
		p_page_directory[i] = UNUSED_PAGE;
		p_page_table_mappings[i] = 0;
	}
	// Set all VM entries to free
	for (i = 0; i < VM_BITMAP_SIZE; i++)
		p_vm_bitmap[i] = 0;
	
	// Loop through all 1024 4KB pages to make up the first 4MB
	for (i = 0; i < PAGE_TABLE_NUM_ENTRIES; i++) {
		int code = UNUSED_USER_PAGE;
		// Move each page by 4kb
		uint32_t address = i * FOUR_KB_SIZE;
		if (address == VRAM_START)
			code = KERNEL_PAGE_TABLE_ENTRY;
		else if (address >= VBE_START && address < VBE_END)
			code = KERNEL_PAGE_TABLE_ENTRY;
		// Set the page table entry
		p_page_table[i] = address | code;
	}
	
	// Find out how many 4MB blocks we need to reserve
	uint32_t space_needed = (uint32_t)&_kernel_end - VM_KERNEL_ADDRESS + INITIAL_KERNEL_STACK_SIZE;
	uint32_t pages_needed = ((space_needed - 1) / FOUR_MB_SIZE) + 1;
	*p_kernel_pages = pages_needed;
	
	// Set this value in the the page directories to point to the page table
	p_page_directory[0] = (uint32_t)p_page_table | KERNEL_PAGE_TABLE_ENTRY;
	p_page_table_mappings[0] = ((uint32_t)page_table) / FOUR_MB_SIZE;
	p_page_directory[VM_KERNEL_ADDRESS / FOUR_MB_SIZE] = (uint32_t)p_page_table | KERNEL_PAGE_TABLE_ENTRY;
	// Reserve spacefor the kernel (mapped from 4MB in physical memory)
	// We must keeped this mapped from 4MB in virtual memory until we jump to the higher address space.
	p_vm_bitmap[0] |= (1 << pages_needed) - 1;
	p_vm_bitmap[VM_KERNEL_START] |= (1 << pages_needed) - 1;
	for (i = 1; i < pages_needed; i++) {
		p_page_directory[i] = (i * FOUR_MB_SIZE) | KERNEL_PAGE_DIRECTORY_ENTRY;
		p_page_directory[i + VM_KERNEL_ADDRESS / FOUR_MB_SIZE] = (i * FOUR_MB_SIZE) | KERNEL_PAGE_DIRECTORY_ENTRY;
	}
	
	// Set the page directory address in the CR3 register, CR4 as we have different sized pages and CR0 enable paging
	load_cr3(p_page_directory);
	load_cr0_and_cr4();
}
