//
//  path.h
//  NeilOS
//
//  Created by Neil Singh on 1/3/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef PATH_H
#define PATH_H

#include <common/types.h>

// Get the specified component of the input path. The pointer returned is
// an indexed pointer of the input path. NULL is returned if no such component exists.
const char* path_get_component(const char* path, uint32_t component, uint32_t* length_out);

// Get the last component of the input path. The pointer returned is
// an indexed pointer of the input path. NULL is returned if no such component exists.
const char* path_last_component(const char* path, uint32_t* length_out);

// Get the parent directory (path - last component) of the input path. The pointer returned is
// an indexed pointer of the input path. NULL is returned if no such component exists.
const char* path_get_parent(const char* path, uint32_t* length_out);

// Append a string to a path
char* path_append(const char* prefix, char* str);

// Get absolute path
char* path_absolute(const char* path, const char* wd);

// TODO: replace generic instances with this
// Copy a path
char* path_copy(const char* path);

#endif
