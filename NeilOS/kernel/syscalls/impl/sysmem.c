//
//  sysmem.c
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "sysmem.h"
#include <program/task.h>
#include <common/log.h>
#include <memory/page_list.h>
#include <syscalls/interrupt.h>
#include <drivers/ipc/shmem.h>

#define	PROT_NONE	0x00
#define	PROT_READ	0x01
#define	PROT_WRITE	0x02
#define	PROT_EXEC	0x04

#define	MAP_SHARED	0x0001
#define	MAP_PRIVATE	0x0002
#define	MAP_COPY	MAP_PRIVATE

#define	MAP_FIXED	 0x0010
#define	MAP_RENAME	 0x0020
#define	MAP_NORESERVE	 0x0040
#define	MAP_INHERIT	 0x0080
#define	MAP_NOEXTEND	 0x0100
#define	MAP_HASSEMAPHORE 0x0200

#define MCL_CURRENT	0x0001
#define MCL_FUTURE	0x0002

#define	MS_SYNC		0x0000
#define MS_ASYNC	0x0001
#define MS_INVALIDATE	0x0002

#ifndef _POSIX_SOURCE
#define MS_KILLPAGES    0x0004
#define MS_DEACTIVATE   0x0008
#endif

#define	MAP_FILE		0x0000
#define	MAP_ANON		0x1000
#define	MAP_ANONYMOUS	0x1000

#define	MADV_NORMAL	0
#define	MADV_RANDOM	1
#define	MADV_SEQUENTIAL	2
#define	MADV_WILLNEED	3
#define	MADV_DONTNEED	4
#define	MADV_FREE		5
#define	POSIX_MADV_NORMAL	MADV_NORMAL
#define	POSIX_MADV_RANDOM	MADV_RANDOM
#define	POSIX_MADV_SEQUENTIAL	MADV_SEQUENTIAL
#define	POSIX_MADV_WILLNEED	MADV_WILLNEED
#define	POSIX_MADV_DONTNEED	MADV_DONTNEED

#define	MINCORE_INCORE	 	 0x
#define	MINCORE_REFERENCED	 0x2
#define	MINCORE_MODIFIED	 0x4
#define	MINCORE_REFERENCED_OTHER 0x8
#define	MINCORE_MODIFIED_OTHER	0x10

// Set the program break to a specific address
uint32_t brk(uint32_t addr) {
	LOG_DEBUG_INFO_STR("(0x%x)", addr);

	if (addr < USER_ADDRESS)
		return -ENOMEM;
	
	down(&current_pcb->lock);
	if (addr > current_pcb->brk) {
		// Allocate new pages
		uint32_t start = (current_pcb->brk-1) - ((current_pcb->brk-1) % FOUR_MB_SIZE) + FOUR_MB_SIZE;
		for (; start < addr; start += FOUR_MB_SIZE) {
			page_list_t* t = page_list_get(&current_pcb->page_list, start, MEMORY_RW, true);
			if (!t) {
				up(&current_pcb->lock);
				brk(current_pcb->brk);
				return -ENOMEM;
			}
			
			// Map this new page into memory
			page_list_map(t, false);
		}
	} else if (addr < current_pcb->brk) {
		uint32_t start = addr - (addr % FOUR_MB_SIZE) + FOUR_MB_SIZE;
		for (; start < current_pcb->brk; start += FOUR_MB_SIZE)
			page_list_remove(&current_pcb->page_list, start);
	}
	
	// Return the new break
	current_pcb->brk = addr;
	up(&current_pcb->lock);
	return 0;
}

// Offset the current program break by a specific ammount
void* sbrk(int32_t offset) {
	LOG_DEBUG_INFO_STR("(0x%x)", offset);

	uint32_t prev_brk = current_pcb->brk;
	if (brk(current_pcb->brk + offset) != 0)
		return (void*)-1;
	return (void*)prev_brk;
}

// Helper to find a number of available 4kb pages in the specified pcb (returns NULL if none are found)
// above the min address (must be 4mb aligned)
void* find_available_pages(pcb_t* pcb, uint32_t num, uint32_t min) {
	// Loop through all addresses and see if we can find "num" avaiable 4kb pages
	uint32_t vaddr = min;
	uint32_t len = 0;
	uint32_t start_vaddr = 0;
	while (vaddr < VM_KERNEL_ADDRESS) {
		page_list_t* current_page = current_page = page_list_get(&pcb->page_list, vaddr, 0, false);
		if (!current_page || (current_page && !current_page->page_table)) {
			// There was no page found here, go to the next (if it is not using a page table,
			// all of its memory is taken)
			vaddr += FOUR_MB_SIZE;
			start_vaddr = vaddr;
			len = 0;
			continue;
		}
		
		// Loop through all the 4kb pages of this page
		for (uint32_t z = 0; z < FOUR_MB_SIZE / FOUR_KB_SIZE; z++) {
			if (!current_page->page_table->pages[z]) {
				// This page is open, if we have enough then just return that.
				if (++len >= num)
					return (void*)start_vaddr;
			} else {
				// This page is not open, reset our search
				start_vaddr = vaddr + FOUR_KB_SIZE;
				len = 0;
			}
			vaddr += FOUR_KB_SIZE;
		}
	}
	
	return NULL;
}

