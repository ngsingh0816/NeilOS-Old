
#include "memory.h"
#include <boot/x86_desc.h>
#include <common/lib.h>
#include <drivers/terminal/terminal.h>

#define PAGE_DIRECTORY_NUM_ENTRIES	1024
#define PAGE_TABLE_NUM_ENTRIES		1024
#define PAGE_DIRECTORY_NUM_KERNEL_ENTRIES	256

#define VRAM_START					0xB8000
#define VBE_START					0xA0000
#define VBE_END						0xB8000

// VM Allocator
#define VM_BITMAP_SIZE				(1024 / 8 / sizeof(uint32_t))			// 4GB / 4MB / 32
#define VM_KERNEL_START				0x00									// 0GB / 4MB / 32
#define VM_SHARED_START				0x08									// 1GB / 4MB / 32
#define VM_USER_START				0x10									// 2GB / 4MB / 32

//This is our page directory which holds all pages. It is aligned by 4 KB. It holds 1024 
//4mb pages/page tables for a total of 4gb.
uint32_t page_directory[PAGE_DIRECTORY_NUM_ENTRIES] __attribute__((aligned(FOUR_KB_SIZE)));
//Page table holds 1024 4kb pages from 0-4mb in video memory
uint32_t page_table[PAGE_TABLE_NUM_ENTRIES] __attribute__((aligned(FOUR_KB_SIZE)));
// User page table (for user VRAM)
uint32_t user_page_table[PAGE_TABLE_NUM_ENTRIES] __attribute__((aligned(FOUR_KB_SIZE)));


//load cr3 register with the page directory's address
extern void load_cr3(uint32_t* directory_address);
//load cr0 to state paging is enabled and cr4 to state different sized pages
extern void load_cr0_and_cr4();

// Bitmap for holding unmapped Virtual Memory locations
uint32_t vm_bitmap[VM_BITMAP_SIZE];

// Gets the address of the next unmapped 4MB page of the specific type
uint32_t vm_get_next_unmapped_page(uint32_t type) {
	return vm_get_next_unmapped_pages(1, type);
}

// Gets the address of the next unmapped 4MB pages of the specific type
uint32_t vm_get_next_unmapped_pages(uint32_t pages, uint32_t type) {
	// Standard linear search
	uint32_t z = VM_KERNEL_START;
	if (type == VIRTUAL_MEMORY_SHARED)
		type = VM_SHARED_START;
	if (type == VIRTUAL_MEMORY_USER)
		type = VM_USER_START;
	
	static const uint32_t bits_in_uint32_t = 8 * sizeof(uint32_t);
	
	for (; z < VM_BITMAP_SIZE; z++) {
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

// Maps a virtual addresss (4MB aligned) to a physical address (4MB aligned)
void vm_map_page(uint32_t vaddr, uint32_t paddr, uint32_t permissions) {
	// Set the page in the bitmap as mapped
	uint32_t page = vaddr / FOUR_MB_SIZE;
	uint32_t num_entries_per_bitmap = sizeof(uint32_t) * 8;
	vm_bitmap[page / num_entries_per_bitmap] |= (1 << (page % num_entries_per_bitmap));
	
	// Map the page
	page_directory[page] = paddr | permissions;
}

// Unmaps a virtual address and allows it to be used
void vm_unmap_page(uint32_t vaddr) {
	// Free it in the bitmap
	uint32_t page = vaddr / FOUR_MB_SIZE;
	uint32_t num_entries_per_bitmap = sizeof(uint32_t) * 8;
	vm_bitmap[page / num_entries_per_bitmap] &= ~(1 << (page % num_entries_per_bitmap));
	
	// Unmap it
	page_directory[page] = UNUSED_PAGE;
}

// Returns whether the 4MB page is mapped
bool vm_is_page_mapped(uint32_t vaddr) {
	uint32_t page = vaddr / FOUR_MB_SIZE;
	uint32_t num_entries_per_bitmap = sizeof(uint32_t) * 8;
	
	return (vm_bitmap[page / num_entries_per_bitmap] & (1 << (page % num_entries_per_bitmap)));
}

// Gets the page a virtual page is mapped to
void* vm_virtual_to_physical(uint32_t vaddr) {
	uint32_t page = vaddr / FOUR_MB_SIZE;
	return (void*)(page_directory[page] & ~(FOUR_MB_SIZE - 1));
}

// Gets the type for a virtual page
uint32_t vm_get_virtual_page_type(uint32_t vaddr) {
	if (vaddr < ONE_GB_SIZE)
		return VIRTUAL_MEMORY_KERNEL;
	else if (vaddr < 2 * ONE_GB_SIZE)
		return VIRTUAL_MEMORY_SHARED;
	return VIRTUAL_MEMORY_USER;
}

// Flush the TLB
void flush_tlb() {
	asm volatile ("movl %cr3, %eax\n"
				  "movl %eax, %cr3");
}

// Set up paging
int setup_pages() {
	int i;										// For looping over page directory / table

	// Set all pages to unused
	for (i = 0; i < PAGE_DIRECTORY_NUM_ENTRIES; i++)
		page_directory[i] = UNUSED_PAGE;
	// Set all VM entries to free
	for (i = 0; i < VM_BITMAP_SIZE; i++)
		vm_bitmap[i] = 0;
	
	// Loop through all 1024 4KB pages to make up the first 4MB
	for (i = 0; i < PAGE_TABLE_NUM_ENTRIES; i++) {
		int code = USER_NOT_PRESENT_PAGE;
		// Move each page by 4kb
		uint32_t address = i * FOUR_KB_SIZE;
		if (address == VRAM_START)
			code = KERNEL_PAGE_TABLE_ENTRY;
		else if (address >= VBE_START && address < VBE_END)
			code = KERNEL_PAGE_TABLE_ENTRY;
		// Set the page table entry
		page_table[i] = address | code;
	}
	
	// Fill in the user page table as well so it is cached
	for (i = 0; i < PAGE_TABLE_NUM_ENTRIES; i++) {
		int code = USER_NOT_PRESENT_PAGE;
		// Move each page by 4kb
		uint32_t address = i * FOUR_KB_SIZE;
		if (address == VRAM_START)
			code = USER_PAGE_TABLE_ENTRY;
		// Set the page table entry
		user_page_table[i] = address | code;
	}
	
	// Set this value in the the page directories to point to the page table
	vm_map_page(FOUR_MB_SIZE * 0, (uint32_t)page_table, KERNEL_PAGE_TABLE_ENTRY);
	// Reserve 12 MB for the kernel
	for (i = 1; i < 3; i++)
		vm_map_page(i * FOUR_MB_SIZE, i * FOUR_MB_SIZE, KERNEL_PAGE_DIRECTORY_ENTRY);
	
	// Set the page directory address in the CR3 register, CR4 as we have different sized pages and CR0 enable paging
	load_cr3(page_directory);
	load_cr0_and_cr4();
	
	return 0;
}
