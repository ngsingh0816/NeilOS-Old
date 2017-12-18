//
//  ext2.c
//  NeilOS
//
//  Created by Neil Singh on 12/30/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#include "ext2.h"
#include <drivers/ATA/ata.h>
#include <drivers/filesystem/path.h>
#include <memory/allocation/heap.h>
#include "inode.h"
#include "block.h"
#include <common/time.h>
#include <program/task.h>

// Superblock cache
ext_superblock_t superblock;
disk_info_t fs;

// Internal functions
uint32_t ext2_allocate_block();
bool ext2_allocate_indirect_block(uint32_t first_block_id, uint32_t* output_block_id);
uint64_t ext2_truncate_inode_zero(ext_inode_t* inode, uint64_t size, bool fill);

// Initialize the EXT2 filesytem
bool ext2_init(disk_info_t* disk) {
	// Load the superblock information
	ata_partition_lock(disk);
	ata_partition_llseek(disk, uint64_make(0, SUPERBLOCK_ADDRESS), SEEK_SET);
	ata_partition_read(disk, &superblock, sizeof(ext_superblock_t));
	ata_partition_unlock(disk);
	
	if (superblock.magic != EXT2_MAGIC)
		return false;
	
	// Cache the disk info
	fs = *disk;
	
	// TODO: optimize the ata_partition to take a variable block_size
		
	return true;
}

// Return the ext2 file system device
disk_info_t* ext2_fs() {
	return &fs;
}

// The EXT2 superblock
ext_superblock_t* ext2_superblock() {
	return &superblock;
}

// Set the superblock info
void ext2_set_superblock() {
	// Modify this superblock
	ata_partition_lock(&fs);
	ata_partition_llseek(&fs, uint64_make(0, SUPERBLOCK_ADDRESS), SEEK_SET);
	ata_partition_write(&fs, &superblock, sizeof(ext_superblock_t));
	ata_partition_unlock(&fs);
	
	uint16_t backup = superblock.block_group_nr;
	
	// Backup blocks are located at groups 1, and then powers of 3, 5, and 7
	uint32_t num_groups = 1 + ((superblock.block_count - 1) / superblock.blocks_per_group);
	uint32_t group;
	for (group = 1; group < num_groups; group++) {
		if (!(group == 1 || (group % 3) == 0 || (group % 5) == 0 || (group % 7) == 0))
			continue;
		
		// Update the backup
		uint64_t addr = uint64_shl(uint64_make(0, superblock.first_data_block + group * superblock.blocks_per_group),
						  EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size);
		superblock.block_group_nr = group;
		ata_partition_lock(&fs);
		ata_partition_llseek(&fs, addr, SEEK_SET);
		ata_partition_write(&fs, &superblock, sizeof(ext_superblock_t));
		ata_partition_unlock(&fs);
	}
	
	superblock.block_group_nr = backup;
}

// Gets the block size
uint32_t ext2_get_block_size() {
	return 1 << (EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size);
}

/* Both */

// Return the inode of a particular filesystem entry
ext_inode_t ext2_open(const char* path) {
	// Loop through each path component and find the relative inode
	ext_inode_t inode = ext_inode_create(EXT2_INODE_ROOT);
	uint32_t path_length = 0;
	char* component = (char*)kmalloc(strlen(path) + 1);
	if (!component)
		return ext_inode_create(EXT2_INODE_INVALID);
	
	while ((path = path_get_component(&path[path_length], 0, &path_length)) != NULL) {
		// Copy the string over
		component[path_length] = 0;
		memcpy(component, path, path_length);
		
		// Get the next inode
		inode = ext2_get_relative_inode(component, &inode);
		
		// If we can't find the next inode, return error
		if (inode.inode == EXT2_INODE_INVALID) {
			kfree(component);
			return ext_inode_create(EXT2_INODE_INVALID);
		}
	}
	kfree(component);
	
	// Update the access time of this inode
	inode.info.atime = get_current_time().val;
	ext2_set_inode_info(inode.inode, &inode.info);

	// Return the inode found
	return inode;
}

