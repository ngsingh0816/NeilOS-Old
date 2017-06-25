//
//  sysfs.c
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "sysfs.h"
#include <drivers/filesystem/filesystem.h>

// Make a directory
uint32_t mkdir(const char* name) {
	return (fmkdir(name) ? 0 : -1);
}

// Hard link a file
uint32_t link(const char* filename, const char* new_name) {
	// Open the file and assert valid
	file_descriptor_t f;
	if (!fopen(filename, FILE_MODE_READ, &f))
		return -1;
	
	// Link and close
	bool ret = flink(&f, new_name);
	ret &= fclose(&f);
	
	return (ret ? 0 : -1);
}

// Unlink a file (note: deletes the file if it is the last remaining link)
uint32_t unlink(const char* filename) {
	// Open the file and assert valid
	file_descriptor_t f;
	if (!fopen(filename, FILE_MODE_READ | FILE_MODE_DELETE_ON_CLOSE, &f))
		return -1;
	
	// Close (and therefore delete) it
	return (fclose(&f) ? 0 : -1);
}
