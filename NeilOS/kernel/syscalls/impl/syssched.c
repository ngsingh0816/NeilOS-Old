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
		if (current_pcb->should_terminate)
			return end - get_current_time().val;
		schedule();
	}

	return 0;
}
