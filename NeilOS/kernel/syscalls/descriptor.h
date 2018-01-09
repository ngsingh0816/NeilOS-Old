//
//  descriptor.h
//  NeilOS
//
//  Created by Neil Singh on 12/23/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <common/time.h>
#include <common/concurrency/semaphore.h>

// File types
#define FILE_FILE_TYPE			0
#define DIRECTORY_FILE_TYPE		1
#define STANDARD_INOUT_TYPE		2
#define RTC_FILE_TYPE			3
#define DISK_FILE_TYPE			4
#define MOUSE_FILE_TYPE			5
#define AUDIO_FILE_TYPE			6
#define SHARED_MEMORY_FILE_TYPE	7
#define MESSAGE_QUEUE_FILE_TYPE	8

// File Modes
#define FILE_MODE_READ				0x001
#define FILE_MODE_WRITE				0x002
#define FILE_MODE_READ_WRITE		(FILE_MODE_READ | FILE_MODE_WRITE)
#define FILE_MODE_APPEND			0x008
#define FILE_MODE_CREATE			0x200
#define	FILE_MODE_TRUNCATE			0x400
#define FILE_MODE_DELETE_ON_CLOSE	0x100
#define FILE_MODE_EXCLUSIVE			0x800
#define FILE_MODE_NONBLOCKING		0x4000

// Types of device (goes in mode)
#define		FILE_TYPE_DIRECTORY		0040000
#define		FILE_TYPE_CHARACTER		0020000
#define		FILE_TYPE_BLOCK			0060000
#define		FILE_TYPE_REGULAR		0100000
#define		FILE_TYPE_SYMBOLIC		0120000
#define		FILE_TYPE_SOCKET		0140000
#define		FILE_TYPE_PIPE			0010000
#define		FILE_TYPE_ALL			(~(0010000 - 1))

// For llseek
#define SEEK_SET		0
#define SEEK_CUR		1
#define SEEK_END		2

typedef struct {
	// From pusha
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
} __attribute__((packed)) context_state_t;

typedef struct {
	uint32_t dev_id;
	uint32_t inode;
	uint32_t mode;
	uint16_t num_links;
	uint32_t size;
	uint32_t block_size;
	uint32_t num_512_blocks;
	
	time_t atime;
	time_t mtime;
	time_t ctime;
} sys_stat_type;

typedef struct {
	time_t user_time;
	time_t system_time;
	time_t child_user_time;
	time_t child_process_time;
} sys_time_type;

typedef	struct {
	uint32_t bits[2];
} fd_set;

// File descriptor
typedef struct file_descriptor {
	char* filename;
	int32_t type;				// TODO: get rid of this
	uint32_t mode;				// Access modes
	uint8_t ref_count;			// Number of references to this descriptor
	
	// File dependent data
	void* info;
	
	// Pointers for the syscalls
	uint32_t (*read)(struct file_descriptor* f, void* buf, uint32_t nbytes);
	uint32_t (*write)(struct file_descriptor* f, const void* buf, uint32_t nbytes);
	uint64_t (*llseek)(struct file_descriptor* f, uint64_t offset, int whence);
	uint64_t (*truncate)(struct file_descriptor* f, uint64_t nsize);
	uint32_t (*stat)(struct file_descriptor* f, sys_stat_type* data);
	uint32_t (*ioctl)(struct file_descriptor* f, int request, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);
	bool (*can_read)(struct file_descriptor* f);
	bool (*can_write)(struct file_descriptor* f);
	struct file_descriptor* (*duplicate)(struct file_descriptor* f);
	uint32_t (*close)(struct file_descriptor* f);	// Open allocates a file descriptor, but the syscall close frees it
	
	mutex_t lock;
} file_descriptor_t;

// Avaiable descriptors
#define NUMBER_OF_DESCRIPTORS	64
// Current task's descriptors
extern file_descriptor_t** descriptors;

#endif
