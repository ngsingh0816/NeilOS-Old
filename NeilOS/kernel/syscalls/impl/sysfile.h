//
//  sysfile.h
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef SYSFILE_H
#define SYSFILE_H

#include <common/types.h>
#include <syscalls/descriptor.h>

// Helper for open
file_descriptor_t* open_handle(const char* filename, uint32_t mode);
// Open a file descriptor
uint32_t open(const char* filename, uint32_t mode);

// Read from a file descriptor
uint32_t read(int32_t fd, void* buf, uint32_t nbytes);

// Write to a file descriptor
uint32_t write(int32_t fd, const void* buf, uint32_t nbytes);

// Seek to an offset
uint32_t llseek(int32_t fd, uint32_t offset_high, uint32_t offset_low, int whence);

// Change the size of a file
uint32_t truncate(int32_t fd, uint32_t length_high, uint32_t length_low);

// Get information about a file descriptor
uint32_t stat(int32_t fd, sys_stat_type* data);

// Close a file descriptor
uint32_t close(int32_t fd);

// Is the file descriptor a terminal?
uint32_t isatty(int32_t fd);

// Duplicate a file descriptor
uint32_t dup(int32_t fd);

// Duplicate a file descriptor into the specific file descriptor
uint32_t dup2(int32_t fd, int32_t new_fd);

// Create a pipe
uint32_t pipe(int32_t pipefd[2]);

// Extensions
int fcntl(int32_t fd, int32_t cmd, ...);

#endif /* SYSFILE_H */
