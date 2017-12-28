
#ifndef	SCHED_H
#define	SCHED_H

#include <sys/types.h>

// Threads
unsigned int thread_fork(void* user_stack);
pid_t gettid();
unsigned int thread_exit();

#endif
