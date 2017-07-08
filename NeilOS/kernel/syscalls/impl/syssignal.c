//
//  syssignal.c
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "syssignal.h"
#include <program/task.h>
#include <program/signal.h>
#include <common/log.h>

// Send a signal to a process
uint32_t kill(uint32_t pid, uint32_t signum) {
	LOG_DEBUG_INFO_STR("(%d, %d)", pid, signum);

	if (signum > NUMBER_OF_SIGNALS || signum == 0)
		return -1;
	
	pcb_t* pcb = pcb_from_pid(pid);
	if (!pcb)
		return -1;
	
	signal_send(pcb, signum);
	return 0;
}

// Set the signal handler for a certain signal
uint32_t signal(uint32_t signum, sighandler_t handler) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x)", signum, handler);

	if (signum > NUMBER_OF_SIGNALS || signum == 0)
		return -1;
	
	signal_set_handler(current_pcb, signum, handler);
	return 0;
}

// Mask signals
uint32_t sigsetmask(uint32_t signum, bool masked) {
	LOG_DEBUG_INFO_STR("(%d, %d)", signum, masked);

	signal_set_masked(current_pcb, signum, masked);
	return 0;
}

// Get signal mask
uint32_t siggetmask(uint32_t signum) {
	LOG_DEBUG_INFO_STR("(%d)", signum);

	return signal_is_masked(current_pcb, signum);
}
