//
//  sysfs.c
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "sysfs.h"
#include <drivers/filesystem/filesystem.h>
#include <drivers/filesystem/path.h>
#include <program/task.h>
#include <common/log.h>
#include <syscalls/interrupt.h>

// Make a directory
uint32_t mkdir(const char* name) {
	LOG_DEBUG_INFO_STR("(%s)", name);
    
    if (!name)
		return -EFAULT;
	
	char* path = path_absolute(name, current_pcb->working_dir);

    uint32_t ret = fmkdir(path);
	kfree(path);
	
	return ret;
}

// Hard link a file
uint32_t link(const char* filename, const char* new_name) {
	LOG_DEBUG_INFO_STR("(%s, %s)", filename, new_name);
    
    if (!filename || !new_name)
		return -EFAULT;
	
	char* path = path_absolute(filename, current_pcb->working_dir);
	char* new_path = path_absolute(new_name, current_pcb->working_dir);

	// Open the file and assert valid
	file_descriptor_t f;
	if (!fopen(path, FILE_MODE_READ, &f)) {
		kfree(path);
		kfree(new_path);
		return -ENOENT;
	}
	
	// Link and close
	int ret = flink(&f, new_path);
    fclose(&f);
	
	kfree(path);
	kfree(new_path);
	
    return ret;
}

uint32_t readdir(uint32_t fd, void* buf, int size, dirent_t* dirent) {
	if (fd >= NUMBER_OF_DESCRIPTORS)
		return -EBADF;
	if (!descriptors[fd])
		return -EBADF;
	return filesystem_readdir(descriptors[fd], buf, size, dirent);
}

// Unlink a file or directory (note: deletes the file if it is the last remaining link)
uint32_t unlink(const char* filename, bool dir) {
	LOG_DEBUG_INFO_STR("(%s)", filename);
	
    if (!filename)
		return -EFAULT;
    
	char* path = path_absolute(filename, current_pcb->working_dir);
	
	// Verify it is a dir if needed
	if (dir && !fisdir(path)) {
		kfree(path);
        return -ENOTDIR;
	}

	int ret = fsunlink(path);
	kfree(path);
    return ret;
}

// Update a file's access and modification time
uint32_t utime(const char* filename, uint32_t* times) {
	LOG_DEBUG_INFO_STR("(%s, 0x%x)", filename, times);
    
    if (!filename)
		return -EFAULT;
	
	char* path = path_absolute(filename, current_pcb->working_dir);
	
	file_descriptor_t f;
	if (!fopen(path, FILE_MODE_READ, &f)) {
		kfree(path);
        return -ENOENT;
	}
	kfree(path);
	
	if (times)
		fsettime(&f, times[0], times[1]);
	else {
		uint32_t t = get_current_unix_time().val;
		fsettime(&f, t, t);
	}
	fclose(&f);
	
	return 0;
}
