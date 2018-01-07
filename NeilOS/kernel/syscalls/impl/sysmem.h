//
//  sysmem.h
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef SYSMEM_H
#define SYSMEM_H

#include <common/types.h>

// Set the program break to a specific address
uint32_t brk(uint32_t addr);

// Offset the current program break by a specific ammount
void* sbrk(int32_t offset);

// Map a region of memory
void* mmap(void* addr, uint32_t length, int prot, int flags, int fd, uint32_t offset);

// Unmap a region of memory
int munmap(void* addr, uint32_t length);

// Flush memory contents
int msync(void* addr, uint32_t length, int flags);

#endif /* SYSMEM_H */

