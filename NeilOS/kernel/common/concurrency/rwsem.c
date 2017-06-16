//
//  rwsem.c
//  NeilOS
//
//  Created by Neil Singh on 12/24/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#include "rwsem.h"

// Initialize a reader/writer semaphore
void init_rwsem(rw_semaphore_t* sema) {
	*sema = RW_SEMAPHORE_UNLOCKED;
}

// Lock a reader (sleeps until it can be locked)
void down_read(rw_semaphore_t* sema) {
	down(&sema->lock);
	sema->readers++;
	up(&sema->lock);
}

// Try to lock a reader
bool down_read_trylock(rw_semaphore_t* sema) {
	if (!down_trylock(&sema->lock))
		return false;
	
	sema->readers++;
	up(&sema->lock);
	
	return true;
}

// Release a reader
void up_read(rw_semaphore_t* sema) {
	down(&sema->lock);
	if (sema->readers != 0)
		sema->readers--;
	up(&sema->lock);
}

// Lock a write (sleeps until it can be locked)
void down_write(rw_semaphore_t* sema) {
	for (;;) {
		down(&sema->lock);
		if (sema->readers == 0)
			break;
		up(&sema->lock);
	}
}

// Try to lock a writer
bool down_write_trylock(rw_semaphore_t* sema) {
	if (!down_trylock(&sema->lock))
		return false;
	
	if (sema->readers == 0)
		return true;
	
	up(&sema->lock);
	return false;
}

// Release a writer
void up_write(rw_semaphore_t* sema) {
	up(&sema->lock);
}

// Converts a writer long into a reader lock
void downgrade_write(rw_semaphore_t* sema) {
	sema->readers++;
	up(&sema->lock);
}
