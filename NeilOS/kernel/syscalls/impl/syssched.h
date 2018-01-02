//
//  syssched.h
//  NeilOS
//
//  Created by Neil Singh on 8/31/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef SYSSCHED_H
#define SYSSCHED_H

#include <common/types.h>
#include <syscalls/descriptor.h>

// Put calling thread to sleep
uint32_t sleep(uint32_t seconds);

// Yield the calling thread
uint32_t sched_yield();

#endif /* SYSSCHED_H */