// Helper to map contiguous 4kb pages into a pcb (allocating physical memory as needed)
bool map_contigous_pages(pcb_t* pcb, uint32_t start, uint32_t num_pages, uint32_t permissions, bool shared,
						 uint32_t* paddrs, uint32_t paddr_length, uint32_t initial_offset) {
	// Get first page
	page_list_t* current_page = NULL;
	uint32_t prev_addr = 0;
	uint32_t addr = start;
	uint32_t count = 0;
	uint32_t page_offset = initial_offset / FOUR_KB_SIZE;
	while (num_pages) {
		// If we have reached a different 4mb page, update the current page
		if (prev_addr / FOUR_MB_SIZE != addr / FOUR_MB_SIZE) {
			current_page = page_list_get_no_mem(&pcb->page_list, addr, MEMORY_RW, true);
			if (!current_page)
				return false;
			prev_addr = addr;
			
			if (!current_page->page_table) {
				down(&current_page->lock);
				if (!page_list_set_up_page_table(current_page)) {
					up(&current_page->lock);
					page_list_remove(&pcb->page_list, current_page->vaddr);
					return false;
				}
				memset(current_page->page_table->pages, 0, FOUR_KB_SIZE);
				up(&current_page->lock);
			}
			
			page_list_map(current_page, false);
		}
		
		// We are gauranteed that the page and page table has been set up by now
		uint32_t offset = (addr % FOUR_MB_SIZE) / FOUR_KB_SIZE;
		uint32_t paddr = 0;
		uint32_t perm = 0;
		bool paddr_used = false;
		if (paddrs && count + page_offset < paddr_length) {
			// Use the predetermined physical memory address if available
			paddr = paddrs[count + page_offset];
			perm = permissions;
			paddr_used = true;
		} else {
			// Otherwise, allocate a 4kb page and set the corrosponding page table entry to it
			paddr = (uint32_t)page_physical_get_aligned_four_kb(VIRTUAL_MEMORY_USER);
			if (!paddr) {
				page_list_remove(&pcb->page_list, current_page->vaddr);
				return false;
			}
		}
		down(&current_page->lock);
		current_page->page_table->pages[offset] = vm_create_page_table_entry(paddr, perm);
		page_list_write_bitmap(current_page->page_table->owners, offset, !paddr_used);
		page_list_write_bitmap(current_page->page_table->cow, offset, !shared);
		up(&current_page->lock);
		
		// Move on to the next 4kb page
		addr += FOUR_KB_SIZE;
		num_pages--;
		count++;
	}
	
	return true;
}

