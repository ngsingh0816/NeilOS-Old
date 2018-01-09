//
//  es1371.h
//  NeilOS
//
//  Created by Neil Singh on 1/2/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef ES1371_H
#define ES1371_H

#include <common/types.h>
#include <syscalls/descriptor.h>

void es_init();

// Open the sound device
file_descriptor_t* es_open(const char* filename, uint32_t mode);

// Blocks until more data is needed then returns the amount of bytes needed to write
uint32_t es_read(file_descriptor_t* f, void* buf, uint32_t bytes);

// Write sound data
uint32_t es_write(file_descriptor_t* f, const void* buf, uint32_t nbytes);

// Get info
uint32_t es_stat(file_descriptor_t* f, sys_stat_type* data);

// Seek (returns error)
uint64_t es_llseek(file_descriptor_t* f, uint64_t offset, int whence);

// ioctl
uint32_t es_ioctl(file_descriptor_t* f, int request, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);

// Duplicate the file handle
file_descriptor_t* es_duplicate(file_descriptor_t* f);

// Close the sound device
uint32_t es_close(file_descriptor_t* fd);

#endif
