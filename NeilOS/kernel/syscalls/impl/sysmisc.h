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
uint32_t sysconf(int name);

// Get most recent error number
uint32_t sys_errno();

// Perform I/O Control
int ioctl(int fd, int request, ...);

#endif /* SYSMISC_H */
