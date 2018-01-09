//
//  pthread.h
//  pthread
//
//  Created by Neil Singh on 12/28/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef pthread_h
#define pthread_h

#include <unistd.h>
#include <sched.h>
#include <time.h>

#define PTHREAD_THREADS_MAX			128
#define PTHREAD_CANCELLED 			((void*)-1)

enum {
	PTHREAD_CANCEL_ENABLE,
#define PTHREAD_CANCEL_ENABLE	PTHREAD_CANCEL_ENABLE
	PTHREAD_CANCEL_DISABLE
#define PTHREAD_CANCEL_DISABLE	PTHREAD_CANCEL_DISABLE
};

enum {
	PTHREAD_CANCEL_DEFERRED,
#define PTHREAD_CANCEL_DEFERRED	PTHREAD_CANCEL_DEFERRED
	PTHREAD_CANCEL_ASYNCHRONOUS
#define PTHREAD_CANCEL_ASYNCHRONOUS	PTHREAD_CANCEL_ASYNCHRONOUS
};

enum {
	PTHREAD_CREATE_JOINABLE,
#define PTHREAD_CREATE_JOINABLE	PTHREAD_CREATE_JOINABLE
	PTHREAD_CREATE_DETACHED
#define PTHREAD_CREATE_DETACHED	PTHREAD_CREATE_DETACHED
};

enum {
	PTHREAD_INHERIT_SCHED,
#define PTHREAD_INHERIT_SCHED	PTHREAD_INHERIT_SCHED
	PTHREAD_EXPLICIT_SCHED
#define PTHREAD_EXPLICIT_SCHED	PTHREAD_EXPLICIT_SCHED
};

enum {
	PTHREAD_SCOPE_SYSTEM,
#define PTHREAD_SCOPE_SYSTEM	PTHREAD_SCOPE_SYSTEM
	PTHREAD_SCOPE_PROCESS
#define PTHREAD_SCOPE_PROCESS	PTHREAD_SCOPE_PROCESS
};

enum {
	PTHREAD_PROCESS_PRIVATE,
#define PTHREAD_PROCESS_PRIVATE	PTHREAD_PROCESS_PRIVATE
	PTHREAD_PROCESS_SHARED
#define PTHREAD_PROCESS_SHARED	PTHREAD_PROCESS_SHARED
};

enum {
	PTHREAD_MUTEX_NORMAL,
	PTHREAD_MUTEX_RECURSIVE,
	PTHREAD_MUTEX_ERRORCHECK,
#define PTHREAD_MUTEX_DEFAULT	PTHREAD_MUTEX_NORMAL
};

enum {
	PTHREAD_PRIO_NONE,
	PTHREAD_PRIO_INHERIT,
	PTHREAD_PRIO_PROTECT
};

// Mutex
typedef struct {
	int prioceiling;
	int protocol;
	int pshared;
	int type;
} pthread_mutexattr_t;

typedef struct {
	pthread_mutexattr_t attr;
	int lock;
} pthread_mutex_t;

#define PTHREAD_MUTEX_INITIALIZER	((pthread_mutex_t){ { 0, 0, 0, 0 }, 0 })

int   pthread_mutexattr_init(pthread_mutexattr_t *);
int   pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *, int *);
int   pthread_mutexattr_getprotocol(const pthread_mutexattr_t *, int *);
int   pthread_mutexattr_getpshared(const pthread_mutexattr_t *, int *);
int   pthread_mutexattr_gettype(const pthread_mutexattr_t *, int *);
int   pthread_mutexattr_setprioceiling(pthread_mutexattr_t *, int);
int   pthread_mutexattr_setprotocol(pthread_mutexattr_t *, int);
int   pthread_mutexattr_setpshared(pthread_mutexattr_t *, int);
int   pthread_mutexattr_settype(pthread_mutexattr_t *, int);
int   pthread_mutexattr_destroy(pthread_mutexattr_t *);

int   pthread_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *);
int   pthread_mutex_lock(pthread_mutex_t *);
int   pthread_mutex_trylock(pthread_mutex_t *);
int   pthread_mutex_unlock(pthread_mutex_t *);
int   pthread_mutex_getprioceiling(const pthread_mutex_t *, int *);
int   pthread_mutex_setprioceiling(pthread_mutex_t *, int, int *);
int   pthread_mutex_destroy(pthread_mutex_t *);

// Read Write Lock

typedef struct {
	int pshared;
} pthread_rwlockattr_t;

typedef struct {
	pthread_rwlockattr_t attr;
	int lock;
	int readers;
} pthread_rwlock_t;

#define PTHREAD_RWLOCK_INITIALIZER	((pthread_rwlock_t){ { 0 }, 0, 0 })

int   pthread_rwlockattr_init(pthread_rwlockattr_t *);
int   pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *, int *);
int   pthread_rwlockattr_setpshared(pthread_rwlockattr_t *, int);
int   pthread_rwlockattr_destroy(pthread_rwlockattr_t *);

