//
//  syssched.c
//  NeilOS
//
//  Created by Neil Singh on 8/31/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "syssched.h"
#include <common/log.h>
#include <program/task.h>
#include <syscalls/interrupt.h>

// Put calling thread to sleep
uint32_t nanosleep(const struct timespec* req, struct timespec* rem) {
	LOG_DEBUG_INFO_STR("(%d.%d)", req->tv_sec, req->tv_usec);

	// Require minimum of 1ms of sleep
	uint32_t sec = req->tv_sec, nanos = req->tv_nsec;
	if (sec == 0 && nanos < NANOS_IN_MS)
		nanos = NANOS_IN_MS;
	
	struct timeval end = time_add(time_get(), (struct timeval){ sec, nanos / NANOS_IN_US });
	while (time_less(time_get(), end)) {
		down(&current_pcb->lock);
		if (current_pcb->should_terminate || signal_occurring(current_pcb)) {
			up(&current_pcb->lock);
			if (rem)
				*rem = timeval_to_timespec(time_subtract(end, time_get()));
			return -EINTR;
		}
		up(&current_pcb->lock);
		schedule();
	}
	
	return 0;
}

// Yield the calling thread
uint32_t sched_yield() {
	schedule();
	return 0;
}
