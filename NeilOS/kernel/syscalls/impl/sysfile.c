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
#include <drivers/ipc/pipe/pipe.h>
#include <drivers/ipc/pipe/fifo.h>
#include <common/log.h>
#include <syscalls/interrupt.h>
#include <drivers/devices/devices.h>

file_descriptor_t** descriptors = NULL;

file_descriptor_t* open_handle(const char* filename, uint32_t mode, uint32_t type) {
	file_descriptor_t* f = NULL;
	
	if (type != 0 && type <= NUM_DEVICE_TYPES) {
		f = device_open_functions[type-1](filename, mode);
		if (f)
			f->ref_count = 1;
		return f;
	}
	
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
	if (f)
		f->ref_count = 1;
	return f;
}

// Open a file descriptor
uint32_t open(const char* filename, uint32_t mode, uint32_t type) {
	LOG_DEBUG_INFO_STR("(%s, %d, %d)", filename, mode, type);
	
	// Find and open descriptor
	pcb_t* pcb = current_pcb;
	down(&pcb->descriptor_lock);
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
		up(&pcb->descriptor_lock);
        return -ENFILE;
	}
	
	char* path = path_absolute(filename, pcb->working_dir);

	pcb->descriptors[current_fd] = open_handle(path, mode, type);
	up(&pcb->descriptor_lock);
	kfree(path);
	
	if (!pcb->descriptors[current_fd])
        return -ENOENT;
	
	return current_fd;
}

// Read from a file descriptor
uint32_t read(uint32_t fd, void* buf, uint32_t nbytes) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x, 0x%x)", fd, buf, nbytes);
	
	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS)
		return -EBADF;
	if (!buf)
		return -EFAULT;
	
	down(&current_pcb->descriptor_lock);
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->read) {
		up(&current_pcb->descriptor_lock);
		return -EBADF;
	}
	
	file_descriptor_t* d = descriptors[fd];
	down(&d->lock);
	file_descriptor_retain(d);
	up(&current_pcb->descriptor_lock);
	
	if (!(d->mode & FILE_MODE_READ)) {
		if (!file_descriptor_release(d))
			up(&d->lock);
		return -EACCES;
	}
	
	// Call to the driver specific call
	uint32_t ret = d->read(d, buf, nbytes);
	if (!file_descriptor_release(d))
		up(&d->lock);
	
	return ret;
}

// Write to a file descriptor
uint32_t write(uint32_t fd, const void* buf, uint32_t nbytes) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x, %d)", fd, buf, nbytes);

    // Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS)
        return -EBADF;
	if (!buf)
		return -EFAULT;
	
	down(&current_pcb->descriptor_lock);
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->write) {
		up(&current_pcb->descriptor_lock);
		return -EBADF;
	}
	
	file_descriptor_t* d = descriptors[fd];
	down(&d->lock);
	file_descriptor_retain(d);
	up(&current_pcb->descriptor_lock);
	
	if (!((d->mode & FILE_MODE_WRITE) || (d->mode & FILE_MODE_APPEND))) {
		if (!file_descriptor_release(d))
			up(&d->lock);
		return -EACCES;
	}
	
	// Call to the driver specific call
	uint32_t ret = d->write(d, buf, nbytes);
	if (!file_descriptor_release(d))
		up(&d->lock);
	
	return ret;
}

// Seek to an offset
uint32_t llseek(uint32_t fd, uint32_t offset_high, uint32_t offset_low, int whence) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x, 0x%x, %d)", fd, offset_high, offset_low, whence);

	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS)
        return -EBADF;
	if (!(whence >= SEEK_SET && whence <= SEEK_END))
		return -EINVAL;
	
	down(&current_pcb->descriptor_lock);
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->llseek) {
		up(&current_pcb->descriptor_lock);
		return -EBADF;
	}
	
	file_descriptor_t* d = descriptors[fd];
	down(&d->lock);
	file_descriptor_retain(d);
	up(&current_pcb->descriptor_lock);
	
	// Call to the driver specific call
	// TODO: for now only return the lower 32 bits
	uint32_t ret = d->llseek(d, uint64_make(offset_high, offset_low), whence).low;
	if (!file_descriptor_release(d))
		up(&d->lock);
	
	return ret;
}

// Change the size of a file
uint32_t truncate(uint32_t fd, uint32_t length_high, uint32_t length_low) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x, 0x%x)", fd, length_high, length_low);

    // Check if the arguments are in range
    if (fd >= NUMBER_OF_DESCRIPTORS)
		return -EBADF;
	
	down(&current_pcb->descriptor_lock);
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->truncate) {
		up(&current_pcb->descriptor_lock);
		return -EBADF;
	}
	
	file_descriptor_t* d = descriptors[fd];
	down(&d->lock);
	file_descriptor_retain(d);
	up(&current_pcb->descriptor_lock);
	
	// Call to the driver specific call
	// TODO: for now just return the lower 32 bits
	uint32_t ret = d->truncate(d, uint64_make(length_high, length_low)).low;
	if (!file_descriptor_release(d))
		up(&d->lock);
	
	return ret;
}

