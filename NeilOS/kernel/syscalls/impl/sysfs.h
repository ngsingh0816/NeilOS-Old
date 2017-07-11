//
//  sysfs.h
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef SYSFS_H
#define SYSFS_H

#include <common/types.h>

typedef struct
{
	uint32_t ino;
	uint32_t off;
	unsigned short int reclen;
	unsigned char type;
	char name[256];
} dirent_t;

// Make a directory
uint32_t mkdir(const char* name);

// Hard link a file
uint32_t link(const char* filename, const char* new_name);

// Unlink a file (note: deletes the file if it is the last remaining link)
uint32_t unlink(const char* filename);

// Read a directory entry
uint32_t readdir(int fd, void* buf, int size, dirent_t* dirent);

// Update a file's access and modification time
uint32_t utime(const char* filename, uint32_t* times);

#endif /* SYSFS_H */
