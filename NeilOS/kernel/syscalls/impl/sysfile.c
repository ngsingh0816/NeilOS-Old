//
//  sysfile.c
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "sysfile.h"
#include <common/lib.h>
#include <memory/allocation/heap.h>
#include <program/task.h>
#include <drivers/filesystem/filesystem.h>
#include <drivers/ATA/ata.h>
#include <drivers/rtc/rtc.h>
#include <drivers/pipe/pipe.h>
#include <drivers/pipe/fifo.h>

// Device for syscall's purposes
typedef struct syscall_device {
	char* name;
	file_descriptor_t* (*open)(const char*, uint32_t mode);
} syscall_device_t;

file_descriptor_t** descriptors = NULL;
syscall_device_t syscall_devices[] = {
	{ "stdin", terminal_open },
	{ "stdout", terminal_open },
	{ "stderr", terminal_open },
	{ "rtc", rtc_open },
	{ "disk0", ata_open },
	{ "disk0s1", ata_open },
};

file_descriptor_t* open_handle(const char* filename, uint32_t mode) {
	file_descriptor_t* f = NULL;
	
	// Fifo
	if ((mode & FILE_TYPE_PIPE) && (mode & FILE_MODE_CREATE)) {
		f = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
		if (!f)
			return NULL;
		
		if (!fopen(filename, mode, f)) {
			kfree(f);
			return NULL;
		}
		
		f->ref_count = 1;
		return f;
	}
	
	// Loop through the devices
	int z;
	for (z = 0; z < sizeof(syscall_devices) / sizeof(syscall_device_t); z++) {
		if (strncmp(filename, syscall_devices[z].name, strlen(syscall_devices[z].name) + 1) == 0) {
			f = syscall_devices[z].open(filename, mode);
			if (f) {
				f->ref_count = 1;
				return f;
			}
			return NULL;
		}
	}
	
	// Default to the filesystem if no device was found
	f = filesystem_open(filename, mode);
	if (f) {
		f->ref_count = 1;
		return f;
	}
	
	return NULL;
}

// Open a file descriptor
uint32_t open(const char* filename, uint32_t mode) {
	// Find and open descriptor
	uint32_t current_fd = -1;
	int z;								// Looping over descriptors
	for (z = 0; z < NUMBER_OF_DESCRIPTORS; z++) {
		if (!descriptors[z]) {
			current_fd = z;
			break;
		}
	}
	
	// We have no more descriptors available so we can't open anything
	if (current_fd == -1)
		return -1;
	
	pcb_t* pcb = current_pcb;
	pcb->descriptors[current_fd] = open_handle(filename, mode);
	if (!pcb->descriptors[current_fd])
		return -1;
	
	descriptors = pcb->descriptors;
	return current_fd;
}

// Read from a file descriptor
uint32_t read(int32_t fd, void* buf, uint32_t nbytes) {
	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS || fd < 0 || nbytes == 0 || !buf)
		return -1;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->read)
		return -1;
	
	// Call to the driver specific call
	return descriptors[fd]->read(fd, buf, nbytes);
}

// Write to a file descriptor
uint32_t write(int32_t fd, const void* buf, uint32_t nbytes) {
	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS || fd < 0 || nbytes == 0 || !buf)
		return -1;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->write)
		return -1;
	
	// Call to the driver specific call
	return descriptors[fd]->write(fd, buf, nbytes);
}

// Seek to an offset
uint32_t llseek(int32_t fd, uint32_t offset_high, uint32_t offset_low, int whence) {
	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS || fd < 0 || !(whence >= SEEK_SET && whence <= SEEK_END))
		return -1;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->llseek)
		return -1;
	
	// Call to the driver specific call
	// TODO: for now only return the lower 32 bits
	return descriptors[fd]->llseek(fd, uint64_make(offset_high, offset_low), whence).low;
	
	return 0;
}

// Change the size of a file
uint32_t truncate(int32_t fd, uint32_t length_high, uint32_t length_low) {
	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS || fd < 0)
		return -1;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->truncate)
		return -1;
	
	// Call to the driver specific call
	// TODO: for now just return the lower 32 bits
	return descriptors[fd]->truncate(fd, uint64_make(length_high, length_low)).low;
}

