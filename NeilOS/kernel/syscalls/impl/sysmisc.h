//
//  sysmisc.h
//  NeilOS
//
//  Created by Neil Singh on 6/29/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef SYSMISC_H
#define SYSMISC_H

#include <common/types.h>

// Get some info about a limit
uint32_t sysconf(uint32_t name);

// Get some info about a file limit
uint32_t fpathconf(uint32_t fd, uint32_t name);

// Perform I/O Control
int ioctl(uint32_t fd, int request, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);

#endif /* SYSMISC_H */
