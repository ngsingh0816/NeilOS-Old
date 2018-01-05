//
//  systime.h
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef SYSTIME_H
#define SYSTIME_H

#include <common/types.h>
#include <syscalls/descriptor.h>

// Time
// Get the timing information for a process
uint32_t times(sys_time_type* data);

// Returns the time of day in milliseconds
uint32_t gettimeofday(struct timeval* t);

#endif /* SYSTIME_H */
