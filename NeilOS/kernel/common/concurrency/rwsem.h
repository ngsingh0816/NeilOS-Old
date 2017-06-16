//
//  rwsem.h
//  NeilOS
//
//  Created by Neil Singh on 12/24/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#ifndef RWSEM_H
#define RWSEM_H

#include <common/types.h>
#include "semaphore.h"

#define RW_SEMAPHORE_UNLOCKED		(rw_semaphore_t){ .readers = 0, .lock = MUTEX_UNLOCKED }

// Reader/Writer semaphores allow infinite readers at once,
// but a writer has individual access (sleeps on unavailable lock)
typedef struct {
	uint32_t readers;
	
	semaphore_t lock;
} rw_semaphore_t;

// Initialize a reader/writer semaphore
void init_rwsem(rw_semaphore_t* sema);

// Lock a reader (sleeps until it can be locked)
void down_read(rw_semaphore_t* sema);
// Try to lock a reader
bool down_read_trylock(rw_semaphore_t* sema);
// Release a reader
void up_read(rw_semaphore_t* sema);

// Lock a write (sleeps until it can be locked)
void down_write(rw_semaphore_t* sema);
// Try to lock a writer
bool down_write_trylock(rw_semaphore_t* sema);
// Release a writer
void up_write(rw_semaphore_t* sema);
// Converts a writer long into a reader lock
void downgrade_write(rw_semaphore_t* sema);

#endif
