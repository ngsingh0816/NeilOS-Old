//
//  signal.c
//  NeilOS
//
//  Created by Neil Singh on 6/10/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "signal.h"
#include <program/task.h>

extern void sigreturn();
extern void sigjmp(uint32_t exex, void* ret, uint32_t signum);

// Default signal actions
// Do nothing
void signal_ignore(pcb_t* pcb) {
}

// Terminate the task
void signal_terminate(pcb_t* pcb) {
	if (pcb == current_pcb)
		terminate_task(-1);
	else
		pcb->should_terminate = true;
}

// Continue the task
void signal_continue(pcb_t* pcb) {
	
}

// Suspend the task
void signal_suspend(pcb_t* pcb) {
	
}

// Signal default list
void (*signal_defaults[NUMBER_OF_SIGNALS])(pcb_t*) = {
	signal_terminate, // SIGHUP
	signal_terminate, // SIGINT (implemented)
	signal_terminate, // SIGQUIT
	signal_terminate, // SIGILL (implemented)
	signal_terminate, // SIGTRAP (implemented)
	signal_terminate, // SIGIOT (implemented)
	signal_terminate, // SIGEMT
	signal_terminate, // SIGFPE (implemented)
	signal_terminate, // SIGKILL (implemented)
	signal_terminate, // SIGBUS (implemented)
	signal_terminate, // SIGSEGV (implemented)
	signal_terminate, // SIGSYS
	signal_terminate, // SIGPIPE
	signal_terminate, // SIGALARM
	signal_terminate, // SIGTERM
	signal_terminate, // SIGURG
	signal_suspend,   // SIGSTOP
	signal_suspend,   // SIGTSTP
	signal_continue,  // SIGCONT
	signal_ignore,	  // SIGCHILD (implemented - TODO: only signals on termination for right now)
	signal_terminate, // SIGTTIN
	signal_terminate, // SIGTTOU
	signal_terminate, // SIGIO
	signal_terminate, // SIGWINCH
	signal_terminate, // SIGUSR1
	signal_terminate, // SIGUSR2
};

// Pending signals
void signal_set_pending(pcb_t* pcb, uint32_t signum, bool pending) {
	if (pending)
		pcb->signal_pending |= (1 << signum);
	else
		pcb->signal_pending &= ~(1 << signum);
}

bool signal_is_pending(pcb_t* pcb, uint32_t signum) {
	return (pcb->signal_pending & (1 << signum)) != 0;
}

bool signal_pending(pcb_t* pcb) {
	// Check if there are any unblocked signals
	bool has_alarm = (pcb->alarm.val != 0 && pcb->alarm.val <= get_current_time().val);
	return (pcb->signal_pending != 0 && pcb->signal_pending != pcb->signal_mask) || has_alarm;
}

bool signal_occurring(struct pcb* pcb) {
	return (pcb && (signal_pending(pcb) || pcb->signal_occurred));
}

// Signal handlers
void signal_set_handler(pcb_t* pcb, uint32_t signum, sigaction_t handler) {
	// These signals cannot be caught or ignored
	if (signum == SIGKILL || signum == SIGSTOP)
		return;
	
	pcb->signal_handlers[signum] = handler;
}

sigaction_t signal_get_handler(pcb_t* pcb, uint32_t signum) {
	return pcb->signal_handlers[signum];
}

// Mask (block) signals
void signal_set_masked(pcb_t* pcb, uint32_t signum, bool masked) {
	// These signals cannot be caught or ignored
	if (signum == SIGKILL || signum == SIGSTOP)
		return;
	
	if (masked)
		pcb->signal_mask |= (1 << signum);
	else
		pcb->signal_mask &= ~(1 << signum);
}

bool signal_is_masked(pcb_t* pcb, uint32_t signum) {
	return (pcb->signal_mask & (1 << signum)) != 0;
}

void signal_set_mask(struct pcb* pcb, sigset_t mask) {
	pcb->signal_mask = mask;
}

void signal_block(struct pcb* pcb, sigset_t mask) {
	pcb->signal_mask |= mask;
}

void signal_unblock(struct pcb* pcb, sigset_t mask) {
	pcb->signal_mask &= ~mask;
}

// Wait for a signal
void signal_wait(struct pcb* pcb, const sigset_t* mask) {
	down(&current_pcb->lock);

	// Save the sigmask
	pcb->signal_save_mask[0] = pcb->signal_mask;
	pcb->signal_mask = *mask;
	pcb->signal_waiting = true;
	
	// Wait for this to be changed
	while (pcb->signal_waiting) {
		schedule();
		
		up(&current_pcb->lock);
		down(&current_pcb->lock);
	}
	
	up(&current_pcb->lock);
}

// Send a signal
void signal_send(pcb_t* pcb, uint32_t signum) {
	signal_set_pending(pcb, signum, true);
}

// Handle signals (returns true if signal is used)
void signal_handle(pcb_t* pcb) {
	// Check for alarms
	if (pcb->alarm.val != 0 && pcb->alarm.val <= get_current_time().val) {
		pcb->alarm.val = 0;
		signal_send(pcb, SIGALRM);
	}
	
	if (!signal_pending(pcb))
		return;
	
	// Check which one
	int signum;
	for (signum = 1; signum < NUMBER_OF_SIGNALS; signum++) {
		if (signal_is_pending(pcb, signum) && !signal_is_masked(pcb, signum))
			break;
	}
	
	// Mark this signal as no longer pending and mark as a signal occurring
	signal_set_pending(pcb, signum, false);
	pcb->signal_occurred = true;
	
	// Get the executable address
	uint32_t exec_addr = (uint32_t)pcb->signal_handlers[signum].handler;
	if (exec_addr == (uint32_t)NULL) {
		signal_defaults[signum - 1](pcb);
		return;
	}
	
	// Save the sigmask
	pcb->signal_save_mask[signum] = pcb->signal_mask;
	pcb->signal_mask |= pcb->signal_handlers[signum].mask;
	signal_set_masked(pcb, signum, true);

	/*
	 * This works because we context switch into the signal handler, and then when it is done,
	 * it will call a normal "ret", which will use the return address that this function
	 * is supposed to use, which leads us to performing another context switch into the correct
	 * original return place (kind of like ROP programming)
	 */
	
	sigjmp(exec_addr, sigreturn, signum);
}

void sigreturn_impl(int signum) {
	// Restore the sigmask
	current_pcb->signal_mask = current_pcb->signal_save_mask[signum];
	if (current_pcb->signal_waiting) {
		current_pcb->signal_mask = current_pcb->signal_save_mask[0];
		current_pcb->signal_waiting = false;
	}
}
