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

// Set the program break to a specific address
uint32_t brk(uint32_t addr) {
	LOG_DEBUG_INFO_STR("(0x%x)", addr);

	if (addr < USER_ADDRESS)
		return -1;
	
	
	if (addr > current_pcb->brk) {
		// Allocate new pages
		uint32_t start = current_pcb->brk - (current_pcb->brk % FOUR_MB_SIZE) + FOUR_MB_SIZE;
		for (; start < addr; start += FOUR_MB_SIZE) {
			page_list_t* t = page_list_get(&current_pcb->page_list, start, MEMORY_WRITE, true);
			if (!t) {
				brk(current_pcb->brk);
				return -1;
			}
			
			// Map this new page into memory
			page_list_map(t);
		}
	} else if (addr < current_pcb->brk) {
		uint32_t start = addr - (addr % FOUR_MB_SIZE) + FOUR_MB_SIZE;
		for (; start < current_pcb->brk; start += FOUR_MB_SIZE)
			page_list_remove(&current_pcb->page_list, start);
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
