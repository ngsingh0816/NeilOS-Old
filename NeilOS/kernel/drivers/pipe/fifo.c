//
//  fifo.c
//  NeilOS
//
//  Created by Neil Singh on 6/26/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "fifo.h"
#include <common/concurrency/semaphore.h>
#include <drivers/filesystem/ext2/ext2.h>
#include <drivers/filesystem/filesystem.h>
#include <drivers/filesystem/path.h>
#include <drivers/pipe/pipe.h>
#include <memory/allocation/heap.h>
#include <program/task.h>

// Currently open fifo lists
typedef struct fifo_list {
	char* filename;
	volatile uint32_t num_readers;
	volatile uint32_t num_writers;
	
	char* buffer;
	volatile uint32_t pos;
	semaphore_t lock;
	
	struct fifo_list* next;
	struct fifo_list* prev;
} fifo_list_t;

fifo_list_t* open_fifos = NULL;
semaphore_t open_fifos_lock = MUTEX_UNLOCKED;

// Open a (unnamed) pipe (only called if file exists and is a fifo)
file_descriptor_t* fifo_open(const char* filename, uint32_t mode) {
	file_descriptor_t* desc = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!desc)
		return NULL;
	
	uint32_t name_len = strlen(filename);
	memset(desc, 0, sizeof(file_descriptor_t));
	desc->filename = (char*)kmalloc(name_len + 1);
	if (!desc->filename) {
		kfree(desc);
		return NULL;
	}
	memcpy(desc->filename, filename, name_len + 1);
	
	// Find this in the list of open fifos
	fifo_list_t* prev = NULL;
	bool found = false;
	down(&open_fifos_lock);
	fifo_list_t* t = open_fifos;
	while (t) {
		if (strncmp(filename, t->filename, name_len + 1) == 0) {
			found = true;
			break;
		}
		prev = t;
		t = t->next;
	}
	// Create a new entry if we have to
	if (!found) {
		fifo_list_t* list = (fifo_list_t*)kmalloc(sizeof(fifo_list_t));
		if (!list) {
			up(&open_fifos_lock);
			kfree(desc->filename);
			kfree(desc);
			return NULL;
		}
		memset(list, 0, sizeof(fifo_list_t));
		list->lock = MUTEX_UNLOCKED;
		
		list->filename = (char*)kmalloc(name_len + 1);
		if (!list->filename) {
			up(&open_fifos_lock);
			kfree(list);
			kfree(desc->filename);
			kfree(desc);
			return NULL;
		}
		memcpy(list->filename, filename, name_len + 1);
		
		list->buffer = (char*)kmalloc(PIPE_MAX_BUFFER_SIZE);
		if (!list->buffer) {
			up(&open_fifos_lock);
			kfree(list->filename);
			kfree(list);
			kfree(desc->filename);
			kfree(desc);
			return NULL;
		}
		
		if (prev) {
			prev->next = list;
			list->prev = prev;
		}
		else
			open_fifos = list;
		t = list;
	}
	if (mode & FILE_MODE_READ)
		t->num_readers++;
	if (mode & FILE_MODE_WRITE)
		t->num_writers++;
	up(&open_fifos_lock);
	
	desc->info = t;
	desc->mode = mode | FILE_TYPE_PIPE;
	if (mode & FILE_MODE_READ)
		desc->read = fifo_read;
	if (mode & FILE_MODE_WRITE)
		desc->write = fifo_write;
	desc->stat = fifo_stat;
	desc->duplicate = fifo_duplicate;
	desc->close = fifo_close;
	
	// Block until the opposite type is found
	if (!(mode & FILE_MODE_NONBLOCKING)) {
		while ((((mode & FILE_MODE_READ) && t->num_writers == 0 && t->pos == 0) ||
				((mode & FILE_MODE_WRITE) && t->num_readers == 0)) &&
			   !current_pcb->should_terminate)
				schedule();
	}
	
	return desc;
}

