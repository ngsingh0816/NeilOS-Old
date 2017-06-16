//
//  spinlock.c
//  NeilOS
//
//  Created by Neil Singh on 12/24/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#include "spinlock.h"

// Initialize a spinlock
void spin_lock_init(spinlock_t* lock) {
	*lock = SPIN_LOCK_UNLOCKED;
}

// Lock a spinlock (spin until we can lock it)
void spin_lock(spinlock_t* lock) {
	while (!spin_trylock(lock)) {}
}

// Unlock a spinlock
void spin_unlock(spinlock_t* lock) {
	lock->lock = 0;
}

