//
//  devices.h
//  NeilOS
//
//  Created by Neil Singh on 9/5/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef DEVICES_H
#define DEVICES_H

#include <common/types.h>
#include <syscalls/descriptor.h>

#define NUM_DEVICE_TYPES				2
#define DEVICE_SHARED_MEMORY_TYPE		1
#define DEVICE_MESSAGE_QUEUE_TYPE		2

// Pointers to open functions
extern file_descriptor_t* (*device_open_functions[NUM_DEVICE_TYPES])(const char* filename, uint32_t mode);
// Pointers to unlink functions
extern uint32_t (*device_unlink_functions[NUM_DEVICE_TYPES])(const char* filename);

bool devices_init();

#endif /* DEVICES_H */