// Read a pipe
uint32_t fifo_read(int32_t fd, void* buf, uint32_t bytes) {
	fifo_list_t* info = (fifo_list_t*)descriptors[fd]->info;
	
	// Writer pipe was closed
	if (!info || (info->num_writers == 0 && info->pos == 0))
		return 0;
	
	if (!(descriptors[fd]->mode & FILE_MODE_NONBLOCKING) &&
		info->pos == 0)
		return -1;
	
	// Block until there is data
	while (info->pos == 0 && !current_pcb->should_terminate) {
		schedule();
	}
	
	// Read the data in
	down(&info->lock);
	uint32_t min = (bytes < info->pos) ? bytes : info->pos;
	memcpy(buf, info->buffer, min);
	if (min < info->pos)
		memmove(info->buffer, &info->buffer[min], info->pos - min);
	info->pos -= min;
	up(&info->lock);
	
	return min;
}

// Write to a pipe
uint32_t fifo_write(int32_t fd, const void* buf, uint32_t bytes) {
	fifo_list_t* info = (fifo_list_t*)descriptors[fd]->info;
	
	// Reader pipe has been closed
	if (!info || info->num_readers == 0) {
		signal_send(current_pcb, SIGPIPE);
		return -1;
	}
	
	if (!(descriptors[fd]->mode & FILE_MODE_NONBLOCKING) &&
		info->pos == PIPE_MAX_BUFFER_SIZE)
		return -1;
	
	// Block until the pipe isn't full
	while (info->pos == PIPE_MAX_BUFFER_SIZE && !current_pcb->should_terminate) {
		schedule();
	}
	
	// Write the data
	down(&info->lock);
	uint32_t remaining = PIPE_MAX_BUFFER_SIZE - info->pos;
	uint32_t min = (bytes > remaining) ? remaining : bytes;
	memcpy(&info->buffer[info->pos], buf, min);
	info->pos += min;
	up(&info->lock);
	
	return min;
}

// Get info about a pipe
uint32_t fifo_stat(int32_t fd, sys_stat_type* data) {
	fifo_list_t* list = (fifo_list_t*)descriptors[fd]->info;
	memset(data, 0, sizeof(sys_stat_type));
	data->dev_id = 1;
	data->block_size = PIPE_MAX_BUFFER_SIZE;
	if (list) {
		data->size = list->pos;
		data->num_links = list->num_writers + list->num_readers;
	}
	data->num_512_blocks = data->size / 512;
	data->mode = descriptors[fd]->mode;
	
	return 0;
}

// Close a pipe
uint32_t fifo_close(file_descriptor_t* fd) {
	down(&open_fifos_lock);
	fifo_list_t* list = (fifo_list_t*)fd->info;
	if (fd->mode & FILE_MODE_READ)
		list->num_readers--;
	if (fd->mode & FILE_MODE_WRITE)
		list->num_writers--;
	if (list->num_readers == 0 && list->num_writers == 0) {
		if (list->prev)
			list->prev->next = list->next;
		else
			open_fifos = list->next;
		kfree(list->buffer);
		kfree(list->filename);
		kfree(list);
	}
	up(&open_fifos_lock);
	
	kfree(fd->filename);
	
	return 0;
}

// Duplicate a pipe
file_descriptor_t* fifo_duplicate(file_descriptor_t* fd) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	
	down(&open_fifos_lock);
	memcpy(d, fd, sizeof(file_descriptor_t));
	
	uint32_t len = strlen(fd->filename);
	d->filename = kmalloc(len + 1);
	if (!d->filename) {
		up(&open_fifos_lock);
		kfree(d);
		return NULL;
	}
	memcpy(d->filename, fd->filename, len + 1);
	
	fifo_list_t* list = (fifo_list_t*)d->info;
	if (fd->mode & FILE_MODE_READ)
		list->num_readers++;
	if (fd->mode & FILE_MODE_WRITE)
		list->num_writers++;
	up(&open_fifos_lock);
	
	return d;
}