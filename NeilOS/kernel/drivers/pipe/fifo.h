//
//  fifo.h
//  NeilOS
//
//  Created by Neil Singh on 6/26/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef FIFO_H
#define FIFO_H

#include <common/types.h>
#include <syscalls/descriptor.h>

// Open a (unnamed) pipe
file_descriptor_t* fifo_open(const char* filename, uint32_t mode);

// Read a pipe
uint32_t fifo_read(int32_t fd, void* buf, uint32_t bytes);

// Write to a pipe
uint32_t fifo_write(int32_t fd, const void* buf, uint32_t bytes);

// Get info about a pipe
uint32_t fifo_stat(int32_t fd, sys_stat_type* data);

// Seek a fifo (returns error)
uint64_t fifo_llseek(int32_t fd, uint64_t offset, int whence);

// Close a pipe
uint32_t fifo_close(file_descriptor_t* fd);

// Duplicate a pipe
file_descriptor_t* fifo_duplicate(file_descriptor_t* fd);

#endif /* FIFO_H */
