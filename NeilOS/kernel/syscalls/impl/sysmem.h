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


#endif /* SYSMEM_H */
