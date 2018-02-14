//
//  inode.c
//  NeilOS
//
//  Created by Neil Singh on 1/7/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "inode.h"
#include <drivers/ATA/ata.h>
#include <memory/allocation/heap.h>
#include "ext2.h"
#include "block.h"

// Get information about an inode
ext_inode_info_t ext2_get_inode_info(uint32_t inode) {
	ext_inode_info_t info;
	
	// Find which block group the inode resides in
	uint32_t block_group = (inode - 1) / ext2_superblock()->inodes_per_group;
	uint32_t local_inode = (inode - 1) % ext2_superblock()->inodes_per_group;
	
	// Read the inode relative to its inode table
	ext_block_group_descriptor_t group_info = ext2_get_block_group_info(block_group);
	uint64_t addr = uint64_shl(uint64_make(0, group_info.inode_table),
							   EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	addr = uint64_add(addr, uint64_make(0, ext2_superblock()->inode_size * local_inode));
	
	// Read the info
	ata_partition_lock(ext2_fs());
	ata_partition_llseek(ext2_fs(), addr, SEEK_SET);
	ata_partition_read(ext2_fs(), &info, sizeof(ext_inode_info_t));
	ata_partition_unlock(ext2_fs());
	
	return info;
}

// Set information about an inode
void ext2_set_inode_info(uint32_t inode, ext_inode_info_t* data) {
	// Find which block group the inode resides in
	uint32_t block_group = (inode - 1) / ext2_superblock()->inodes_per_group;
	uint32_t local_inode = (inode - 1) % ext2_superblock()->inodes_per_group;
	
	// Read the inode relative to its inode table
	ext_block_group_descriptor_t group_info = ext2_get_block_group_info(block_group);
	uint64_t addr = uint64_shl(uint64_make(0, group_info.inode_table),
							   EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	addr = uint64_add(addr, uint64_make(0, ext2_superblock()->inode_size * local_inode));
	
	// Read the info
	ata_partition_lock(ext2_fs());
	ata_partition_llseek(ext2_fs(), addr, SEEK_SET);
	ata_partition_write(ext2_fs(), data, sizeof(ext_inode_info_t));
	ata_partition_unlock(ext2_fs());
}

// Returns true if the inode is a directory inode
bool ext2_inode_is_directory(ext_inode_t* inode) {
	return ((inode->info.mode & EXT2_INODE_MODE_DIR) != 0);
}

// Allocate a new inode for use
uint32_t ext2_allocate_inode(bool directory) {
	// We can't allocate any if no inodes are free
	if (ext2_superblock()->free_inode_count == 0)
		return EXT2_INODE_INVALID;
	
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	uint32_t* bitmap = (uint32_t*)kmalloc(block_size);
	if (!bitmap)
		return EXT2_INODE_INVALID;
	
	// Find the first free inode by looping over all the descriptors
	uint32_t group;
	uint32_t num_groups = 1 + ((ext2_superblock()->inode_count - 1) / ext2_superblock()->inodes_per_group);
	uint32_t inodes_per_bitmap_group = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size + 3);
		// block_size * 8
	uint32_t bitmap_blocks_per_group = ((ext2_superblock()->inodes_per_group - 1) >>
										(EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size + 3)) + 1;
		// (blocks per group - 1) / (block size * 8) + 1
	for (group = 0; group < num_groups; group++) {
		ext_block_group_descriptor_t desc = ext2_get_block_group_info(group);
		if (desc.free_inodes == 0)
			continue;
		
		// Search for the block in this group that is free
		uint32_t bitmap_block;
		for (bitmap_block = desc.inode_bitmap; bitmap_block < desc.inode_bitmap + bitmap_blocks_per_group; bitmap_block++) {
			// Read the bitmap
			ata_partition_lock(ext2_fs());
			ata_partition_llseek(ext2_fs(), ext2_get_block_address(bitmap_block), SEEK_SET);
			if (ata_partition_read(ext2_fs(), bitmap, block_size) == -1) {
				// Error
				ata_partition_unlock(ext2_fs());
				kfree(bitmap);
				return EXT2_INODE_INVALID;
			}
			ata_partition_unlock(ext2_fs());
			
			// Check each block in the bitmap
			uint32_t blocks;
			for (blocks = 0; blocks < ext2_superblock()->inodes_per_group / (sizeof(uint32_t) * 8); blocks++) {
				// If these inodes are taken, don't consider
				if (bitmap[blocks] == (uint32_t)-1)
					continue;
				
				// Find the first free inode
				uint32_t z;
				for (z = 0; z < sizeof(uint32_t) * 8; z++) {
					if ((bitmap[blocks] >> z) & 0x1)
						continue;
					
					// Get which inode this corresponds to
					uint32_t ret = ext2_superblock()->inodes_per_group * group +
						(bitmap_block - desc.inode_bitmap) * inodes_per_bitmap_group +
						blocks * sizeof(uint32_t) * 8 + z + 1;
					
					// Mark this inode as used
					bitmap[blocks] |= 1 << z;
					ata_partition_lock(ext2_fs());
					ata_partition_llseek(ext2_fs(), ext2_get_block_address(bitmap_block), SEEK_SET);
					ata_partition_write(ext2_fs(), bitmap, block_size);
					ata_partition_unlock(ext2_fs());
					
					// Update the block group descriptor
					desc.free_inodes--;
					if (directory)
						desc.used_dirs_count++;
					ext2_set_block_group_info(group, &desc);
					
					// Update the super block
					ext2_superblock()->free_inode_count--;
					ext2_set_superblock();
					
					// We are done
					kfree(bitmap);
					
					return ret;
				}
			}
		}
	}
	
	// None could be found for some reason (superblock must be faulty)
	kfree(bitmap);
	return EXT2_INODE_INVALID;
}

// Dealloc an inode
bool ext2_dealloc_inode(ext_inode_t* inode) {
	// Get the block_group info
	uint32_t group = (inode->inode - 1) / ext2_superblock()->inodes_per_group;
	uint32_t inodes_per_bitmap_group = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size + 3);
		// block_size * 8
	
	// Allocate space for the bitmap
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	uint8_t* bitmap = (uint8_t*)kmalloc(block_size);
	if (!bitmap)
		return false;
	
	// Read the group
	ext_block_group_descriptor_t desc = ext2_get_block_group_info(group);
	
	// Figure out which block the block we want to free's bitmap is in
	uint32_t relative_block = (inode->inode - 1) % ext2_superblock()->inodes_per_group;
	uint32_t bitmap_block = desc.inode_bitmap + relative_block / inodes_per_bitmap_group;
	uint32_t relative_bitmap_block = relative_block % inodes_per_bitmap_group;
	
	// Read the bitmap
	ata_partition_lock(ext2_fs());
	ata_partition_llseek(ext2_fs(), ext2_get_block_address(bitmap_block), SEEK_SET);
	ata_partition_read(ext2_fs(), bitmap, block_size);
	
	// Set the specific block as free and write the data back
	bitmap[relative_bitmap_block / 8] &= ~(1 << (relative_bitmap_block % 8));
	ata_partition_llseek(ext2_fs(), ext2_get_block_address(bitmap_block), SEEK_SET);
	ata_partition_write(ext2_fs(), bitmap, block_size);
	ata_partition_unlock(ext2_fs());
	
	kfree(bitmap);
	
	// Update the block group info
	desc.free_inodes++;
	if (ext2_inode_is_directory(inode))
		desc.used_dirs_count--;
	
	ext2_set_block_group_info(group, &desc);
	
	// Update the superblock
	ext2_superblock()->free_inode_count++;
	ext2_set_superblock();
		
	return true;
}