// Create an inode at a particular path (creates a new file no matter what,
// so only call this when the file doesn't exist)
ext_inode_t ext2_create(ext_inode_t* parent, const char* name, unsigned int mode) {
	bool directory = (mode & FILE_TYPE_DIRECTORY) != 0;
	// Allocate a new inode
	uint32_t node = ext2_allocate_inode(directory);
	if (node == EXT2_INODE_INVALID)
		return ext_inode_create(EXT2_INODE_INVALID);
	
	ext_inode_t ret;
	memset(&ret, 0, sizeof(ext_inode_t));
	ret.inode = node;
	if (mode & FILE_TYPE_DIRECTORY)
		ret.info.mode |= EXT2_INODE_MODE_DIR;
	else if (mode & FILE_TYPE_PIPE)
		ret.info.mode |= EXT2_INODE_MODE_FIFO;
	else
		ret.info.mode |= EXT2_INODE_MODE_REG;
	ret.info.mode |= EXT2_INODE_MODE_ATTR_ALL;
	
	// Update the times of this inode
	ret.info.ctime = get_current_time().val;
	ret.info.atime = ret.info.ctime;
	ret.info.mtime = ret.info.ctime;
	
	// Write this inode data
	ext2_set_inode_info(ret.inode, &ret.info);
	
	// Link in the hard links (., ..) if needed
	if (directory) {
		if (!ext2_link_inode(&ret, &ret, ".")) {
			ext2_dealloc_inode(&ret);
			return ext_inode_create(EXT2_INODE_INVALID);
		}
		
		if (!ext2_link_inode(&ret, parent, "..")) {
			ext2_unlink(&ret, ".");
			ext2_dealloc_inode(&ret);
			return ext_inode_create(EXT2_INODE_INVALID);
		}
	}
	
	// Link it in
	if (!ext2_link_inode(parent, &ret, name)) {
		if (directory) {
			ext2_unlink(&ret, ".");
			ext2_unlink(&ret, "..");
		}
		ext2_dealloc_inode(&ret);
		return ext_inode_create(EXT2_INODE_INVALID);
	}
	
	return ret;
}

// Delete an inode at a particular path
bool ext2_delete(ext_inode_t* parent, const char* name) {
	ext_inode_t inode = ext2_get_relative_inode(name, parent);
	if (inode.inode == EXT2_INODE_INVALID || inode.inode == EXT2_INODE_ROOT)
		return false;
	
	// If this is our last hard link, also delete the data
	uint32_t num_links = ext2_inode_is_directory(&inode) ? 2 : 1;
	if (inode.info.link_count != num_links) {
		// Otherwise, just unlink it
		if (!ext2_unlink(parent, name)) {
			// Undo
			ext2_link_inode(parent, &inode, name);
			return false;
		}
		
		return true;
	}
	
	// Truncate to 0
	if (!uint64_equal(ext2_truncate_inode(&inode, uint64_make(0, 0)), uint64_make(0, 0)))
			return false;
	
	// Unlink it
	if (!ext2_unlink(parent, name)) {
		// Undo
		ext2_link_inode(parent, &inode, name);
		return false;
	}
	
	// Delete the inode
	return ext2_dealloc_inode(&inode);
}

// Unlink (remove) an inode (If this is a directory, it must be empty or space will be leaked)
bool ext2_unlink(ext_inode_t* parent, const char* name) {
	ext_inode_t inode = ext2_get_relative_inode(name, parent);

	if (!ext2_unlink_inode(parent, name))
		return false;
	
	// Update modified times
	parent->info.mtime = get_current_time().val;
	ext2_set_inode_info(parent->inode, &parent->info);
	
	// Decrease the link count for this inode
	inode.info.link_count--;
	ext2_set_inode_info(inode.inode, &inode.info);
	
	return true;
}

