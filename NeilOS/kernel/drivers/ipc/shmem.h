//
//  shmem.h
//  product
//
//  Created by Neil Singh on 1/7/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef SHMEM_H
#define SHMEM_H

#include <common/types.h>
#include <syscalls/descriptor.h>

// Get an array of the paddrs available (must be kfreed)
uint32_t* shm_get_paddrs(file_descriptor_t* f, uint32_t* length);

// Open a shared memory region
file_descriptor_t* shm_open(const char* filename, uint32_t mode);

// Read a shared memory region (reads 0's to clear newly allocated memory)
uint32_t shm_read(file_descriptor_t* f, void* buf, uint32_t bytes);

// Write a shared memory region (nop)
uint32_t shm_write(file_descriptor_t* f, const void* buf, uint32_t bytes);

// Seek a shared memory region (nop)
uint64_t shm_llseek(file_descriptor_t* f, uint64_t offset, int whence);

// Resize a shared memory region
uint64_t shm_truncate(file_descriptor_t* f, uint64_t size);

// Get info
uint32_t shm_stat(file_descriptor_t* f, sys_stat_type* data);

// Can read or write to a shared memory region
bool shm_can_read(file_descriptor_t* f);
bool shm_can_write(file_descriptor_t* f);

// Duplicate a shared memory region
file_descriptor_t* shm_duplicate(file_descriptor_t* f);

// Close a shared memory region
uint32_t shm_close(file_descriptor_t* f);

// Unlink a shared memory region
uint32_t shm_unlink(const char* filename);

#endif /* SHMEM_H */
