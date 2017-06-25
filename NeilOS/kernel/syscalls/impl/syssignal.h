//
//  syssignal.h
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef SYSSIGNAL_H
#define SYSSIGNAL_H

#include <common/types.h>
#include <program/signal.h>

// Send a signal to a process
uint32_t kill(uint32_t pid, uint32_t signum);

// Set the signal handler for a certain signal
uint32_t signal(uint32_t signum, sighandler_t handler);

// Mask signals
uint32_t sigsetmask(uint32_t signum, bool masked);

// Unmask signals
uint32_t siggetmask(uint32_t signum);

#endif /* SYSSIGNAL_H */
