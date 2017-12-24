//
//  devices.c
//  NeilOS
//
//  Created by Neil Singh on 9/5/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "devices.h"

#include "null.h"
#include "zero.h"

#include <drivers/ATA/ata.h>
#include <drivers/filesystem/filesystem.h>
#include <drivers/filesystem/path.h>
#include <drivers/rtc/rtc.h>
#include <drivers/terminal/terminal.h>
#include <memory/allocation/heap.h>

// Initialize the /dev directory
bool devices_init() {
	const char* prefix = "/dev/";
	// Remove the contents of /dev
	file_descriptor_t file;
	if (!fopen(prefix, FILE_MODE_READ, &file))
		return false;
	char* buffer = kmalloc(512);
	if (!buffer) {
		fclose(&file);
		return false;
	}
	uint32_t num = 	fseek(&file, uint64_make(0, 0), SEEK_END).low;
	fseek(&file, uint64_make(0, 2), SEEK_SET);
	while (num != 2) {
		fread(buffer, 512, 1, &file);
		char* path = path_append(prefix, buffer);
		if (!path) {
			fclose(&file);
			return false;
		}
		file_descriptor_t f;
		if (!fopen(path, FILE_MODE_READ | FILE_MODE_DELETE_ON_CLOSE, &f)) {
			kfree(path);
			return false;
		}
		fclose(&f);
		kfree(path);
		num = fseek(&file, uint64_make(0, 2), SEEK_SET).low;
	}
	kfree(buffer);
	fclose(&file);
	
	// Create default dev entries
	if (!device_file_add("stdin", terminal_open))
		return false;
	if (!device_file_add("stdout", terminal_open))
		return false;
	if (!device_file_add("stderr", terminal_open))
		return false;
	if (!device_file_add("tty", terminal_open))
		return false;
	/*if (!device_file_add("rtc", rtc_open))
		return false;*/
	if (!device_file_add("null", null_open))
		return false;
	if (!device_file_add("zero", zero_open))
		return false;
	// TODO: don't hardcode these in
	if (!device_file_add("disk0", ata_open))
		return false;
	if (!device_file_add("disk0s1", ata_open))
		return false;
	
	return true;
}
