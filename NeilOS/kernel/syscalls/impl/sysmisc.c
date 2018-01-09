//
//  sysmisc.c
//  NeilOS
//
//  Created by Neil Singh on 6/29/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "sysmisc.h"
#include <syscalls/descriptor.h>
#include <memory/memory.h>
#include <common/lib.h>
#include <common/log.h>
#include <syscalls/interrupt.h>

/*
 *  sysconf values per IEEE Std 1003.1, 2008 Edition
 */

#define _SC_ARG_MAX                       0
#define _SC_CHILD_MAX                     1
#define _SC_CLK_TCK                       2
#define _SC_NGROUPS_MAX                   3
#define _SC_OPEN_MAX                      4
#define _SC_JOB_CONTROL                   5
#define _SC_SAVED_IDS                     6
#define _SC_VERSION                       7
#define _SC_PAGESIZE                      8
#define _SC_PAGE_SIZE                     _SC_PAGESIZE
/* These are non-POSIX values we accidentally introduced in 2000 without
 guarding them.  Keeping them unguarded for backward compatibility. */
#define _SC_NPROCESSORS_CONF              9
#define _SC_NPROCESSORS_ONLN             10
#define _SC_PHYS_PAGES                   11
#define _SC_AVPHYS_PAGES                 12
/* End of non-POSIX values. */
#define _SC_MQ_OPEN_MAX                  13
#define _SC_MQ_PRIO_MAX                  14
#define _SC_RTSIG_MAX                    15
#define _SC_SEM_NSEMS_MAX                16
#define _SC_SEM_VALUE_MAX                17
#define _SC_SIGQUEUE_MAX                 18
#define _SC_TIMER_MAX                    19
#define _SC_TZNAME_MAX                   20
#define _SC_ASYNCHRONOUS_IO              21
#define _SC_FSYNC                        22
#define _SC_MAPPED_FILES                 23
#define _SC_MEMLOCK                      24
#define _SC_MEMLOCK_RANGE                25
#define _SC_MEMORY_PROTECTION            26
#define _SC_MESSAGE_PASSING              27
#define _SC_PRIORITIZED_IO               28
#define _SC_REALTIME_SIGNALS             29
#define _SC_SEMAPHORES                   30
#define _SC_SHARED_MEMORY_OBJECTS        31
#define _SC_SYNCHRONIZED_IO              32
#define _SC_TIMERS                       33
#define _SC_AIO_LISTIO_MAX               34
#define _SC_AIO_MAX                      35
#define _SC_AIO_PRIO_DELTA_MAX           36
#define _SC_DELAYTIMER_MAX               37
#define _SC_THREAD_KEYS_MAX              38
#define _SC_THREAD_STACK_MIN             39
#define _SC_THREAD_THREADS_MAX           40
#define _SC_TTY_NAME_MAX                 41
#define _SC_THREADS                      42
#define _SC_THREAD_ATTR_STACKADDR        43
#define _SC_THREAD_ATTR_STACKSIZE        44
#define _SC_THREAD_PRIORITY_SCHEDULING   45
#define _SC_THREAD_PRIO_INHERIT          46
/* _SC_THREAD_PRIO_PROTECT was _SC_THREAD_PRIO_CEILING in early drafts */
#define _SC_THREAD_PRIO_PROTECT          47
#define _SC_THREAD_PRIO_CEILING          _SC_THREAD_PRIO_PROTECT
#define _SC_THREAD_PROCESS_SHARED        48
#define _SC_THREAD_SAFE_FUNCTIONS        49
#define _SC_GETGR_R_SIZE_MAX             50
#define _SC_GETPW_R_SIZE_MAX             51
#define _SC_LOGIN_NAME_MAX               52
#define _SC_THREAD_DESTRUCTOR_ITERATIONS 53
#define _SC_ADVISORY_INFO                54
#define _SC_ATEXIT_MAX                   55
#define _SC_BARRIERS                     56
#define _SC_BC_BASE_MAX                  57
#define _SC_BC_DIM_MAX                   58
#define _SC_BC_SCALE_MAX                 59
#define _SC_BC_STRING_MAX                60
#define _SC_CLOCK_SELECTION              61
#define _SC_COLL_WEIGHTS_MAX             62
#define _SC_CPUTIME                      63
#define _SC_EXPR_NEST_MAX                64
#define _SC_HOST_NAME_MAX                65
#define _SC_IOV_MAX                      66
#define _SC_IPV6                         67
#define _SC_LINE_MAX                     68
#define _SC_MONOTONIC_CLOCK              69
#define _SC_RAW_SOCKETS                  70
#define _SC_READER_WRITER_LOCKS          71
#define _SC_REGEXP                       72
#define _SC_RE_DUP_MAX                   73
#define _SC_SHELL                        74
#define _SC_SPAWN                        75
#define _SC_SPIN_LOCKS                   76
#define _SC_SPORADIC_SERVER              77
#define _SC_SS_REPL_MAX                  78
#define _SC_SYMLOOP_MAX                  79
#define _SC_THREAD_CPUTIME               80
#define _SC_THREAD_SPORADIC_SERVER       81
#define _SC_TIMEOUTS                     82
#define _SC_TRACE                        83
#define _SC_TRACE_EVENT_FILTER           84
#define _SC_TRACE_EVENT_NAME_MAX         85
#define _SC_TRACE_INHERIT                86
#define _SC_TRACE_LOG                    87
#define _SC_TRACE_NAME_MAX               88
#define _SC_TRACE_SYS_MAX                89
#define _SC_TRACE_USER_EVENT_MAX         90
#define _SC_TYPED_MEMORY_OBJECTS         91
#define _SC_V7_ILP32_OFF32               92
#define _SC_V6_ILP32_OFF32               _SC_V7_ILP32_OFF32
#define _SC_XBS5_ILP32_OFF32             _SC_V7_ILP32_OFF32
#define _SC_V7_ILP32_OFFBIG              93
#define _SC_V6_ILP32_OFFBIG              _SC_V7_ILP32_OFFBIG
#define _SC_XBS5_ILP32_OFFBIG            _SC_V7_ILP32_OFFBIG
#define _SC_V7_LP64_OFF64                94
#define _SC_V6_LP64_OFF64                _SC_V7_LP64_OFF64
#define _SC_XBS5_LP64_OFF64              _SC_V7_LP64_OFF64
#define _SC_V7_LPBIG_OFFBIG              95
#define _SC_V6_LPBIG_OFFBIG              _SC_V7_LPBIG_OFFBIG
#define _SC_XBS5_LPBIG_OFFBIG            _SC_V7_LPBIG_OFFBIG
#define _SC_XOPEN_CRYPT                  96
#define _SC_XOPEN_ENH_I18N               97
#define _SC_XOPEN_LEGACY                 98
#define _SC_XOPEN_REALTIME               99
#define _SC_STREAM_MAX                  100
#define _SC_PRIORITY_SCHEDULING         101
#define _SC_XOPEN_REALTIME_THREADS      102
#define _SC_XOPEN_SHM                   103
#define _SC_XOPEN_STREAMS               104
#define _SC_XOPEN_UNIX                  105
#define _SC_XOPEN_VERSION               106
#define _SC_2_CHAR_TERM                 107
#define _SC_2_C_BIND                    108
#define _SC_2_C_DEV                     109
#define _SC_2_FORT_DEV                  110
#define _SC_2_FORT_RUN                  111
#define _SC_2_LOCALEDEF                 112
#define _SC_2_PBS                       113
#define _SC_2_PBS_ACCOUNTING            114
#define _SC_2_PBS_CHECKPOINT            115
#define _SC_2_PBS_LOCATE                116
#define _SC_2_PBS_MESSAGE               117
#define _SC_2_PBS_TRACK                 118
#define _SC_2_SW_DEV                    119
#define _SC_2_UPE                       120
#define _SC_2_VERSION                   121
#define _SC_THREAD_ROBUST_PRIO_INHERIT  122
#define _SC_THREAD_ROBUST_PRIO_PROTECT  123
#define _SC_XOPEN_UUCP                  124
#define _SC_LEVEL1_ICACHE_SIZE          125
#define _SC_LEVEL1_ICACHE_ASSOC         126
#define _SC_LEVEL1_ICACHE_LINESIZE      127
#define _SC_LEVEL1_DCACHE_SIZE          128
#define _SC_LEVEL1_DCACHE_ASSOC         129
#define _SC_LEVEL1_DCACHE_LINESIZE      130
#define _SC_LEVEL2_CACHE_SIZE           131
#define _SC_LEVEL2_CACHE_ASSOC          132
#define _SC_LEVEL2_CACHE_LINESIZE       133
#define _SC_LEVEL3_CACHE_SIZE           134
#define _SC_LEVEL3_CACHE_ASSOC          135
#define _SC_LEVEL3_CACHE_LINESIZE       136
#define _SC_LEVEL4_CACHE_SIZE           137
#define _SC_LEVEL4_CACHE_ASSOC          138
#define _SC_LEVEL4_CACHE_LINESIZE       139
#define _SC_POSIX_26_VERSION            140

