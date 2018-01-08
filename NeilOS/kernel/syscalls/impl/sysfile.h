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
#include <common/time.h>

// Helper for open
file_descriptor_t* open_handle(const char* filename, uint32_t mode, uint32_t type);
// Open a file descriptor
uint32_t open(const char* filename, uint32_t mode, uint32_t type);

// Read from a file descriptor
uint32_t read(uint32_t fd, void* buf, uint32_t nbytes);

// Write to a file descriptor
uint32_t write(uint32_t fd, const void* buf, uint32_t nbytes);

// Seek to an offset
uint32_t llseek(uint32_t fd, uint32_t offset_high, uint32_t offset_low, int whence);

// Change the size of a file
uint32_t truncate(uint32_t fd, uint32_t length_high, uint32_t length_low);

// Get information about a file descriptor
uint32_t stat(uint32_t fd, sys_stat_type* data);

// Close a file descriptor
uint32_t close(uint32_t fd);

// Is the file descriptor a terminal?
uint32_t isatty(uint32_t fd);

// Duplicate a file descriptor
uint32_t dup(uint32_t fd);

// Duplicate a file descriptor into the specific file descriptor
uint32_t dup2(uint32_t fd, uint32_t new_fd);

// Create a pipe
uint32_t pipe(uint32_t pipefd[2]);

// Extensions
int fcntl(uint32_t fd, int32_t cmd, ...);

// Wait for changes to file descriptors
int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);

#endif /* SYSFILE_H */
