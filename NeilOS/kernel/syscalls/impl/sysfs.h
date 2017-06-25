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

// Make a directory
uint32_t mkdir(const char* name);

// Hard link a file
uint32_t link(const char* filename, const char* new_name);

// Unlink a file (note: deletes the file if it is the last remaining link)
uint32_t unlink(const char* filename);

#endif /* SYSFS_H */
