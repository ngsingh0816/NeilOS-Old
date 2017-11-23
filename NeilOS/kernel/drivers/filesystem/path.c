//
//  path.c
//  NeilOS
//
//  Created by Neil Singh on 1/3/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "path.h"
#include <common/lib.h>
#include <memory/allocation/heap.h>

// Get the specified component of the input path. The pointer returned is
// an indexed pointer of the input path. NULL is returned if no such component exists.
const char* path_get_component(const char* path, uint32_t component, uint32_t* length_out) {
	// Only deal with valid paths
	if (!path)
		return NULL;
	
	uint32_t length = strlen(path);
	
	// Ignore leading slashes
	uint32_t pos = 0;
	while (path[pos] == '/' && pos < length)
		pos++;
	
	// Loop through and find each '/' as a component
	uint32_t found_pos = pos;
	uint32_t component_count = 0;
	bool found = (component == 0);
	for (; pos < length; pos++) {
		// Check for a path divider
		if (path[pos] == '/' || ((pos == length - 1) && found)) {
			// Check if this is the component we want
			component_count++;
			uint32_t prev_pos = pos;
			
			// Skip all extra slashes
			while (path[pos] == '/' && pos < length)
				pos++;
			
			if (component_count == component) {
				// We have found the start
				found_pos = pos;
				found = true;
			} else if (component_count == component + 1) {
				// Return the correct component
				if (length_out)
					*length_out = prev_pos - found_pos + (path[prev_pos] != '/');
				return &path[found_pos];
			}
		}
	}
	
	// The specified component was not found
	return NULL;
}

// Get the last component of the input path. The pointer returned is
// an indexed pointer of the input path. NULL is returned if no such component exists.
const char* path_last_component(const char* path, uint32_t* length_out) {
	// Only deal with valid paths
	if (!path)
		return NULL;
	
	uint32_t length = strlen(path);
	
	// Ignore trailing slashes
	uint32_t pos = length - 1;
	while (path[pos] == '/' && pos != 0)
		pos--;
	
	// Find the previous slash
	uint32_t end_pos = pos + 1;
	while (path[pos] != '/' && pos != 0)
		pos--;
	
	// Ignore leading slash
	if (path[pos] == '/')
		pos++;
	
	// Return the path
	if (length_out)
		*length_out = end_pos - pos;
	return &path[pos];
}

// Get the parent directory (path - last component) of the input path. The pointer returned is
// an indexed pointer of the input path. NULL is returned if no such component exists.
const char* path_get_parent(const char* path, uint32_t* length_out) {
	const char* lc = path_last_component(path, NULL);
	*length_out = (uint32_t)((void*)lc - (void*)path);
	return path;
}

// Append a string to a path
char* path_append(const char* prefix, char* str) {
	const uint32_t prefix_len = strlen(prefix);
	uint32_t len = strlen(str);
	char* path = kmalloc(prefix_len + len + 1);
	if (!path)
		return NULL;
	memcpy(path, prefix, prefix_len);
	memcpy(&path[prefix_len], str, len);
	path[prefix_len + len] = 0;
	return path;
}
