//
//  mouse.h
//  NeilOS
//
//  Created by Neil Singh on 1/2/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef MOUSE_H
#define MOUSE_H

#include <common/types.h>
#include <syscalls/descriptor.h>

void mouse_init();

void mouse_handle();

// Initialize the mouse
file_descriptor_t* mouse_open(const char* filename, uint32_t mode);

// Returns the last mouse position if in nonblocking mode, otherwise blocks until mouse is moved
uint32_t mouse_read(file_descriptor_t* f, void* buf, uint32_t bytes);

// Sets the cursor position
uint32_t mouse_write(file_descriptor_t* f, const void* buf, uint32_t nbytes);

// Get info
uint32_t mouse_stat(file_descriptor_t* f, sys_stat_type* data);

// Can read
bool mouse_can_read(file_descriptor_t* f);

// Can write
bool mouse_can_write(file_descriptor_t* f);

// Seek a mouse (returns error)
uint64_t mouse_llseek(file_descriptor_t* f, uint64_t offset, int whence);

// Duplicate the file handle
file_descriptor_t* mouse_duplicate(file_descriptor_t* f);

// Close the mouse
uint32_t mouse_close(file_descriptor_t* fd);

#endif