// Hard link an inode
bool ext2_link(ext_inode_t* inode, ext_inode_t* link_parent, const char* link_name) {
	// Get the inode
	if (inode->inode == EXT2_INODE_INVALID)
		return false;
	
	// Link it in
	if (!ext2_link_inode(link_parent, inode, link_name))
		return false;
	
	// Update modified times
	link_parent->info.mtime = get_current_time().val;
	ext2_set_inode_info(link_parent->inode, &link_parent->info);
	
	return true;
}

/* Files */

// Read an inode from a specific offset (returns bytes read and does not contain file length overflow checking)
uint32_t ext2_read_data(ext_inode_t* inode, uint64_t offset, void* buffer, uint32_t length) {
	if (inode->inode == EXT2_INODE_INVALID)
		return -1;
	
	pcb_t* pcb = current_pcb;
	
	// Calculate the starting and ending blocks
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size);
	uint32_t start_block = uint64_shr(offset, EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size).low;
	uint32_t end_block = uint64_shr(uint64_add(offset, uint64_make(0, length)),
									EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size).low;
	
	// Loop over every block
	uint32_t z;
	uint32_t copy_pos = 0;
	for (z = start_block; z <= end_block; z++) {
		uint32_t block_id = ext2_get_block_id_at_index(inode, z);
		
		// Seek to the right position
		uint64_t addr = uint64_shl(uint64_make(0, block_id),
								   EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size);
		ata_partition_lock(&fs);
		ata_partition_llseek(&fs, addr, SEEK_SET);
		
		// Only copy what we need to
		uint32_t size = block_size;
		if (z == start_block) {
			uint32_t copy_offset = offset.low % block_size;
			size -= copy_offset;
			
			ata_partition_llseek(&fs, uint64_make(0, copy_offset), SEEK_CUR);
		}
		if (z == end_block) {
			size = length - copy_pos;
			if (size > block_size) {
				// Something went wrong
				ata_partition_unlock(&fs);
				return -1;
			}
		}
		
		// Don't go for zero length blocks
		if (size == 0) {
			ata_partition_unlock(&fs);
			break;
		}
		
		// Copy the data over
		copy_pos += ata_partition_read(&fs, buffer + copy_pos, size);
		
		ata_partition_unlock(&fs);
		
		if (pcb && signal_pending(pcb))
			break;
	}
	
	// Return bytes read
	return copy_pos;
}

// Write data to an inode from a specific offset and length.
// If the offset is out of file range, sufficient space will try to be allocated towards the file.
uint32_t ext2_write_data(ext_inode_t* inode, uint64_t offset, const void* buffer, uint32_t length) {
	if (inode->inode == EXT2_INODE_INVALID)
		return -1;
	
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size);
	// Check if we need to allocate any space
	uint64_t end_pos = uint64_add(offset, uint64_make(0, length));
	uint64_t space_allocated = uint64_shl(uint64_make(0, inode->info.num_blocks), EXT2_INODE_BLOCK_COUNT_SIZE);
	// Try and allocate more space if needed
	if (uint64_greater(end_pos, space_allocated)) {
		if (!uint64_equal(end_pos, ext2_truncate_inode_zero(inode, end_pos, false)))
			return -1;
	}
	
	// Calculate the starting and ending blocks
	uint32_t start_block = uint64_shr(offset, EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size).low;
	uint32_t end_block = uint64_shr(end_pos, EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size).low;
	
	// Loop over every block
	uint32_t z;
	uint32_t copy_pos = 0;
	for (z = start_block; z <= end_block; z++) {
		uint32_t block_id = ext2_get_block_id_at_index(inode, z);
		
		// Seek to the right position
		uint64_t addr = uint64_shl(uint64_make(0, block_id),
								   EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size);
		ata_partition_lock(&fs);
		ata_partition_llseek(&fs, addr, SEEK_SET);
		
		// Only copy what we need to
		uint32_t size = block_size;
		if (z == start_block) {
			uint32_t copy_offset = offset.low % block_size;
			size -= copy_offset;
			
			ata_partition_llseek(&fs, uint64_make(0, copy_offset), SEEK_CUR);
		}
		if (z == end_block) {
			size = length - copy_pos;
			if (size > block_size) {
				// Something went wrong
				ata_partition_unlock(&fs);
				return -1;
			}
		}
		
		// Don't go for zero length blocks
		if (size == 0) {
			ata_partition_unlock(&fs);
			break;
		}
		
		// Copy the data over
		copy_pos += ata_partition_write(&fs, buffer + copy_pos, size);
		
		ata_partition_unlock(&fs);
	}
	
	// Update the file size if needed
	uint64_t file_size = uint64_make(inode->info.size_high, inode->info.size);
	uint64_t true_size = uint64_add(offset, uint64_make(0, copy_pos));
	if (uint64_less(file_size, true_size)) {
		inode->info.size_high = true_size.high;
		inode->info.size = true_size.low;
	}
	// Update inode
	inode->info.mtime = get_current_time().val;
	ext2_set_inode_info(inode->inode, &inode->info);

	
	// Return bytes read
	return copy_pos;
}

