//
//  syssched.c
//  NeilOS
//
//  Created by Neil Singh on 8/31/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "syssched.h"
#include <common/log.h>
#include <common/time.h>
#include <program/task.h>

// Put calling thread to sleep
uint32_t sleep(uint32_t seconds) {
	LOG_DEBUG_INFO_STR("(%d)", seconds);
	
	uint32_t end = get_current_time().val + seconds;
	while (get_current_time().val < end) {
		down(&current_pcb->lock);
		if (current_pcb->should_terminate) {
			up(&current_pcb->lock);
			return end - get_current_time().val;
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
