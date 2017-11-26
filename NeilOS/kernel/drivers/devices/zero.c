//
//  zero.c
//  NeilOS
//
//  Created by Neil Singh on 9/5/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "zero.h"

#include <common/lib.h>
#include <memory/allocation/heap.h>

// Open a zero device
file_descriptor_t* zero_open(const char* filename, uint32_t mode) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memset(d, 0, sizeof(file_descriptor_t));
	// Mark in use
	d->type = FILE_FILE_TYPE;
	d->mode = FILE_MODE_WRITE | FILE_TYPE_BLOCK;
	d->filename = "zero";
	
	// Assign the functions
	d->read = zero_read;
	d->write = zero_write;
	d->stat = zero_stat;
	d->duplicate = zero_duplicate;
	d->close = zero_close;
	
	return d;
}

// Read zeros
uint32_t zero_read(int32_t fd, void* buf, uint32_t bytes) {
	memset(buf, 0, bytes);
	return bytes;
}

// Write all
uint32_t zero_write(int32_t fd, const void* buf, uint32_t nbytes) {
	return nbytes;
}

// Get info
uint32_t zero_stat(int32_t fd, sys_stat_type* data) {
	data->dev_id = 1;
	data->size = 0;
	data->mode = descriptors[fd]->mode;
	return 0;
}

// Duplicate the file handle
file_descriptor_t* zero_duplicate(file_descriptor_t* f) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memcpy(d, f, sizeof(file_descriptor_t));
	
	return d;
}

// Close the zero device
uint32_t zero_close(file_descriptor_t* fd) {
	return 0;
}