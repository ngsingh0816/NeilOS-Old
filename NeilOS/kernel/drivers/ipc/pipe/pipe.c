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
#include <syscalls/interrupt.h>

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
	f->lock = MUTEX_UNLOCKED;
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
	f->llseek = pipe_llseek;
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
uint32_t pipe_read(file_descriptor_t* f, void* buf, uint32_t bytes) {
	pipe_info_t* info = (pipe_info_t*)f->info;
	
	// Writer pipe was closed and no data left
	down(&info->lock);
	if (!info->writer && info->pos == 0) {
		up(&info->lock);
		return 0;
	}
	
	if ((f->mode & FILE_MODE_NONBLOCKING) &&
		info->pos == 0) {
		up(&info->lock);
		return 0;
	}
	
	// Block until there is data
	while (info->pos == 0 && !current_pcb->should_terminate && !f->closed) {
		up(&info->lock);
		up(&f->lock);
		schedule();
		down(&f->lock);
		down(&info->lock);
	}
	
	// Read the data in
	uint32_t min = (bytes < info->pos) ? bytes : info->pos;
	memcpy(buf, info->buffer, min);
	if (min < info->pos)
		memmove(info->buffer, &info->buffer[min], info->pos - min);
	info->pos -= min;
	up(&info->lock);
	
	return min;
}

// Write to a pipe
uint32_t pipe_write(file_descriptor_t* f, const void* buf, uint32_t bytes) {
	pipe_info_t* info = (pipe_info_t*)f->info;
	
	// Reader pipe has been closed
	down(&info->lock);
	if (!info->reader) {
		up(&info->lock);
		signal_send(current_pcb, SIGPIPE);
        return -EPIPE;
	}
	
	if ((f->mode & FILE_MODE_NONBLOCKING) &&
		info->pos == PIPE_MAX_BUFFER_SIZE) {
		up(&info->lock);
		return 0;
	}

	// Block until the pipe isn't full
	while (info->pos == PIPE_MAX_BUFFER_SIZE && !current_pcb->should_terminate && !f->closed) {
		if (signal_occurring(current_pcb)) {
			up(&info->lock);
			return -EINTR;
		}
		up(&info->lock);
		up(&f->lock);
		schedule();
		down(&f->lock);
		down(&info->lock);
	}
	
	// Write the data
	uint32_t remaining = PIPE_MAX_BUFFER_SIZE - info->pos;
	uint32_t min = (bytes > remaining) ? remaining : bytes;
	memcpy(&info->buffer[info->pos], buf, min);
	info->pos += min;
	up(&info->lock);
	
	return min;
}

// Get info about a pipe
uint32_t pipe_stat(file_descriptor_t* f, sys_stat_type* data) {
	pipe_info_t* info = (pipe_info_t*)f->info;
	memset(data, 0, sizeof(sys_stat_type));
	data->dev_id = 1;
	data->block_size = PIPE_MAX_BUFFER_SIZE;
	if (info) {
		down(&info->lock);
		data->size = info->pos;
		data->num_links = (info->reader != NULL) + (info->writer != NULL);
		up(&info->lock);
	}
	data->num_512_blocks = data->size / 512;
	data->mode = f->mode;
	
	return 0;
}

// Seek a pipe (returns error)
uint64_t pipe_llseek(file_descriptor_t* f, uint64_t offset, int whence) {
	return uint64_make(-1, -ESPIPE);
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
