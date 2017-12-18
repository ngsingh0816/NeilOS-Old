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
#include <drivers/filesystem/path.h>
#include <drivers/ATA/ata.h>
#include <drivers/rtc/rtc.h>
#include <drivers/pipe/pipe.h>
#include <drivers/pipe/fifo.h>
#include <common/log.h>
#include <syscalls/interrupt.h>

file_descriptor_t** descriptors = NULL;

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
	LOG_DEBUG_INFO_STR("(%s, %d)", filename, mode);
	
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
	if (current_fd == -1) {
        return -ENFILE;
	}
	
	pcb_t* pcb = current_pcb;
	char* path = path_absolute(filename, pcb->working_dir);

	pcb->descriptors[current_fd] = open_handle(path, mode);
	kfree(path);
	if (!pcb->descriptors[current_fd]) {
        return -ENOENT;
	}
	
	descriptors = pcb->descriptors;
	return current_fd;
}

// Read from a file descriptor
uint32_t read(uint32_t fd, void* buf, uint32_t nbytes) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x, %d)", fd, buf, nbytes);
	
	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS)
		return -EBADF;
    if (!buf)
        return -EFAULT;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->read)
		return -EBADF;
	
	// Call to the driver specific call
	return descriptors[fd]->read(fd, buf, nbytes);
}

// Write to a file descriptor
uint32_t write(uint32_t fd, const void* buf, uint32_t nbytes) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x, %d)", fd, buf, nbytes);

    // Check if the arguments are in range
    if (fd >= NUMBER_OF_DESCRIPTORS)
        return -EBADF;
    if (!buf)
        return -EFAULT;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->write)
		return -EBADF;
	
	// Call to the driver specific call
	return descriptors[fd]->write(fd, buf, nbytes);
}

// Seek to an offset
uint32_t llseek(uint32_t fd, uint32_t offset_high, uint32_t offset_low, int whence) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x, 0x%x, %d)", fd, offset_high, offset_low, whence);

	// Check if the arguments are in range
    if (fd >= NUMBER_OF_DESCRIPTORS)
        return -EBADF;
    if (!(whence >= SEEK_SET && whence <= SEEK_END))
        return -EINVAL;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->llseek)
		return -EBADF;
	
	// Call to the driver specific call
	// TODO: for now only return the lower 32 bits
	return descriptors[fd]->llseek(fd, uint64_make(offset_high, offset_low), whence).low;
}

// Change the size of a file
uint32_t truncate(uint32_t fd, uint32_t length_high, uint32_t length_low) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x, 0x%x)", fd, length_high, length_low);

    // Check if the arguments are in range
    if (fd >= NUMBER_OF_DESCRIPTORS)
        return -EBADF;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->truncate)
		return -EBADF;
	
	// Call to the driver specific call
	// TODO: for now just return the lower 32 bits
	return descriptors[fd]->truncate(fd, uint64_make(length_high, length_low)).low;
}

// Get information about a file descriptor
uint32_t stat(uint32_t fd, sys_stat_type* data) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x)", fd, data);
	
    // Check if the arguments are in range
    if (fd >= NUMBER_OF_DESCRIPTORS)
        return -EBADF;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->stat)
		return -EBADF;
	
	return descriptors[fd]->stat(fd, data);
}