// Helper function to align thigns to n bytes
uint32_t align_bytes(uint32_t bytes, uint32_t n) {
	uint32_t mod = (bytes % n);
	if (mod != 0)
		return bytes + n - mod;
	return bytes;
}

// Return the inode for the relative path in relation to the given inode.
// Returns EXT2_INODE_INVALID if no inode could be found
ext_inode_t ext2_get_relative_inode(const char* path, ext_inode_t* inode) {
	// Initialize a buffer to hold the block data (dentries cannot span a block)
	uint32_t block_id = 0;
	uint32_t index = 0;
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	int8_t* buffer = (int8_t*)kmalloc(block_size);
	if (!buffer)
		return ext_inode_create(EXT2_INODE_INVALID);
	uint32_t path_length = strlen(path);
	
	// Loop over all valid blocks to find a matching inode
	while ((block_id = ext2_get_block_id_at_index(inode, index++)) != EXT2_BLOCK_ID_INVALID) {
		// Read the block
		uint64_t addr = uint64_shl(uint64_make(0, block_id),
								   EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
		ata_partition_lock(ext2_fs());
		ata_partition_llseek(ext2_fs(), addr, SEEK_SET);
		ata_partition_read(ext2_fs(), buffer, block_size);
		ata_partition_unlock(ext2_fs());
		
		// Loop through all the possible dentries
		uint32_t pos;
		for (pos = 0; pos < block_size;) {
			ext_dentry_t* dentry = (ext_dentry_t*)&buffer[pos];
			
			// Check if we have a match
			if (path_length == dentry->name_len &&
				(strncmp(dentry->name, path, path_length) == 0)) {
				// We found a match
				ext_inode_t ret = ext_inode_create(dentry->inode);
				kfree(buffer);
				return ret;
			}
			
			// Move on to the next dentry
			pos += dentry->rec_len;
		}
	}
	kfree(buffer);
	
	// No matching inode was found
	return ext_inode_create(EXT2_INODE_INVALID);
}

// Link an inode entry in
bool ext2_link_inode(ext_inode_t* parent, ext_inode_t* inode, const char* name) {
	// Initialize a buffer to hold the block data (dentries cannot span a block)
	uint32_t block_id = 0;
	uint32_t index = 0;
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	int8_t* buffer = (int8_t*)kmalloc(block_size);
	if (!buffer)
		return false;
	// Align the length to 4 bytes
	uint32_t name_len = strlen(name);
	uint32_t entry_len = align_bytes(EXT2_BASE_DENTRY_SIZE + name_len, 4);
	
	// Loop over all valid blocks to find a matching inode
	uint64_t last_addr = uint64_make(0, 0);
	uint32_t last_pos = 0;
	
	// Loop all the blocks
	while ((block_id = ext2_get_block_id_at_index(parent, index++)) != EXT2_BLOCK_ID_INVALID) {
		// Read the block
		uint64_t addr = ext2_get_block_address(block_id);
		ata_partition_lock(ext2_fs());
		ata_partition_llseek(ext2_fs(), addr, SEEK_SET);
		ata_partition_read(ext2_fs(), buffer, block_size);
		ata_partition_unlock(ext2_fs());
		
		// Loop through all the possible dentries
		uint32_t pos;
		for (pos = 0; pos < block_size;) {
			ext_dentry_t* dentry = (ext_dentry_t*)&buffer[pos];
			
			// Calculate space used by this dentry
			uint32_t dentry_len = align_bytes(EXT2_BASE_DENTRY_SIZE + dentry->name_len, 4);
			bool empty_dentry = dentry->inode == 0;
			if (empty_dentry)
				dentry_len = 0;
			// Check if we have enough space on a free entry
			if (dentry_len + entry_len <= dentry->rec_len) {
				// We found the space, so update the info
				ext_dentry_t* new_dentry = (ext_dentry_t*)&buffer[pos + dentry_len];
				new_dentry->inode = inode->inode;
				new_dentry->rec_len = dentry->rec_len - dentry_len;
				new_dentry->name_len = name_len;
				new_dentry->file_type = ext2_inode_is_directory(inode) ? EXT2_FT_DIR : EXT2_FT_REG_FILE;
				memcpy(new_dentry->name, name, name_len);
				if (!empty_dentry)
					dentry->rec_len = dentry_len;

				// Write it
				ata_partition_lock(ext2_fs());
				ata_partition_llseek(ext2_fs(), addr, SEEK_SET);
				ata_partition_write(ext2_fs(), buffer, block_size);
				ata_partition_unlock(ext2_fs());
				kfree(buffer);
				
				// Update the linked inode
				inode->info.link_count++;
				ext2_set_inode_info(inode->inode, &inode->info);
				return true;
			}
			
			// Update the last address
			last_addr = addr;
			last_pos = pos;
			
			// Move on to the next dentry
			pos += dentry->rec_len;
		}
	}
	
	// We could not find an available spot so we must make a new block
	uint32_t new_block = ext2_allocate_block();
	if (!new_block) {
		kfree(buffer);
		return false;
	}
	
	// Update the last entry to point to the end of the block
	if (!uint64_equal(last_addr, uint64_make(0, 0))) {
		ext_dentry_t* dentry = (ext_dentry_t*)&buffer[last_pos];
		dentry->rec_len = block_size - last_pos;
		
		// Write it
		ata_partition_lock(ext2_fs());
		ata_partition_llseek(ext2_fs(), last_addr, SEEK_SET);
		ata_partition_write(ext2_fs(), buffer, block_size);
		ata_partition_unlock(ext2_fs());
	}
	
	// Construct a new dentry
	ext_dentry_t new_dentry;
	new_dentry.inode = inode->inode;
	new_dentry.rec_len = entry_len;
	new_dentry.name_len = name_len;
	new_dentry.file_type = ext2_inode_is_directory(inode) ? EXT2_FT_DIR : EXT2_FT_REG_FILE;
	memcpy(new_dentry.name, name, name_len);
	
	// Construct a follow up dentry
	ext_dentry_t new_dentry2;
	memset(&new_dentry2, 0, sizeof(ext_dentry_t));
	new_dentry2.rec_len = block_size - entry_len;
	
	// Write them in
	memset(buffer, 0, sizeof(ext_dentry_t) * 2);
	memcpy(buffer, &new_dentry, name_len + EXT2_BASE_DENTRY_SIZE);
	memcpy(&buffer[entry_len], &new_dentry2, sizeof(ext_dentry_t));
	uint64_t addr = ext2_get_block_address(new_block);
	ata_partition_lock(ext2_fs());
	ata_partition_llseek(ext2_fs(), addr, SEEK_SET);
	ata_partition_write(ext2_fs(), buffer, sizeof(ext_dentry_t) * 2);
	ata_partition_unlock(ext2_fs());
	
	// Update the blocks
	if (!ext2_set_block_id_at_index(parent, index - 1, new_block)) {
		ext2_dealloc_block(new_block);
		kfree(buffer);
		return false;
	}
	
	// Update the size
	parent->info.num_blocks += block_size >> EXT2_INODE_BLOCK_COUNT_SIZE;
	uint64_t size = uint64_shl(uint64_make(0, parent->info.num_blocks), EXT2_INODE_BLOCK_COUNT_SIZE);
	
	// Save the new inode data
	parent->info.size_high = size.high;
	parent->info.size = size.low;
	ext2_set_inode_info(parent->inode, &parent->info);
	
	// Update the linked inode
	inode->info.link_count++;
	ext2_set_inode_info(inode->inode, &inode->info);
	
	return true;
}

// Unlink an inode entry
bool ext2_unlink_inode(ext_inode_t* parent, const char* name) {
	// Initialize a buffer to hold the block data (dentries cannot span a block)
	uint32_t block_id = 0;
	uint32_t index = 0;
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	int8_t* buffer = (int8_t*)kmalloc(block_size);
	if (!buffer)
		return false;
	uint32_t name_len = strlen(name);
	
	// Loop over all valid blocks to find a matching inode
	while ((block_id = ext2_get_block_id_at_index(parent, index++)) != EXT2_BLOCK_ID_INVALID) {
		// Read the block
		uint64_t addr = ext2_get_block_address(block_id);
		ata_partition_lock(ext2_fs());
		ata_partition_llseek(ext2_fs(), addr, SEEK_SET);
		ata_partition_read(ext2_fs(), buffer, block_size);
		ata_partition_unlock(ext2_fs());
		
		// Loop through all the possible dentries
		uint32_t pos;
		for (pos = 0; pos < block_size;) {
			ext_dentry_t* dentry = (ext_dentry_t*)&buffer[pos];
			
			// Check if we have a match
			if (name_len == dentry->name_len &&
				(strncmp(dentry->name, name, name_len) == 0)) {
				// Check if this entire block is empty
				bool empty = true;
				uint32_t backup_pos = pos;
				for (pos = 0; pos < block_size;) {
					// Don't consider the one we are deleting
					if (pos == backup_pos)
						continue;
					
					// Check if it is a valid entry
					dentry = (ext_dentry_t*)&buffer[pos];
					if (dentry->inode != 0) {
						empty = false;
						break;
					}
					
					// Go to the next entry
					pos += dentry->rec_len;
				}
				
				// Delete this block if it is empty
				if (empty) {
					if (!ext2_set_block_id_at_index(parent, index - 1, EXT2_BLOCK_ID_INVALID))
						return false;
					return ext2_dealloc_block(block_id);
				}
				
				// Delete this entry
				dentry = (ext_dentry_t*)&buffer[backup_pos];
				uint32_t rec_len = dentry->rec_len;
				memset(dentry, 0, rec_len);
				dentry->rec_len = rec_len;
				
				// Write it
				ata_partition_lock(ext2_fs());
				ata_partition_llseek(ext2_fs(), addr, SEEK_SET);
				ata_partition_write(ext2_fs(), buffer, block_size);
				ata_partition_unlock(ext2_fs());
				
				return true;
			}
			
			// Move on to the next dentry
			pos += dentry->rec_len;
		}
	}
	kfree(buffer);
	
	// No matching inode was found
	return false;
}
