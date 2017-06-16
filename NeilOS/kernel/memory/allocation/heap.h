//
//  heap.h
//  MemoryAllocator
//
//  Created by Neil Singh on 12/12/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#ifndef heap_h
#define heap_h

#include <common/types.h>

void heap_init();

void* kmalloc(uint32_t size);
void kfree(void* addr);

#endif /* heap_h */
