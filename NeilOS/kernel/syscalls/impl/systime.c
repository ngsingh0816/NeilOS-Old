//
//  systime.c
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "systime.h"
#include <common/time.h>
#include <common/log.h>
#include <syscalls/interrupt.h>

// Get the timing information for a process
uint32_t times(sys_time_type* data) {
	LOG_DEBUG_INFO_STR("(0x%x)", data);

	if (!data)
		return -EFAULT;
	
	// TODO: fill these out
	data->child_user_time.val = 0;
	data->child_process_time.val = 0;
	data->system_time.val = 0;
	data->user_time.val = 0;
	
	return 0;
}

// Returns the time of day in seconds
uint32_t gettimeofday(struct timeval* t) {
	LOG_DEBUG_INFO_STR("(0x%x)", t);

	if (t)
		*t = time_get();
	
	return 0;
}
