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

// Make a directory
uint32_t mkdir(const char* name) {
	LOG_DEBUG_INFO_STR("(%s)", name);
	
	char* path = path_absolute(name, current_pcb->working_dir);

	uint32_t ret = (fmkdir(path) ? 0 : -1);
	kfree(path);
	
	return ret;
}

// Hard link a file
uint32_t link(const char* filename, const char* new_name) {
	LOG_DEBUG_INFO_STR("(%s, %s)", filename, new_name);
	
	char* path = path_absolute(filename, current_pcb->working_dir);
	char* new_path = path_absolute(new_name, current_pcb->working_dir);

	// Open the file and assert valid
	file_descriptor_t f;
	if (!fopen(path, FILE_MODE_READ, &f)) {
		kfree(path);
		kfree(new_path);
		return -1;
	}
	
	// Link and close
	bool ret = flink(&f, new_path);
	ret &= fclose(&f);
	
	kfree(path);
	kfree(new_path);
	
	return (ret ? 0 : -1);
}

uint32_t readdir(int fd, void* buf, int size, dirent_t* dirent) {
	return filesystem_readdir(fd, buf, size, dirent);
}

// Unlink a file (note: deletes the file if it is the last remaining link)
uint32_t unlink(const char* filename) {
	LOG_DEBUG_INFO_STR("(%s)", filename);
	
	char* path = path_absolute(filename, current_pcb->working_dir);

	// Open the file and assert valid
	file_descriptor_t f;
	if (!fopen(path, FILE_MODE_READ | FILE_MODE_DELETE_ON_CLOSE, &f)) {
		kfree(path);
		return -1;
	}
	kfree(path);
	
	// Close (and therefore delete) it
	return (fclose(&f) ? 0 : -1);
}

// Update a file's access and modification time
uint32_t utime(const char* filename, uint32_t* times) {
	LOG_DEBUG_INFO_STR("(%s, 0x%x)", filename, times);
	
	char* path = path_absolute(filename, current_pcb->working_dir);
	
	file_descriptor_t f;
	if (!fopen(path, FILE_MODE_READ, &f)) {
		kfree(path);
		return -1;
	}
	kfree(path);
	
	if (times)
		fsettime(&f, times[0], times[1]);
	else {
		uint32_t t = get_current_time().val;
		fsettime(&f, t, t);
	}
	fclose(&f);
	
	return 0;
}
