//
//  inode.h
//  NeilOS
//
//  Created by Neil Singh on 1/7/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef INODE_H
#define INODE_H

#include <common/types.h>
#include "defs.h"

// Get information about an inode
ext_inode_info_t ext2_get_inode_info(uint32_t inode);

// Set information about an inode
void ext2_set_inode_info(uint32_t inode, ext_inode_info_t* data);

// Returns true if the inode is a directory inode
bool ext2_inode_is_directory(ext_inode_t* inode);

// Allocate a new inode for use
uint32_t ext2_allocate_inode(bool directory);

// Dealloc an inode
bool ext2_dealloc_inode(ext_inode_t* inode);

// Return the inode for the relative path in relation to the given inode.
ext_inode_t ext2_get_relative_inode(const char* path, ext_inode_info_t* info);

// Link an inode entry in
bool ext2_link_inode(ext_inode_t* parent, ext_inode_t* inode, const char* name);

// Unlink an inode entry
bool ext2_unlink_inode(ext_inode_t* parent, const char* name);

#endif
