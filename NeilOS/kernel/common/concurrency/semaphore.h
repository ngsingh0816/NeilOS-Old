//
//  semaphore.h
//  NeilOS
//
//  Created by Neil Singh on 12/24/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <common/types.h>
#include "spinlock.h"

#define MUTEX_UNLOCKED			(semaphore_t){ .val = 1, .lock = { 0 } }
#define SEMAPHORE_UNLOCKED(x)	(semaphore_t){ .val = x, .lock = { 0 } }

// Semaphores are locks with resource counts that will sleep instead of spinning when locking
// if no resources are available
typedef struct {
	uint32_t val;
	spinlock_t lock;
} semaphore_t;

// Initialize a semaphore with a particular resource count
void sema_init(semaphore_t* sema, uint32_t val);
// Initialize a mutex (semaphore with 1 resource)
#define mutex_init(mutex)		sema_init(mutex, 1)

// Gain access to a resource
void down(semaphore_t* sema);
// Try to gain access but don't sleep if unavailable
bool down_trylock(semaphore_t* sema);

// Relase access to a resource
void up(semaphore_t* sema);

#endif
