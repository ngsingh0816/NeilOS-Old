//
//  syssignal.c
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "syssignal.h"
#include <program/task.h>
#include <program/signal.h>
#include <common/log.h>
#include <syscalls/interrupt.h>

// Send a signal to a process
uint32_t kill(uint32_t pid, uint32_t signum) {
	LOG_DEBUG_INFO_STR("(%d, %d)", pid, signum);

	if (signum >= NUMBER_OF_SIGNALS || signum == 0)
		return -EINVAL;
	
	pcb_t* pcb = pcb_from_pid(pid);
	if (!pcb)
		return -ENOMEM;
	
	signal_send(pcb, signum);
	return 0;
}

// Set the signal handler for a certain signal
uint32_t sigaction(uint32_t signum, sigaction_t* act, sigaction_t* oldact) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x, 0x%x)", signum, act, oldact);

	if (signum >= NUMBER_OF_SIGNALS || signum == 0)
		return -EINVAL;
	
	if (oldact)
		*oldact = current_pcb->signal_handlers[signum];
	if (act) {
		if (act->flags & SA_SIGINFO) {
#if DEBUG
			blue_screen("SIGINFO requested but not supported.");
#endif
		}
		signal_set_handler(current_pcb, signum, *act);
	}
	return 0;
}

// Mask signals
uint32_t sigsetmask(uint32_t signum, bool masked) {
	LOG_DEBUG_INFO_STR("(%d, %d)", signum, masked);

	signal_set_masked(current_pcb, signum, masked);
	return 0;
}

// Get signal mask
uint32_t siggetmask(uint32_t signum) {
	LOG_DEBUG_INFO_STR("(%d)", signum);

	return signal_is_masked(current_pcb, signum);
}

#define SIG_SETMASK 0	/* set mask with sigprocmask() */
#define SIG_BLOCK 1	/* set of signals to block */
#define SIG_UNBLOCK 2	/* set of signals to, well, unblock */

// Set signal masks
uint32_t sigprocmask(int how, const sigset_t* set, sigset_t* oldset) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x, 0x%x)", how, set ? *set : NULL, oldset ? *oldset : NULL);
	
	if (how < SIG_SETMASK || how > SIG_UNBLOCK)
		return -EINVAL;
	
	if (oldset)
		*oldset = current_pcb->signal_mask >> 1;
	if (set) {
		if (how == SIG_BLOCK)
			signal_block(current_pcb, (*set) << 1);
		else if (how == SIG_UNBLOCK)
			signal_unblock(current_pcb, (*set) << 1);
		else
			signal_set_mask(current_pcb, (*set) << 1);
	}

	return 0;
}

// Suspend execution until signal
uint32_t sigsuspend(const sigset_t* mask) {
	LOG_DEBUG_INFO_STR("(%d)", mask ? *mask : NULL);
	
	if (!mask)
		return -EFAULT;
	
	signal_wait(current_pcb, mask);
	return -EINTR;
}

// Set up alarm
uint32_t alarm(uint32_t seconds) {
	LOG_DEBUG_INFO_STR("(%d)", seconds);
	
	uint32_t prev = current_pcb->alarm.val;
	uint32_t time = get_current_time().val;
	
	if (seconds == 0)
		current_pcb->alarm.val = 0;
	else
		current_pcb->alarm.val = time + seconds;
	
	return (prev == 0 ? 0 : (prev - time));
}