// Get information about a file descriptor
uint32_t stat(uint32_t fd, sys_stat_type* data) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x)", fd, data);
	
    // Check if the arguments are in range
    if (fd >= NUMBER_OF_DESCRIPTORS)
		return -EBADF;
	
	down(&current_pcb->descriptor_lock);

	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->stat) {
		up(&current_pcb->descriptor_lock);
		return -EBADF;
	}
	
	file_descriptor_t* d = descriptors[fd];
	down(&d->lock);
	file_descriptor_retain(d);
	up(&current_pcb->descriptor_lock);
	
	uint32_t ret = d->stat(d, data);
	if (!file_descriptor_release(d))
		up(&d->lock);
	
	return ret;
}

// Close a file descriptor
uint32_t close(uint32_t fd) {
	LOG_DEBUG_INFO_STR("(%d)", fd);

	// Check if this descriptor is within the bounds
	if (fd >= NUMBER_OF_DESCRIPTORS)
		return -EBADF;
	
	// If we are trying to close an invalid descriptor, return failure
	down(&current_pcb->descriptor_lock);
	if (!descriptors[fd]) {
		up(&current_pcb->descriptor_lock);
		return -EBADF;
	}
	
	file_descriptor_t* d = descriptors[fd];
	down(&d->lock);
	d->closed = true;
	if (!file_descriptor_release(d))
		up(&d->lock);
	current_pcb->descriptors[fd] = NULL;
	
	up(&current_pcb->descriptor_lock);
	
	return 0;
}

// Is the file descriptor a terminal?
uint32_t isatty(uint32_t fd) {
	LOG_DEBUG_INFO_STR("(%d)", fd);

	// Check if this descriptor is within the bounds
	if (fd >= NUMBER_OF_DESCRIPTORS)
		return -EBADF;
	
	down(&current_pcb->descriptor_lock);
	// If we are trying to close an invalid descriptor, return failure
	if (!descriptors[fd]) {
		up(&current_pcb->descriptor_lock);
		return -EBADF;
	}
	
	file_descriptor_t* d = descriptors[fd];
	down(&d->lock);
	up(&current_pcb->descriptor_lock);
	
	uint32_t ret = (d->type == STANDARD_INOUT_TYPE);
	
	up(&d->lock);
	return ret;
}

// Duplicate a file descriptor
uint32_t dup(uint32_t fd) {
	LOG_DEBUG_INFO_STR("(%d)", fd);

	down(&current_pcb->descriptor_lock);
	
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
		up(&current_pcb->descriptor_lock);
		return -ENFILE;
	}
	
	// Point the new file handle to the old one
	descriptors[current_fd] = descriptors[fd];
	file_descriptor_retain(descriptors[current_fd]);
	
	up(&current_pcb->descriptor_lock);
	
	return current_fd;
}

// Duplicate a file descripter into the first available fd after the argument
uint32_t dup2_greater(int32_t fd, int32_t new_fd) {
	LOG_DEBUG_INFO_STR("(%d, %d)", fd, new_fd);
	
	down(&current_pcb->descriptor_lock);

	while (new_fd < NUMBER_OF_DESCRIPTORS && current_pcb->descriptors[new_fd] && new_fd >= 0)
		new_fd++;
	
    if (new_fd >= NUMBER_OF_DESCRIPTORS || new_fd < 0) {
		up(&current_pcb->descriptor_lock);
		return -EINVAL;
    }
	
	// Point the new file handle to the old one
	descriptors[new_fd] = descriptors[fd];
	file_descriptor_retain(descriptors[new_fd]);
	
	up(&current_pcb->descriptor_lock);
	
	return new_fd;
}

// Duplicate a file descriptor into the specific file descriptor
uint32_t dup2(uint32_t fd, uint32_t new_fd) {
	LOG_DEBUG_INFO_STR("(%d, %d)", fd, new_fd);

	// Check if this descriptor is within the bounds
	if (fd >= NUMBER_OF_DESCRIPTORS || new_fd >= NUMBER_OF_DESCRIPTORS)
		return -EBADF;
	
	pcb_t* pcb = current_pcb;
	
	down(&pcb->descriptor_lock);
	if (!descriptors[fd]) {
		up(&pcb->descriptor_lock);
		return -EBADF;
	}
	
	// Close the file handle if opened
	if (pcb->descriptors[new_fd]) {
		down(&pcb->descriptors[new_fd]->lock);
		pcb->descriptors[new_fd]->close(pcb->descriptors[new_fd]);
		if ((--pcb->descriptors[new_fd]->ref_count) == 0) {
			up(&pcb->descriptors[new_fd]->lock);
			kfree(pcb->descriptors[new_fd]);
		} else
			up(&pcb->descriptors[new_fd]->lock);
	}
	// Point the new file handle to the old one
	pcb->descriptors[new_fd] = pcb->descriptors[fd];
	file_descriptor_retain(pcb->descriptors[new_fd]);
	
	up(&pcb->descriptor_lock);
	
	return new_fd;
}

