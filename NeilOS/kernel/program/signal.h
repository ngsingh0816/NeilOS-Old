//
//  signal.h
//  NeilOS
//
//  Created by Neil Singh on 6/10/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef SIGNAL_H
#define SIGNAL_H

#include <common/types.h>

struct pcb;

#define NUMBER_OF_SIGNALS	21

// Signal numbers
#define SIGHUP		1		// Hangup
#define SIGINT		2		// Terminal interrupt
#define SIGQUIT		3		// Terminal quit
#define SIGILL		4		// Illegal instruction
#define SIGTRAP		5		// Trace trap
#define SIGIOT		6		// IOT trap
#define SIGBUS		7		// Bus error
#define SIGFPE		8		// Floating point exception
#define SIGKILL		9		// Kill
#define SIGUSR1		10		// User defined 1
#define SIGSEGV		11		// Segfault
#define SIGUSR2		12		// User defined 2
#define SIGPIPE		13		// Broken pipe
#define SIGALARM	14		// Alarm
#define	SIGTERM		15		// Terminate
#define SIGSTKFLT	16		// Stack fault
#define SIGCHILD	17		// Chip process stopped
#define SIGCONT		18		// Continue executing
#define SIGSTOP		19		// Stop executing
#define SIGTSTP		20		// Terminal stop signal

#define SA_NOCLDSTOP 0x1   /* Do not generate SIGCHLD when children stop */
#define SA_SIGINFO   0x2   /* Invoke the signal catching function with */
/*   three arguments instead of one. */
#define SA_ONSTACK   0x4   /* Signal delivery will be on a separate stack. */

typedef unsigned long sigset_t;
typedef void (*sighandler_t)();

typedef struct {
	sighandler_t handler;
	sigset_t mask;
	unsigned int flags;
} sigaction_t;

// Pending signals
void signal_set_pending(struct pcb* pcb, uint32_t signum, bool pending);
bool signal_is_pending(struct pcb* pcb, uint32_t signum);
// Any signal pending?
bool signal_pending(struct pcb* pcb);

// Signal handlers
void signal_set_handler(struct pcb* pcb, uint32_t signum, sigaction_t handler);
sigaction_t signal_get_handler(struct pcb* pcb, uint32_t signum);

// Mask (block) signals
void signal_set_masked(struct pcb* pcb, uint32_t signum, bool masked);
bool signal_is_masked(struct pcb* pcb, uint32_t signum);
void signal_set_mask(struct pcb* pcb, sigset_t mask);
void signal_block(struct pcb* pcb, sigset_t mask);
void signal_unblock(struct pcb* pcb, sigset_t mask);

// Wait for a signal
void signal_wait(struct pcb* pcb, const sigset_t* mask);

// Send a signal
void signal_send(struct pcb* pcb, uint32_t signum);

// Handle signals
void signal_handle(struct pcb* pcb);

#endif /* SIGNAL_H */
