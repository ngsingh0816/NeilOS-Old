//
//  rwlock.c
//  NeilOS
//
//  Created by Neil Singh on 12/24/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#include "rwlock.h"
#include <common/lib.h>

// Initialize a reader/writer lock
void rwlock_init(rwlock_t* lock) {
	*lock = RW_LOCK_UNLOCKED;
}

// Lock a reader (spin until we can lock it)
void read_lock(rwlock_t* lock) {
	spin_lock(&lock->lock);
	lock->readers++;
	spin_unlock(&lock->lock);
}


// Unlock a reader
void read_unlock(rwlock_t* lock) {
	spin_lock(&lock->lock);
	if (lock->readers != 0)
		lock->readers--;
	spin_unlock(&lock->lock);
}

// Lock a writer (spin until we can lock it)
void write_lock(rwlock_t* lock) {
	for (;;) {
		spin_lock(&lock->lock);
		if (lock->readers == 0)
			break;
		spin_unlock(&lock->lock);
	}
}

// Try to lock a writer (but don't block if we can't get it)
bool write_trylock(rwlock_t* lock) {
	if (!spin_trylock(&lock->lock))
		return false;
	
	if (lock->readers != 0) {
		spin_unlock(&lock->lock);
		return false;
	}
	
	return true;
}

// Unlock a writer
void write_unlock(rwlock_t* lock) {
	spin_unlock(&lock->lock);
}
