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

typedef unsigned long sigset_t;

// Set the signal handler for a certain signal
uint32_t sigaction(uint32_t signum, sigaction_t* act, sigaction_t* oldact);

// Mask signals
uint32_t sigsetmask(uint32_t mask);

// Unmask signals
uint32_t siggetmask();

// Set signal masks
uint32_t sigprocmask(int how, const sigset_t* set, sigset_t* oldset);

// Suspend execution until signal
uint32_t sigsuspend(const sigset_t* mask);

// Set up alarm
uint32_t alarm(uint32_t seconds);

#endif /* SYSSIGNAL_H */
