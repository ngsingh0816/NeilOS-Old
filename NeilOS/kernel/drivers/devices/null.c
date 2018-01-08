//
//  null.c
//  NeilOS
//
//  Created by Neil Singh on 9/5/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "null.h"

#include <common/lib.h>
#include <memory/allocation/heap.h>
#include <drivers/filesystem/filesystem.h>

// Open a null device
file_descriptor_t* null_open(const char* filename, uint32_t mode) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memset(d, 0, sizeof(file_descriptor_t));
	// Mark in use
	d->lock = MUTEX_UNLOCKED;
	d->type = FILE_FILE_TYPE;
	d->mode = FILE_MODE_WRITE | FILE_TYPE_BLOCK;
	d->filename = "null";
	
	// Assign the functions
	d->read = null_read;
	d->write = null_write;
	d->llseek = null_llseek;
	d->stat = null_stat;
	d->duplicate = null_duplicate;
	d->close = null_close;
	
	return d;
}

// Read nothing
uint32_t null_read(file_descriptor_t* f, void* buf, uint32_t bytes) {
	return 0;
}

// Write all
uint32_t null_write(file_descriptor_t* f, const void* buf, uint32_t nbytes) {
	return nbytes;
}

// Seek
uint64_t null_llseek(file_descriptor_t* f, uint64_t offset, int whence) {
	return uint64_make(0, 0);
}

// Get info
uint32_t null_stat(file_descriptor_t* f, sys_stat_type* data) {
	data->dev_id = f->type;
	data->size = 0;
	data->mode = f->mode;
	data->inode = filesystem_get_inode("/dev/null");
	return 0;
}

// Duplicate the file handle
file_descriptor_t* null_duplicate(file_descriptor_t* f) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memcpy(d, f, sizeof(file_descriptor_t));
	d->lock = MUTEX_UNLOCKED;
	
	return d;
}

// Close the null device
uint32_t null_close(file_descriptor_t* fd) {
	return 0;
}
