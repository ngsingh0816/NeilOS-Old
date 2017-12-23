//
//  concurrency.S
//  NeilOS
//
//  Created by Neil Singh on 12/24/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

.globl spin_trylock

#ifdef __APPLE__
.globl _spin_trylock
_spin_trylock:
#endif

// bool spin_trylock(spinlock_t* lock)
// Try to aquire a spinlock but don't block if we cannot
// Returns 1 if lock was obtained, otherwise 0
spin_trylock:
	// ebx = lock
	pushl %ebx
	movl 8(%esp), %ebx
	mov $1, %al

	// Atomically exchange the lock
	xchg %al, (%ebx)

	// Check if the lock was set before
	test %al, %al
	jnz is_one

	// It was clear before, so we locked it
	movl $1, %eax
	popl %ebx
	ret

	// It was set before, so we did not lock it
	is_one:
		movl $0, %eax
		popl %ebx
		ret
