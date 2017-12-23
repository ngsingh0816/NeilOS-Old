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

struct pcb;

#define MUTEX_UNLOCKED			(semaphore_t){ .val = 1, .lock = { 0 }, .owner = NULL }
#define MUTEX_LOCKED			(semaphore_t){ .val = 0, .lock = { 0 }, .owner = NULL }
#define SEMAPHORE_UNLOCKED(x)	(semaphore_t){ .val = x, .lock = { 0 }, .owner = NULL }

// Semaphores are locks with resource counts that will sleep instead of spinning when locking
// if no resources are available
typedef struct {
	uint32_t val;
	spinlock_t lock;
	// TODO: make this work for semaphore's so its a list of owners
	// TODO: have mutex's be accounted for in scheduling (so you don't schedule in something
	// that's waiting for a mutex thats unavailable - also if a process releases a mutex that a higher
	// priority process is waiting for, then switch to that higher process - also should probably
	// implement priority inversion.
	struct pcb* owner;	// Most recent owner (really for mutex's)
} semaphore_t;

typedef semaphore_t mutex_t;

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
