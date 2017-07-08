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

// Set the program break to a specific address
uint32_t brk(uint32_t addr) {
	LOG_DEBUG_INFO_STR("(0x%x)", addr);

	if (addr < USER_ADDRESS)
		return -1;
	
	
	if (addr > current_pcb->brk) {
		bool flags = set_multitasking_enabled(false);
		// Allocate new pages
		uint32_t start = current_pcb->brk - (current_pcb->brk % FOUR_MB_SIZE) + FOUR_MB_SIZE;
		for (; start < addr; start += FOUR_MB_SIZE) {
			memory_list_t* t = current_pcb->memory_map;
			memory_list_t* prev = NULL;
			bool found = false;
			while (t) {
				if (t->vaddr == start) {
					found = true;
					break;
				}
				prev = t;
				t = t->next;
			}
			if (found)
				continue;
			prev->next = kmalloc(sizeof(memory_list_t));
			if (!prev->next) {
				brk(current_pcb->brk);
				set_multitasking_enabled(flags);
				return -1;
			}
			
			if (!add_memory_block(prev->next, prev, start)) {
				kfree(prev->next);
				prev->next = NULL;
				brk(current_pcb->brk);
				set_multitasking_enabled(flags);
				return -1;
			}
			
			// Map this new page into memory
			vm_map_page(prev->next->vaddr, prev->next->paddr, USER_PAGE_DIRECTORY_ENTRY);
		}
		set_multitasking_enabled(flags);
	} else if (addr < current_pcb->brk) {
		uint32_t start = addr - (addr % FOUR_MB_SIZE) + FOUR_MB_SIZE;
		bool flags = set_multitasking_enabled(false);
		for (; start < current_pcb->brk; start += FOUR_MB_SIZE) {
			memory_list_t* t = current_pcb->memory_map;
			while (t) {
				if (t->vaddr == start) {
					page_free((void*)t->vaddr);
					if (t->prev)
						t->prev->next = t->next;
					else
						current_pcb->memory_map = t->next;
					if (t->next)
						t->next->prev = t->prev ? t->prev : current_pcb->memory_map;
					kfree(t);
					break;
				}
				t = t->next;
			}
		}
		
		set_multitasking_enabled(flags);
	}
	
	// Return the new break
	current_pcb->brk = addr;
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
