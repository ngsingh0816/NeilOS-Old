//
//  block.h
//  NeilOS
//
//  Created by Neil Singh on 1/7/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef BLOCK_H
#define BLOCK_H

#include <common/types.h>
#include "defs.h"

// Get information about a block group
ext_block_group_descriptor_t ext2_get_block_group_info(uint32_t block_group);

// Set information about a block group
void ext2_set_block_group_info(uint32_t block_group, ext_block_group_descriptor_t* data);

// Return the nth block index in the block array for an inode.
uint32_t ext2_get_block_id_at_index(ext_inode_t* inode, uint32_t index);

// Set the nth block index in the block array for an inode.
bool ext2_set_block_id_at_index(ext_inode_t* inode, uint32_t index, uint32_t block_id);

// Get the byte address of a block
uint64_t ext2_get_block_address(uint32_t block_id);

// Allocate a new block for use in an inode.
uint32_t ext2_allocate_block();

// Mark a block as free for use
bool ext2_dealloc_block(uint32_t block_id);

#endif