// Map a region of memory
void* mmap(void* addr, uint32_t length, int prot, int flags, int fd, uint32_t offset) {
	LOG_DEBUG_INFO_STR("(0x%x, 0x%x, 0x%x, 0x%x, %d, 0x%x)", addr, length, prot, flags, fd, offset);
	
	if (length == 0)
		return (void*)-EINVAL;
	if ((offset % FOUR_KB_SIZE) != 0)
		return (void*)-EINVAL;
	if (!(flags & MAP_ANONYMOUS) && !descriptors[fd])
		return (void*)-EBADF;
	if (((flags & MAP_PRIVATE) != 0) == ((flags & MAP_SHARED) != 0))
		return (void*)-EINVAL;
	
	// Adjust addr to nearest 4kb
	addr = (void*)((uint32_t)addr - ((uint32_t)addr % FOUR_KB_SIZE));
	down(&descriptors[fd]->lock);
	
	// Only allow files that are seekable
	if (!(flags & MAP_ANONYMOUS)) {
		if (!descriptors[fd]->llseek) {
			up(&descriptors[fd]->lock);
			return (void*)-EBADF;
		}
		uint64_t ret = descriptors[fd]->llseek(descriptors[fd], uint64_make(0, 0), SEEK_CUR);
		if ((int32_t)ret.high < 0) {
			up(&descriptors[fd]->lock);
			return (void*)-EBADF;
		}
	}
	
	// Setup permissions
	uint32_t perm = 0;
	if (prot & PROT_READ) {
		if (!(flags & MAP_ANONYMOUS) && (!descriptors[fd]->read || !(descriptors[fd]->mode & FILE_MODE_READ))) {
			up(&descriptors[fd]->lock);
			return (void*)-EACCES;
		}
		perm |= MEMORY_READ;
	}
	if (prot & PROT_WRITE) {
		if (!(flags & MAP_ANONYMOUS) && (!descriptors[fd]->write || !(descriptors[fd]->mode & FILE_MODE_WRITE)
										 || descriptors[fd]->mode & FILE_MODE_APPEND)) {
			up(&descriptors[fd]->lock);
			return (void*)-EACCES;
		}
		perm |= MEMORY_WRITE;
	}
	
	// Alocate new memory user mappings
	mmap_list_t* mapping = mmap_list_create((uint32_t)addr, (uint32_t)addr + length,
											perm, (flags & MAP_ANONYMOUS) ? NULL : descriptors[fd],
											offset, (flags & MAP_SHARED) != 0);
	up(&descriptors[fd]->lock);
	if (!mapping) {
		return (void*)-ENOMEM;
	}
	
	down(&current_pcb->lock);
	uint32_t num_4k_pages = (length - 1) / FOUR_KB_SIZE + 1;
	uint32_t num_4m_pages = (length - 1) / FOUR_MB_SIZE + 1;
	uint32_t min_vaddr = (current_pcb->brk - (current_pcb->brk % FOUR_MB_SIZE)) + FOUR_MB_SIZE;
	
	// Don't overwrite previous mappings unless we are told to
	if (mmap_list_region_exists(current_pcb->user_mappings, (uint32_t)addr, (uint32_t)addr + length)) {
		if (flags & MAP_FIXED) {
			if (!mmap_list_region_remove(&current_pcb->user_mappings, (uint32_t)addr,
										 (uint32_t)addr + length, current_pcb)) {
				up(&current_pcb->lock);
				kfree(mapping);
				return (void*)-ENOMEM;
			}
		} else
			addr = NULL;
	}

	if ((uint32_t)addr < min_vaddr) {
		if (flags & MAP_FIXED) {
			up(&current_pcb->lock);
			kfree(mapping);
			return (void*)-ENOMEM;
		}
		// See if any of the other pages after the program brk have room for us
		addr = find_available_pages(current_pcb, num_4k_pages, min_vaddr);
		
		// If no existing pages have room for us, then allocate new blocks
		if (!addr) {
			addr = (void*)vm_get_next_unmapped_pages_from_back(num_4m_pages, VIRTUAL_MEMORY_USER);
			if ((uint32_t)addr <= min_vaddr) {
				up(&current_pcb->lock);
				mmap_list_dealloc(mapping, &current_pcb->user_mappings, NULL);
				return (void*)-ENOMEM;
			}
		}
	}
	
	// Map this region into memory
	uint32_t paddr_count = 0;
	uint32_t* paddrs = NULL;
	if ((flags & MAP_SHARED) != 0 && mapping->file && mapping->file->type == SHARED_MEMORY_FILE_TYPE) {
		paddrs = shm_get_paddrs(mapping->file, &paddr_count);
		if (!paddrs) {
			up(&current_pcb->lock);
			mmap_list_dealloc(mapping, &current_pcb->user_mappings, NULL);
			return (void*)-ENOMEM;
		}
	}
	mapping->start = (uint32_t)addr;
	mapping->end = (uint32_t)addr + length;
	if (!map_contigous_pages(current_pcb, (uint32_t)addr, num_4k_pages, perm,
							 (flags & MAP_SHARED) != 0, paddrs, paddr_count, offset)) {
		up(&current_pcb->lock);
		if (paddrs)
			kfree(paddrs);
		mmap_list_dealloc(mapping, &current_pcb->user_mappings, NULL);
		return (void*)-ENOMEM;
	}
	if (paddrs)
		kfree(paddrs);
	
	// Include the new memory user mappings
	mmap_list_link(mapping, &current_pcb->user_mappings);
	up(&current_pcb->lock);
	
	return addr;
}

// Unmap a region of memory
int munmap(void* addr, uint32_t length) {
	LOG_DEBUG_INFO_STR("(0x%x, 0x%x)", addr, length);
	
	// Sync contents
	msync(addr, length, 0);
	// Round length up to the nearest 4kb
	if ((length % FOUR_KB_SIZE) != 0)
		length += FOUR_KB_SIZE - (length % FOUR_KB_SIZE);
	down(&current_pcb->lock);
	mmap_list_region_remove(&current_pcb->user_mappings, (uint32_t)addr, (uint32_t)addr + length, current_pcb);
	up(&current_pcb->lock);
	
	return 0;
}

// Flush memory contents
int msync(void* addr, uint32_t length, int flags) {
	LOG_DEBUG_INFO_STR("(0x%x, 0x%x, 0x%x)", addr, length, flags);
	
	uint32_t start = (uint32_t)addr;
	uint32_t end = start + length;
	down(&current_pcb->lock);
	if (!mmap_list_region_exists(current_pcb->user_mappings, start, end)) {
		up(&current_pcb->lock);
		return -ENOMEM;
	}
	mmap_list_sync(current_pcb->user_mappings, start, end, (flags & MS_INVALIDATE) != 0);
	up(&current_pcb->lock);
	
	return 0;
}