// Get some info about a limit
uint32_t sysconf(uint32_t name) {
	LOG_DEBUG_INFO_STR("(%d)", name);

	switch (name) {
		case _SC_ARG_MAX: return 4096;
		case _SC_CHILD_MAX: return 4096;
		case _SC_CLK_TCK: return 100;
		case _SC_NGROUPS_MAX: return 0;
		case _SC_OPEN_MAX: return NUMBER_OF_DESCRIPTORS;
		case _SC_PAGESIZE: return FOUR_MB_SIZE;
		case _SC_VERSION: return 200801;
		// TODO: actually calculate these
		case _SC_PHYS_PAGES: return (FOUR_MB_SIZE / FOUR_KB_SIZE);
		case _SC_NPROCESSORS_ONLN: return 1;
		case _SC_AVPHYS_PAGES: return 1024;
		default: {
#if DEBUG
			blue_screen("Unimplemented sysconf variable %d.", name);
#endif
			return -1;
		}
	}
}

// Get some info about a file limit
uint32_t fpathconf(uint32_t fd, uint32_t name) {
	LOG_DEBUG_INFO_STR("(%d, %d)", fd, name);

	switch (name) {
		default: {
#if DEBUG
			blue_screen("Unimplemented sysconf variable %d.", name);
#endif
			return -1;
		}
	}
}

// Perform I/O Control
int ioctl(uint32_t fd, int request, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
	LOG_DEBUG_INFO_STR("(%d, %d, %d, %d, %d, %d)", fd, request, arg1, arg2, arg3, arg4);

	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS)
		return -EBADF;
	
	down(&current_pcb->descriptor_lock);
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->ioctl) {
		up(&current_pcb->descriptor_lock);
		return descriptors[fd] ? -EINVAL : -EBADF;
	}
	
	file_descriptor_t* d = descriptors[fd];
	down(&d->lock);
	up(&current_pcb->descriptor_lock);
	
	// Call to the driver specific call
	uint32_t ret = d->ioctl(d, request, arg1, arg2, arg3, arg4);
	
	up(&d->lock);
	return ret;
	
	return -1;
}