int   pthread_rwlock_init(pthread_rwlock_t *, const pthread_rwlockattr_t *);
int   pthread_rwlock_rdlock(pthread_rwlock_t *);
int   pthread_rwlock_wrlock(pthread_rwlock_t *);
int   pthread_rwlock_tryrdlock(pthread_rwlock_t *);
int   pthread_rwlock_trywrlock(pthread_rwlock_t *);
int   pthread_rwlock_unlock(pthread_rwlock_t *);
int   pthread_rwlock_destroy(pthread_rwlock_t *);

// Condition variables

typedef struct {
	int pshared;
} pthread_condattr_t;

typedef struct {
	pthread_condattr_t attr;
	pthread_mutex_t lock;
	volatile char waiting[PTHREAD_THREADS_MAX];
} pthread_cond_t;

#define PTHREAD_COND_INITIALIZER	((pthread_cond_t){ { 0 }, PTHREAD_MUTEX_INITIALIZER, { 0 } })

int   pthread_condattr_init(pthread_condattr_t *);
int   pthread_condattr_getpshared(const pthread_condattr_t *, int *);
int   pthread_condattr_setpshared(pthread_condattr_t *, int);
int   pthread_condattr_destroy(pthread_condattr_t *);

int   pthread_cond_init(pthread_cond_t *, const pthread_condattr_t *);
int   pthread_cond_broadcast(pthread_cond_t *);
int   pthread_cond_signal(pthread_cond_t *);
int   pthread_cond_timedwait(pthread_cond_t *, pthread_mutex_t *, const struct timespec *);
int   pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
int   pthread_cond_destroy(pthread_cond_t *);

// Pthread

typedef struct {
	int detach_state;
	size_t gaurd_size;
	int inherit_sched;
	struct sched_param sp;
	int sched_policy;
	int scope;
	void* stack_addr;
	size_t stack_size;
	int is_stack_owner;
} pthread_attr_t;

int   pthread_attr_init(pthread_attr_t *);
int   pthread_attr_getdetachstate(const pthread_attr_t *, int *);
int   pthread_attr_getguardsize(const pthread_attr_t *, size_t *);
int   pthread_attr_getinheritsched(const pthread_attr_t *, int *);
int   pthread_attr_getschedparam(const pthread_attr_t *, struct sched_param *);
int   pthread_attr_getschedpolicy(const pthread_attr_t *, int *);
int   pthread_attr_getscope(const pthread_attr_t *, int *);
int   pthread_attr_getstackaddr(const pthread_attr_t *, void **);
int   pthread_attr_getstacksize(const pthread_attr_t *, size_t *);
int   pthread_attr_setdetachstate(pthread_attr_t *, int);
int   pthread_attr_setguardsize(pthread_attr_t *, size_t);
int   pthread_attr_setinheritsched(pthread_attr_t *, int);
int   pthread_attr_setschedparam(pthread_attr_t *, const struct sched_param *);
int   pthread_attr_setschedpolicy(pthread_attr_t *, int);
int   pthread_attr_setscope(pthread_attr_t *, int);
int   pthread_attr_setstackaddr(pthread_attr_t *, void *);
int   pthread_attr_setstacksize(pthread_attr_t *, size_t);
int   pthread_attr_destroy(pthread_attr_t *);

struct pthread  {
	pthread_attr_t attr;
	pthread_mutex_t lock;
	pid_t tid;
	int cancel_state;
	int cancel_type;
	int cancelled;
	int detached;
	void* (*func)(void*);
	void* arg;
	void* retval;
};

typedef struct pthread* pthread_t;

typedef struct {
	pthread_mutex_t lock;
	int occurred;
} pthread_once_t;

#define PTHREAD_ONCE_INIT ((pthread_once_t){ PTHREAD_MUTEX_INITIALIZER, 0 })

int   pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
pthread_t pthread_self(void);
int   pthread_join(pthread_t, void **);
int   pthread_detach(pthread_t);
int   pthread_equal(pthread_t, pthread_t);
int   pthread_once(pthread_once_t *, void (*)(void));
void  pthread_exit(void *);
int   pthread_cancel(pthread_t);

void  pthread_testcancel(void);

// TODO: cleanup functions
/*void  pthread_cleanup_push(void (*)(void*), void *);
void  pthread_cleanup_pop(int);*/

int   pthread_getconcurrency(void);
int   pthread_getschedparam(pthread_t, int *, struct sched_param *);
int   pthread_setcancelstate(int, int *);
int   pthread_setcanceltype(int, int *);
int   pthread_setconcurrency(int);
int   pthread_setschedparam(pthread_t, int , const struct sched_param *);

// TODO: key functions
/*int   pthread_setspecific(pthread_key_t, const void *);
void *pthread_getspecific(pthread_key_t);
int   pthread_key_create(pthread_key_t *, void (*)(void *));
int   pthread_key_delete(pthread_key_t);*/


#endif /* pthread_h */
