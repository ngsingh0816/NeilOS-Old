//
//  ext2.h
//  NeilOS
//
//  Created by Neil Singh on 12/30/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#ifndef EXT2_H
#define EXT2_H

#include <common/types.h>
#include <drivers/ATA/ata.h>
#include "defs.h"
#include "inode.h"
#include "block.h"

// Initialize the EXT2 filesytem
bool ext2_init(disk_info_t* disk);

// The EXT2 file system device
disk_info_t* ext2_fs();
// The EXT2 superblock
ext_superblock_t* ext2_superblock();
// Set the EXT2 superblock
void ext2_set_superblock();

// Gets the block size
uint32_t ext2_get_block_size();

/* Both */

// Return the inode of a particular filesystem entry
ext_inode_t ext2_open(const char* path);

// Create an inode at a particular path (creates a new file no matter what,
// so only call this when the file doesn't exist)
ext_inode_t ext2_create(ext_inode_t* parent, const char* name, unsigned int mode);

// Delete an inode at a particular path
bool ext2_delete(ext_inode_t* parent, const char* name);

// Unlink (remove) an inode. (If this is a directory, it must be empty or space will be leaked)
bool ext2_unlink(ext_inode_t* parent, const char* name);

// Hard link an inode
bool ext2_link(ext_inode_t* inode, ext_inode_t* new_parent, const char* new_name);

/* Files */

// Read an inode from a specific offset (returns bytes read and does not contain file length overflow checking)
uint32_t ext2_read_data(ext_inode_t* inode, uint64_t offset, void* buffer, uint32_t length);

// Write data to an inode from a specific offset and length.
// If the offset is out of file range, sufficient space will try to be allocated towards the file.
uint32_t ext2_write_data(ext_inode_t* inode, uint64_t offset, const void* buffer, uint32_t length);

// Change the size of a file (if increasing the size, fill it with 0's).
// Returns the new size of the file
uint64_t ext2_truncate_inode(ext_inode_t* inode, uint64_t size);

/* Directories */

// Read a directory inode from a specific offset (returns the byte address of the next directory entry
// or uint64_t(-1) if none exists) and places the name of the entry into the buffer
uint64_t ext2_read_directory(ext_inode_t* inode, uint64_t offset, void* buffer,
							 uint32_t length, uint32_t* length_out, ext_dentry_t* dentry_out);

// Helpers
ext_inode_t ext2_get_parent_inode(const char* filename);

// Create a ext_inode_t structure
ext_inode_t ext_inode_create(uint32_t inode);

// Free an inode's interal structure
void ext_free_inode(ext_inode_t* inode);

#endif
