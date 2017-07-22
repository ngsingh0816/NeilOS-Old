//
//  pipe.h
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef PIPE_H
#define PIPE_H

#include <common/types.h>
#include <syscalls/descriptor.h>

#define PIPE_MAX_BUFFER_SIZE		1024

// Open a (unnamed) pipe
file_descriptor_t* pipe_open(const char* filename, uint32_t mode);

// Connect an input and output pipe
bool pipe_connect(file_descriptor_t* input, file_descriptor_t* output);

// Read a pipe
uint32_t pipe_read(int32_t fd, void* buf, uint32_t bytes);

// Write to a pipe
uint32_t pipe_write(int32_t fd, const void* buf, uint32_t bytes);

// Get info about a pipe
uint32_t pipe_stat(int32_t fd, sys_stat_type* data);

// Close a pipe
uint32_t pipe_close(file_descriptor_t* fd);

// Duplicate a pipe
file_descriptor_t* pipe_duplicate(file_descriptor_t* fd);

#endif /* PIPE_H */