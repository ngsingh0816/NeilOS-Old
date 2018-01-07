//
//  semaphore.c
//  NeilOS
//
//  Created by Neil Singh on 12/24/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#include "semaphore.h"
#include <program/task.h>

// Initialize a semaphore with a particular resource count
void sema_init(semaphore_t* sema, uint32_t val) {
	*sema = SEMAPHORE_UNLOCKED(val);
}

// Gain access to a resource
#if DEBUG
void down_debug(semaphore_t* sema, const char* filename, int line_num) {
#else
void down(semaphore_t* sema) {
#endif
	for (;;) {
		if (spin_trylock(&sema->lock)) {
			if (sema->val != 0) {
				sema->owner = current_pcb;
				sema->val--;
				
#if DEBUG
				sema->filename = filename;
				sema->line_num = line_num;
#endif
				
				spin_unlock(&sema->lock);
				break;
			}
			spin_unlock(&sema->lock);
		}
		
		// Sleep if the resource is not available
		schedule();
	}
}

// Try to gain access but don't sleep if unavailable
bool down_trylock(semaphore_t* sema) {
	if (!spin_trylock(&sema->lock))
		return false;
	
	if (sema->val == 0) {
		spin_unlock(&sema->lock);
		return false;
	}
	
	sema->val--;
	sema->owner = current_pcb;
	spin_unlock(&sema->lock);
	return true;
}

// Relase access to a resource
void up(semaphore_t* sema) {
	spin_lock(&sema->lock);
	sema->val++;
	spin_unlock(&sema->lock);
}
