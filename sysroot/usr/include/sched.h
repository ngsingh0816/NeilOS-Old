
#ifndef	SCHED_H
#define	SCHED_H

#include <sys/types.h>

// Threads
unsigned int thread_create(void (*func)(), void* user_stack);
pid_t gettid();
unsigned int thread_wait(pid_t tid);
unsigned int thread_exit();

#endif
