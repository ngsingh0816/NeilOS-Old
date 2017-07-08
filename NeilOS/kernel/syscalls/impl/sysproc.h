//
//  sysproc.h
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef SYSPROC_H
#define SYSPROC_H

#include <common/types.h>
#include <program/task.h>

// Helpers for execute
int32_t queue_task_load(const char* cmd, const char** argv, const char** envp, pcb_t** ret);
int32_t run(pcb_t* pcb);
int32_t run_with_fake_parent(pcb_t* pcb, pcb_t* parent);

// Program level syscalls

// Duplicate a program and memory space
uint32_t fork();

// Load a new program into the current memory space
uint32_t execve(const char* filename, const char* argv[], const char* envp[]);

// Get the pid of the current process
uint32_t getpid();

// Wait for a child to change state (returns the status that the child returned with)
uint32_t waitpid(uint32_t pid);

// Wait for any child to finish
uint32_t wait(uint32_t pid);

// Exit a program with a specific status
uint32_t exit(int status);

// Get the current working directory
uint32_t getwd(char* buf);

#endif /* SYSPROC_H */