// Change the size of a file (if increasing the size, optionally, fill it with 0's).
// Returns the new size of the file
uint64_t ext2_truncate_inode_zero(ext_inode_t* inode, uint64_t size, bool fill) {
	uint64_t space_allocated = uint64_shl(uint64_make(0, inode->info.num_blocks), EXT2_INODE_BLOCK_COUNT_SIZE);
	uint64_t file_size = uint64_make(inode->info.size_high, inode->info.size);
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size);
	
	// Increase the size
	if (uint64_greater(size, file_size)) {
		if (uint64_greater(size, space_allocated)) {
			// Find out how many blocks we need to allocate
			uint32_t num_blocks = uint64_shr(uint64_sub(size, uint64_inc(space_allocated)),
											 EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size).low + 1;
			uint32_t block_index = uint64_shr(space_allocated, EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size).low;
			while (num_blocks != 0) {
				uint32_t new_block = ext2_allocate_block();
				if (new_block == EXT2_BLOCK_ID_INVALID)
					break;
				
				// Set the new block
				if (!ext2_set_block_id_at_index(inode, block_index++, new_block))
					break;
				inode->info.num_blocks += block_size >> EXT2_INODE_BLOCK_COUNT_SIZE;
				num_blocks--;
			}
			
			if (num_blocks != 0)
				size = uint64_shl(uint64_make(0, inode->info.num_blocks), EXT2_INODE_BLOCK_COUNT_SIZE);
		}
		
		// Save the new inode data
		inode->info.size_high = size.high;
		inode->info.size = size.low;
		inode->info.mtime = get_current_time().val;
		ext2_set_inode_info(inode->inode, &inode->info);
		
		// Write zero's to this new space
		if (fill) {
			uint64_t diff = uint64_sub(size, file_size);
			uint8_t* buffer = (uint8_t*)kmalloc(block_size);
			if (!buffer)
				return file_size;
			memset(buffer, 0, block_size);
			// Write in block chunks
			while (!uint64_equal(diff, uint64_make(0, 0))) {
				uint32_t write_size = uint64_greater(diff, uint64_make(0, block_size)) ? block_size : diff.low;
				write_size = ext2_write_data(inode, file_size, buffer, write_size);
				diff = uint64_sub(diff, uint64_make(0, write_size));
				file_size = uint64_add(file_size, uint64_make(0, write_size));
			}
			kfree(buffer);
		}
	} else if (uint64_less(size, file_size)) {		// Decrease the size
		// Figure out how many blocks we want to dealloc
		uint32_t num_blocks = uint64_shr(uint64_sub(space_allocated, size),
										 EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size).low;
		uint32_t block_index = uint64_shr(space_allocated, EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size).low;
		while (num_blocks != 0) {
			block_index--;
			
			// Dealloc the block
			uint32_t block_id = ext2_get_block_id_at_index(inode, block_index);
			if (!ext2_set_block_id_at_index(inode, block_index, EXT2_BLOCK_ID_INVALID))
				break;
			
			inode->info.num_blocks -= block_size >> EXT2_INODE_BLOCK_COUNT_SIZE;
			
			if (!ext2_dealloc_block(block_id))
				break;
			
			num_blocks--;
		}
		
		// Correct the size if we have to
		if (num_blocks != 0)
			size = uint64_shl(uint64_make(0, inode->info.num_blocks), EXT2_INODE_BLOCK_COUNT_SIZE);
		
		// Save the new inode data
		inode->info.size_high = size.high;
		inode->info.size = size.low;
		inode->info.mtime = get_current_time().val;
		ext2_set_inode_info(inode->inode, &inode->info);
	}
	
	return size;
}

