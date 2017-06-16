//
//  rwlock.h
//  NeilOS
//
//  Created by Neil Singh on 12/24/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#ifndef RWLOCK_H
#define RWLOCK_H

#include <common/types.h>
#include "spinlock.h"
#include <common/lib.h>

#define RW_LOCK_UNLOCKED	(rwlock_t){ .lock = SPIN_LOCK_UNLOCKED }

// Reader/Writer locks allow infinite readers at once,
// but a writer has individual access (spins on unavailable lock)
typedef struct {
	spinlock_t lock;

	// How many readers we currently have
	int readers;
} rwlock_t;

// Initialize a reader/writer lock
void rwlock_init(rwlock_t* lock);

// Lock a reader (spin until we can lock it)
void read_lock(rwlock_t* lock);
// Lock a reader and disable irq's
#define read_lock_irq(lock)						{ cli(); read_lock(lock); }
// Lock a reader, disable irq's and save the flags
#define read_lock_irqsave(lock, flags)			{ cli_and_save(flags); read_lock(lock) };

// Unlock a reader
void read_unlock(rwlock_t* lock);
// Unlock a reader and enable irq's
#define read_unlock_irq(lock)					{ read_unlock(lock); sti(); }
// Unlock a reader, and restore irq's
#define read_unlock_irqrestore(lock, flags)		{ read_unlock(lock); restore_flags(flags); }

// Lock a writer (spin until we can lock it)
void write_lock(rwlock_t* lock);
// Lock a writer and disable irq's
#define write_lock_irq(lock)					{ cli(); write_lock(lock); }
// Lock a writer, disable irq's and save the flags
#define write_lock_irqsave(lock, flags)			{ cli_and_save(flags); write_lock(lock); }
// Try to lock a writer (but don't block if we can't get it)
bool write_trylock(rwlock_t* lock);

// Unlock a writer
void write_unlock(rwlock_t* lock);
// Unlock a writer and enable irq's
#define write_unlock_irq(lock)					{ write_unlock(lock); sti(); }
// Unlock a writer, and restore irq's
#define write_unlock_irqrestore(lock, flags)	{ write_unlock(lock); restore_flags(flags); }

#endif
