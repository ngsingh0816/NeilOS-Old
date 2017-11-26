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

#define NUMBER_OF_SIGNALS	27

// Signal numbers
#define	SIGHUP	1	/* hangup */
#define	SIGINT	2	/* interrupt */
#define	SIGQUIT	3	/* quit */
#define	SIGILL	4	/* illegal instruction (not reset when caught) */
#define	SIGTRAP	5	/* trace trap (not reset when caught) */
#define	SIGIOT	6	/* IOT instruction */
#define	SIGABRT 6	/* used by abort, replace SIGIOT in the future */
#define	SIGEMT	7	/* EMT instruction */
#define	SIGFPE	8	/* floating point exception */
#define	SIGKILL	9	/* kill (cannot be caught or ignored) */
#define	SIGBUS	10	/* bus error */
#define	SIGSEGV	11	/* segmentation violation */
#define	SIGSYS	12	/* bad argument to system call */
#define	SIGPIPE	13	/* write on a pipe with no one to read it */
#define	SIGALRM	14	/* alarm clock */
#define	SIGTERM	15	/* software termination signal from kill */
#define	SIGURG	16	/* urgent condition on IO channel */
#define	SIGSTOP	17	/* sendable stop signal not from tty */
#define	SIGTSTP	18	/* stop signal from tty */
#define	SIGCONT	19	/* continue a stopped process */
#define	SIGCHLD	20	/* to parent on child stop or exit */
#define	SIGTTIN	21	/* to readers pgrp upon background tty read */
#define	SIGTTOU	22	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#define	SIGIO	23	/* input/output possible signal */
#define	SIGWINCH 24	/* window changed */
#define	SIGUSR1 25	/* user defined signal 1 */
#define	SIGUSR2 26	/* user defined signal 2 */

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