// Close a file descriptor
uint32_t close(uint32_t fd) {
	LOG_DEBUG_INFO_STR("(%d)", fd);

	// Check if this descriptor is within the bounds
	if (fd >= NUMBER_OF_DESCRIPTORS)
		return -EBADF;
	
	// If we are trying to close an invalid descriptor, return failure
	if (!descriptors[fd])
		return -EBADF;
	
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
uint32_t isatty(uint32_t fd) {
	LOG_DEBUG_INFO_STR("(%d)", fd);

	// Check if this descriptor is within the bounds
	if (fd >= NUMBER_OF_DESCRIPTORS)
		return -EBADF;
	
	// If we are trying to close an invalid descriptor, return failure
	if (!descriptors[fd])
		return -EBADF;
	
	return (descriptors[fd]->type == STANDARD_INOUT_TYPE);
}

// Duplicate a file descriptor
uint32_t dup(uint32_t fd) {
	LOG_DEBUG_INFO_STR("(%d)", fd);

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
		return -ENFILE;
	
	return dup2(fd, current_fd);
}

// Duplicate a file descripter into the first available fd after the argument
uint32_t dup2_greater(int32_t fd, int32_t new_fd) {
	LOG_DEBUG_INFO_STR("(%d, %d)", fd, new_fd);
	
	while (new_fd < NUMBER_OF_DESCRIPTORS && current_pcb->descriptors[new_fd] && new_fd >= 0)
		new_fd++;
	
    if (new_fd >= NUMBER_OF_DESCRIPTORS || new_fd < 0) {
		return -EINVAL;
    }
	
	return dup2(fd, new_fd);
}

// Duplicate a file descriptor into the specific file descriptor
uint32_t dup2(uint32_t fd, uint32_t new_fd) {
	LOG_DEBUG_INFO_STR("(%d, %d)", fd, new_fd);

	// Check if this descriptor is within the bounds
	if (fd >= NUMBER_OF_DESCRIPTORS || new_fd >= NUMBER_OF_DESCRIPTORS)
		return -EBADF;
	
	if (!descriptors[fd])
		return -EBADF;
	
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
uint32_t pipe(uint32_t pipefd[2]) {
	LOG_DEBUG_INFO_STR("(%d, %d)", pipefd[0], pipefd[1]);
	
	if (!pipefd)
		return -EFAULT;

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
		return -ENFILE;
	
	pcb_t* pcb = current_pcb;
	
	// Open the input pipe
	file_descriptor_t* input = pipe_open(NULL, FILE_MODE_READ);
	if (!input)
		return -ENFILE;
	
	// Open the output pipe
	file_descriptor_t* output = pipe_open(NULL, FILE_MODE_WRITE);
	if (!output) {
		pipe_close(input);
		return -ENFILE;
	}
	
	// Connect the pipes
	if (!pipe_connect(input, output)) {
		pipe_close(input);
		pipe_close(output);
		return -ENFILE;
	}
	
	// Assign
	pcb->descriptors[current_fd[0]] = input;
	pcb->descriptors[current_fd[1]] = output;
	pipefd[0] = current_fd[0];
	pipefd[1] = current_fd[1];
	descriptors = pcb->descriptors;
	
	return 0;
}

#define	F_DUPFD		0		/* duplicate file descriptor */
#define	F_GETFD		1		/* get file descriptor flags */
#define	F_SETFD		2		/* set file descriptor flags */
#define	F_GETFL		3		/* get file status flags */
#define	F_SETFL		4		/* set file status flags */
#define	F_GETOWN	5		/* get SIGIO/SIGURG proc/pgrp */
#define F_SETOWN	6		/* set SIGIO/SIGURG proc/pgrp */
#define	F_GETLK		7		/* get record locking information */
#define	F_SETLK		8		/* set record locking information */
#define	F_SETLKW	9		/* F_SETLK; wait if blocked */
#define	F_RGETLK 	10		/* Test a remote lock to see if it is blocked */
#define	F_RSETLK 	11		/* Set or unlock a remote lock */
#define	F_CNVT 		12		/* Convert a fhandle to an open fd */
#define	F_RSETLKW 	13		/* Set or Clear remote record-lock(Blocking) */
#define	F_DUPFD_CLOEXEC	14	/* As F_DUPFD, but set close-on-exec flag */

// Extensions
int fcntl(uint32_t fd, int32_t cmd, ...) {
	LOG_DEBUG_INFO_STR("(%d, %d, %d)", fd, cmd, (uint32_t)*(&cmd + 1));

	uint32_t* esp = ((uint32_t*)&cmd) + 1;
	switch (cmd) {
		case F_DUPFD:
			return dup2_greater(fd, *esp);
		case F_GETFD:
			// TODO: implement CLOSE_ON_EXEC
			return 0;
		case F_SETFD:
			// TODO: implement CLOSE_ON_EXEC
			return 0;
		case F_GETFL:
			if (!descriptors[fd]) {
                return -ENOENT;
			}
			return descriptors[fd]->mode;
		case F_SETFL:
			return 0;
		case F_DUPFD_CLOEXEC:
			// TODO: implement CLOSE_ON_EXEC
			return dup2_greater(fd, *esp);
		default:
#if DEBUG
			blue_screen("Unimplemented fctnl %d", cmd);
#endif
			return -1;
	}
	return 0;
}
