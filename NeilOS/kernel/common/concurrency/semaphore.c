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
void down(semaphore_t* sema) {
	for (;;) {
		spin_lock(&sema->lock);
		if (sema->val != 0) {
			sema->val--;
			spin_unlock(&sema->lock);
			break;
		}
		spin_unlock(&sema->lock);
		
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
	spin_unlock(&sema->lock);
	return true;
}

// Relase access to a resource
void up(semaphore_t* sema) {
	spin_lock(&sema->lock);
	sema->val++;
	spin_unlock(&sema->lock);
}