// Change the size of a file (if increasing the size, fill it with 0's).
// Returns the new size of the file
uint64_t ext2_truncate_inode(ext_inode_t* inode, uint64_t size) {
	return ext2_truncate_inode_zero(inode, size, true);
}

/* Directories */

// Read a directory inode from a specific offset (returns the byte address of the next directory entry
// or 0 if none exists)
uint64_t ext2_read_directory(ext_inode_t* inode, uint64_t offset, void* buffer, uint32_t length,
							 uint32_t* length_out, ext_dentry_t* dentry_out) {
	if (!ext2_inode_is_directory(inode))
		return uint64_make(0, 0);
	
	// Read the dentry at the offset
	ext_dentry_t dentry = { 0 };
	ata_partition_lock(&fs);
	ata_partition_llseek(&fs, offset, SEEK_SET);
	ata_partition_read(&fs, &dentry, sizeof(ext_dentry_t));
	ata_partition_unlock(&fs);
	
	// Check if this is a valid entry
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + superblock.log_block_size);
	// Don't copy over anything if the inode doesn't exist
	if (dentry.inode == 0)
		dentry.name_len = 0;
	
	// Copy over the name
	uint32_t copy_size = (length < dentry.name_len) ? length : dentry.name_len;
	if (buffer) {
		memcpy(buffer, dentry.name, copy_size);
		if (length > dentry.name_len)
			((uint8_t*)buffer)[dentry.name_len] = 0;
	}
	if (length_out)
		*length_out = copy_size;
	
	if (dentry_out)
		*dentry_out = dentry;
	
	// Return the address of the next
	offset = uint64_add(offset, uint64_make(0, dentry.rec_len));
	if ((offset.low % block_size) == 0)
		return uint64_make(0, 0);
	
	return offset;
}

// Helper function to get parent inode of a path
ext_inode_t ext2_get_parent_inode(const char* filename) {
	ext_inode_t inode;
	memset(&inode, 0, sizeof(ext_inode_t));
	inode.inode = EXT2_INODE_INVALID;
	
	// Find the parent path
	uint32_t parent_len = 0;
	const char* parent_path = path_get_parent(filename, &parent_len);
	char* pp = kmalloc(parent_len + 1);
	if (!pp)
		return inode;
	memcpy(pp, parent_path, parent_len);
	pp[parent_len] = 0;
	
	// Find the parent inode
	inode = ext2_open(pp);
	kfree(pp);
	
	return inode;
}

// Create a ext_inode_t structure
ext_inode_t ext_inode_create(uint32_t inode) {
	ext_inode_t ret;
	memset(&ret, 0, sizeof(ext_inode_t));
	ret.inode = inode;
	if (ret.inode != EXT2_INODE_INVALID)
		ret.info = ext2_get_inode_info(inode);
	
	return ret;
}

// Free an inode's interal structure
void ext_free_inode(ext_inode_t* inode) {
	if (inode->cached_block)
		kfree(inode->cached_block);
}
