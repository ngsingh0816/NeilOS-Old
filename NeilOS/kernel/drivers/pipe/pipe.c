//
//  pipe.c
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "pipe.h"
#include <common/lib.h>
#include <memory/allocation/heap.h>
#include <program/task.h>
#include <common/concurrency/semaphore.h>

typedef struct {
	uint8_t* buffer;
	uint32_t pos;
	file_descriptor_t* reader;
	file_descriptor_t* writer;
	semaphore_t lock;
} pipe_info_t;

// Open a (unnamed) pipe
file_descriptor_t* pipe_open(const char* filename, uint32_t mode) {
	file_descriptor_t* f = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!f)
		return NULL;
	
	memset(f, 0, sizeof(file_descriptor_t));
	if (mode & FILE_MODE_READ) {
		f->read = pipe_read;
	} else if (mode & FILE_MODE_WRITE) {
		f->write = pipe_write;
		pipe_info_t* info = (pipe_info_t*)kmalloc(sizeof(pipe_info_t));
		if (!info) {
			kfree(f);
			return NULL;
		}
		
		// The writer is the original owner of the buffer
		info->buffer = (uint8_t*)kmalloc(PIPE_MAX_BUFFER_SIZE);
		if (!info->buffer) {
			kfree(info);
			kfree(f);
			return NULL;
		}
		info->pos = 0;
		info->reader = NULL;
		info->writer = f;
		info->lock = MUTEX_UNLOCKED;
		f->info = info;
	}
	f->stat = pipe_stat;
	f->close = pipe_close;
	f->duplicate = pipe_duplicate;
	f->mode = mode | FILE_TYPE_PIPE;
	f->ref_count = 1;
	
	return f;
}

// Connect an input and output pipe
bool pipe_connect(file_descriptor_t* input, file_descriptor_t* output) {
	pipe_info_t* output_info = (pipe_info_t*)output->info;
	
	input->info = output_info;
	output_info->reader = input;
	
	return true;
}

// Read a pipe
uint32_t pipe_read(int32_t fd, void* buf, uint32_t bytes) {
	pipe_info_t* info = (pipe_info_t*)descriptors[fd]->info;
	
	// Writer pipe was closed
	if (!info)
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
uint32_t pipe_write(int32_t fd, const void* buf, uint32_t bytes) {
	pipe_info_t* info = (pipe_info_t*)descriptors[fd]->info;
	
	// Reader pipe has been closed
	if (!info->reader) {
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
uint32_t pipe_stat(int32_t fd, sys_stat_type* data) {
	pipe_info_t* info = (pipe_info_t*)descriptors[fd]->info;
	memset(data, 0, sizeof(sys_stat_type));
	data->dev_id = 1;
	data->block_size = PIPE_MAX_BUFFER_SIZE;
	if (info) {
		data->size = info->pos;
		data->num_links = (info->reader != NULL) + (info->writer != NULL);
	}
	data->num_512_blocks = data->size / 512;
	data->mode = descriptors[fd]->mode;
	
	return 0;
}

// Close a pipe
uint32_t pipe_close(file_descriptor_t* fd) {
	if (fd->ref_count == 0) {
		pipe_info_t* info = (pipe_info_t*)fd->info;
		if (!info)
			return 0;
		
		if (fd->mode & FILE_MODE_WRITE)
			info->writer = NULL;
		else
			info->reader = NULL;
		
		if (!info->reader && !info->writer) {
			if (info->buffer)
				kfree(info->buffer);
			kfree(info);
		}
	}
	
	return 0;
}

// Duplicate a pipe
file_descriptor_t* pipe_duplicate(file_descriptor_t* fd) {
	fd->ref_count++;
	return fd;
}