// Create a pipe
uint32_t pipe(uint32_t pipefd[2]) {
	LOG_DEBUG_INFO_STR("(%d, %d)", pipefd[0], pipefd[1]);
	
	if (!pipefd)
		return -EFAULT;

	// Find and open descriptor
	down(&current_pcb->descriptor_lock);
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
	if (current_fd[0] == -1 || current_fd[1] == -1) {
		up(&current_pcb->descriptor_lock);
		return -ENFILE;
	}
	
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
	
	up(&pcb->descriptor_lock);
	
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

// Helper to clear fds
void clear_fds(fd_set* read, fd_set* write, fd_set* except) {
	if (read)
		memset(read, 0, sizeof(fd_set));
	if (write)
		memset(write, 0, sizeof(fd_set));
	if (except)
		memset(except, 0, sizeof(fd_set));
}

// Wait for changes to file descriptors
int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout) {
	LOG_DEBUG_INFO_STR("(%d, 0x%x, 0x%x, 0x%x, 0x%x)", nfds, readfds, writefds, exceptfds, timeout);
	
	if ((!readfds && !writefds && !exceptfds && !timeout) || nfds == 0)
		return -EINVAL;
	
	if (nfds > NUMBER_OF_DESCRIPTORS || nfds < 0)
		nfds = NUMBER_OF_DESCRIPTORS;
	
	// Copy over fd sets and clear them
	fd_set rc;
	if (readfds)
		rc = *readfds;
	else
		memset(&rc, 0, sizeof(fd_set));
	fd_set wc;
	if (writefds)
		wc = *writefds;
	else
		memset(&wc, 0, sizeof(fd_set));
	clear_fds(readfds, writefds, exceptfds);
	
	down(&current_pcb->descriptor_lock);
	uint32_t num_desc = 0;
	for (int z = 0; z < nfds; z++) {
		int index = z / 32;
		int bit = z % 32;
		if ((rc.bits[index] >> bit) & 0x1 || (wc.bits[index] >> bit) & 0x1) {
			num_desc++;
			if (!descriptors[z]) {
				up(&current_pcb->descriptor_lock);
				return -EBADF;
			}
		}
	}
	file_descriptor_t* select_desc[num_desc];
	uint32_t desc_num = 0;
	for (int z = 0; z < nfds; z++) {
		int index = z / 32;
		int bit = z % 32;
		if ((rc.bits[index] >> bit) & 0x1 || (wc.bits[index] >> bit) & 0x1) {
			select_desc[desc_num++] = descriptors[z];
			file_descriptor_retain(descriptors[z]);
		}
	}
	up(&current_pcb->descriptor_lock);
	
	// Keep on checking until something works or the timeout has expired
	struct timeval end = { 0, 0 };
	if (timeout)
		end = time_add(time_get(), *timeout);
	int total = 0;
	while (!current_pcb->should_terminate) {
		if (signal_occurring(current_pcb)) {
			total = -EINTR;
			break;
		}
		// Check all file descriptors and see if they can read or write
		desc_num = 0;
		for (int z = 0; z < nfds; z++) {
			int index = z / 32;
			int bit = z % 32;
			bool rb = (rc.bits[index] >> bit) & 0x1;
			bool wb = (wc.bits[index] >> bit) & 0x1;
			if (!rb && !wb)
				continue;
			
			file_descriptor_t* d = select_desc[desc_num++];
			down(&d->lock);
			if (d->closed) {
				up(&d->lock);
				total = -EBADF;
				goto cleanup;
			}
			
			if (rb && d->can_read && d->can_read(d)) {
				readfds->bits[index] |= (1 << bit);
				total++;
			}
			if (wb && d->can_write && d->can_write(d)) {
				writefds->bits[index] |= (1 << bit);
				total++;
			}
			up(&d->lock);
		}
		if (total != 0)
			break;
		
		if (timeout && !time_less(time_get(), end))
			break;
		
		schedule();
	}
	
cleanup:
	for (uint32_t z = 0; z < num_desc; z++) {
		down(&select_desc[z]->lock);
		if (!file_descriptor_release(select_desc[z]))
			up(&select_desc[z]->lock);
	}
	
	return total;
}
