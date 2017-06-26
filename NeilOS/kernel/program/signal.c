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
	signal_terminate, // SIGBUS
	signal_terminate, // SIGFPE (implemented)
	signal_terminate, // SIGKILL (implemented)
	signal_terminate, // SIGUSR1 (implemented)
	signal_terminate, // SIGSEGV (implemented)
	signal_terminate, // SIGUSR2 (implemented)
	signal_terminate, // SIGPIPE
	signal_terminate, // SIGALARM
	signal_terminate, // SIGTERM
	signal_terminate, // SIGSTKFLT (implemented)
	signal_ignore,	  // SIGCHILD (implemented - TODO: only signals on termination for right now)
	signal_continue,  // SIGCONT
	signal_suspend,   // SIGSTOP
	signal_suspend,   // SIGTSTP
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
	return pcb->signal_pending != 0 && pcb->signal_pending != pcb->signal_mask;
}

// Signal handlers
void signal_set_handler(pcb_t* pcb, uint32_t signum, sighandler_t handler) {
	// These signals cannot be caught or ignored
	if (signum == SIGKILL || signum == SIGSTOP)
		return;
	
	pcb->signal_handlers[signum] = handler;
}

sighandler_t signal_get_handler(pcb_t* pcb, uint32_t signum) {
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
		pcb->signal_mask &= (1 << signum);
}

bool signal_is_masked(pcb_t* pcb, uint32_t signum) {
	return (pcb->signal_mask & (1 << signum)) != 0;
}

// Send a signal
void signal_send(pcb_t* pcb, uint32_t signum) {
	signal_set_pending(pcb, signum, true);
}

// Handle signals (returns true if signal is used)
void signal_handle(pcb_t* pcb) {
	if (!signal_pending(pcb))
		return;
	
	// Check which one
	int signum;
	for (signum = 0; signum < NUMBER_OF_SIGNALS; signum++) {
		if (signal_is_pending(pcb, signum) && !signal_is_masked(pcb, signum))
			break;
	}
	
	// Mark this signal as no longer pending
	signal_set_pending(pcb, signum, false);
	
	// Get the executable address
	uint32_t exec_addr = (uint32_t)pcb->signal_handlers[signum];
	if (exec_addr == (uint32_t)NULL) {
		signal_defaults[signum](pcb);
		return;
	}
	
	// Get and copy this context over with a modified return address
	context_state_t context = *((context_state_t*)pcb->saved_esp);
	context.esp -= sizeof(context_state_t) + sizeof(uint32_t) * 2;
	pcb->saved_esp -= sizeof(context_state_t) + sizeof(uint32_t) * 2;
	memcpy((void*)pcb->saved_esp, &context, sizeof(context_state_t));
	memcpy((void*)(pcb->saved_esp + sizeof(context_state_t)), &exec_addr, sizeof(uint32_t));
	uint32_t ret = (uint32_t)sigreturn;
	memcpy((void*)(pcb->saved_esp + sizeof(context_state_t) + sizeof(uint32_t)), &ret, sizeof(uint32_t));
	
	/*
	 * This works because we context switch into the signal handler, and then when it is done,
	 * it will call a normal "ret", which will use the return address that this function
	 * is supposed to use, which leads us to performing another context switch into the correct
	 * original return place (kind of like ROP programming)
	 */
}
