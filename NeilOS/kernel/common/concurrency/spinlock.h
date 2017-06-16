//
//  spinlock.h
//  NeilOS
//
//  Created by Neil Singh on 12/24/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <common/types.h>
#include <common/lib.h>

#define SPIN_LOCK_UNLOCKED		(spinlock_t){ .lock = 0 }

// Spinlocks should be used for short critical sections
typedef struct {
	bool lock;
} spinlock_t;


// Initialize a spinlock
void spin_lock_init(spinlock_t* lock);

// Lock a spinlock (spin until we can lock it)
void spin_lock(spinlock_t* lock);
// Lock a spinlock and disable irq's
#define spin_lock_irq(lock)					{ cli(); spin_lock(lock) }
// Lock a spinlock, disable irq's and save the flags
#define spin_lock_irqsave(lock, flags)		{ cli_and_save(flags); spin_lock(lock) }

// Unlock a spinlock
void spin_unlock(spinlock_t* lock);
// Unlock a spinlock and enable irq's
#define spin_unlock_irq(lock)				{ spin_unlock(lock); sti(); }
// Unlock a spinlock, and restore irq's
#define spin_unlock_irqrestore(lock, flags)	{ spin_unlock(lock); restore_flags(flags); }

// Try to lock a spin lock but don't block
bool spin_trylock(spinlock_t* lock);


#endif
