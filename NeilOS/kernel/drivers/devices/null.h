//
//  null.h
//  NeilOS
//
//  Created by Neil Singh on 9/5/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef NULL_H
#define NULL_H

#include <common/types.h>
#include <syscalls/descriptor.h>

// Open a null device
file_descriptor_t* null_open(const char* filename, uint32_t mode);

// Read nothing
uint32_t null_read(int32_t fd, void* buf, uint32_t bytes);

// Write all
uint32_t null_write(int32_t fd, const void* buf, uint32_t nbytes);

// Get info
uint32_t null_stat(int32_t fd, sys_stat_type* data);

// Duplicate the file handle
file_descriptor_t* null_duplicate(file_descriptor_t* f);

// Close the null device
uint32_t null_close(file_descriptor_t* fd);

#endif /* NULL_H */
