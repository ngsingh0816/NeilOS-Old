//
//  sysmem.c
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "sysmem.h"
#include <program/task.h>

// Set the program break to a specific address
void* brk(uint32_t addr) {
	if (addr < USER_ADDRESS + FOUR_MB_SIZE)
		return NULL;
	
	addr -= USER_ADDRESS;
	pcb_t* pcb = get_current_pcb();
	uint32_t needed_pages = addr / FOUR_MB_SIZE + 1;
	
	// Get number of current pages
	uint32_t num_pages = 0;
	memory_list_t* t = pcb->memory_map;
	memory_list_t* prev = NULL;
	while (t) {
		num_pages++;
		prev = t;
		t = t->next;
	}
	
	if (needed_pages > num_pages) {
		bool flags = set_multitasking_enabled(false);
		// Allocate new pages
		t = prev;
		while (needed_pages > num_pages) {
			t->next = kmalloc(sizeof(memory_list_t));
			if (!t->next) {
				brk(pcb->brk);
				return NULL;
			}
			
			if (!add_memory_block(t->next, t, t->vaddr + FOUR_MB_SIZE)) {
				kfree(t->next);
				t->next = NULL;
				brk(pcb->brk);
				return NULL;
			}
			
			// Map this new page into memory
			vm_map_page(t->next->vaddr, t->next->paddr, USER_PAGE_DIRECTORY_ENTRY);
			
			t = t->next;
			num_pages++;
		}
		set_multitasking_enabled(flags);
	} else if (needed_pages < num_pages) {
		bool flags = set_multitasking_enabled(false);
		// Dealloc uneccessary pages
		t = prev;
		while (needed_pages < num_pages) {
			prev = t->prev;
			prev->next = NULL;
			page_free((void*)(USER_ADDRESS + FOUR_MB_SIZE * (num_pages - 1)));
			kfree(t);
			
			t = prev;
			num_pages--;
		}
		
		set_multitasking_enabled(flags);
	}
	
	// Return the new break
	pcb->brk = addr + USER_ADDRESS;
	return (void*)pcb->brk;
}

// Offset the current program break by a specific ammount
void* sbrk(int32_t offset) {
	return brk(get_current_pcb()->brk + offset);
}
