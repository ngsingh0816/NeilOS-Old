//
//  zero.h
//  NeilOS
//
//  Created by Neil Singh on 9/5/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef ZERO_H
#define ZERO_H

#include <common/types.h>
#include <syscalls/descriptor.h>

// Open a zero device
file_descriptor_t* zero_open(const char* filename, uint32_t mode);

// Read zeros
uint32_t zero_read(file_descriptor_t* f, void* buf, uint32_t bytes);

// Write all
uint32_t zero_write(file_descriptor_t* f, const void* buf, uint32_t nbytes);

// Seek
uint64_t zero_llseek(file_descriptor_t* f, uint64_t offset, int whence);

// Get info
uint32_t zero_stat(file_descriptor_t* f, sys_stat_type* data);

// Duplicate the file handle
file_descriptor_t* zero_duplicate(file_descriptor_t* f);

// Close the zero device
uint32_t zero_close(file_descriptor_t* fd);

#endif /* ZERO_H */