// Get information about a file descriptor
uint32_t stat(int32_t fd, sys_stat_type* data) {
	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS || fd < 0 || !data)
		return -1;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->stat)
		return -1;
	
	return descriptors[fd]->stat(fd, data);
}

// Close a file descriptor
uint32_t close(int32_t fd) {
	// Check if this descriptor is within the bounds
	if (fd >= NUMBER_OF_DESCRIPTORS || fd < 0)
		return -1;
	
	// If we are trying to close an invalid descriptor, return failure
	if (!descriptors[fd])
		return -1;
	
	uint32_t ret = 0;
	if ((--descriptors[fd]->ref_count) == 0) {
		ret = descriptors[fd]->close((void*)descriptors[fd]);
		kfree(descriptors[fd]);
	}
	pcb_t* pcb = current_pcb;
	pcb->descriptors[fd] = NULL;
	
	descriptors = pcb->descriptors;
	
	return ret;
}

// Is the file descriptor a terminal?
uint32_t isatty(int32_t fd) {
	// Check if this descriptor is within the bounds
	if (fd >= NUMBER_OF_DESCRIPTORS || fd < 0)
		return 0;
	
	// If we are trying to close an invalid descriptor, return failure
	if (!descriptors[fd])
		return 0;
	
	return (descriptors[fd]->type == STANDARD_INOUT_TYPE);
}

// Duplicate a file descriptor
uint32_t dup(int32_t fd) {
	// Find and open descriptor
	uint32_t current_fd = -1;
	int z;								// Looping over descriptors
	for (z = 0; z < NUMBER_OF_DESCRIPTORS; z++) {
		if (!descriptors[z]) {
			current_fd = z;
			break;
		}
	}
	
	// We have no more descriptors available so we can't open anything
	if (current_fd == -1)
		return -1;
	
	return dup2(fd, current_fd);
}

// Duplicate a file descriptor into the specific file descriptor
uint32_t dup2(int32_t fd, int32_t new_fd) {
	// Check if this descriptor is within the bounds
	if (fd >= NUMBER_OF_DESCRIPTORS || new_fd >= NUMBER_OF_DESCRIPTORS)
		return -1;
	
	if (!descriptors[fd])
		return -1;
	
	bool flags = set_multitasking_enabled(false);
	pcb_t* pcb = current_pcb;
	// Close the file handle if opened
	if (pcb->descriptors[new_fd]) {
		pcb->descriptors[new_fd]->close(pcb->descriptors[new_fd]);
		if ((--pcb->descriptors[new_fd]->ref_count) == 0)
			kfree(pcb->descriptors[new_fd]);
	}
	// Point the new file handle to the old one
	pcb->descriptors[new_fd] = pcb->descriptors[fd];
	pcb->descriptors[new_fd]->ref_count++;
	
	descriptors = pcb->descriptors;
	
	set_multitasking_enabled(flags);
	
	return new_fd;
}

// Create a pipe
uint32_t pipe(int32_t pipefd[2]) {
	// Find and open descriptor
	uint32_t current_fd[2] = { -1, -1 };
	int z, i = 0;								// Looping over descriptors
	for (z = 0; z < NUMBER_OF_DESCRIPTORS; z++) {
		if (!descriptors[z]) {
			current_fd[i++] = z;
			if (i == 2)
				break;
		}
	}
	
	// We have no more descriptors available so we can't open anything
	if (current_fd[0] == -1 || current_fd[1] == -1)
		return -1;
	
	pcb_t* pcb = current_pcb;
	
	// Open the input pipe
	file_descriptor_t* input = pipe_open(NULL, FILE_MODE_READ);
	if (!input)
		return -1;
	
	// Open the output pipe
	file_descriptor_t* output = pipe_open(NULL, FILE_MODE_WRITE);
	if (!output) {
		pipe_close(input);
		return -1;
	}
	
	// Connect the pipes
	if (!pipe_connect(input, output)) {
		pipe_close(input);
		pipe_close(output);
		return -1;
	}
	
	// Assign
	pcb->descriptors[current_fd[0]] = input;
	pcb->descriptors[current_fd[1]] = output;
	pipefd[0] = current_fd[0];
	pipefd[1] = current_fd[1];
	descriptors = pcb->descriptors;
	
	return 0;
}
