//
//  block.c
//  NeilOS
//
//  Created by Neil Singh on 1/7/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "block.h"
#include <drivers/ATA/ata.h>
#include <memory/allocation/heap.h>
#include "ext2.h"

// Internal methods
bool ext2_allocate_indirect_block(uint32_t first_block_id, uint32_t* output_block_id);

// Get information about a block group
ext_block_group_descriptor_t ext2_get_block_group_info(uint32_t block_group) {
	ext_block_group_descriptor_t info;
	
	// The block descriptors are in the block after the super block
	uint64_t addr = uint64_shl(uint64_make(0, ext2_superblock()->first_data_block + 1),
							   EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	// Offset to the correct block group description
	addr = uint64_add(addr, uint64_make(0, sizeof(ext_block_group_descriptor_t) * block_group));
	
	// Read the info
	ata_partition_lock(ext2_fs());
	ata_partition_llseek(ext2_fs(), addr, SEEK_SET);
	ata_partition_read(ext2_fs(), &info, sizeof(ext_block_group_descriptor_t));
	ata_partition_unlock(ext2_fs());
	
	return info;
}

// Set information about a block group
void ext2_set_block_group_info(uint32_t block_group, ext_block_group_descriptor_t* data) {
	// The block descriptors are in the block after the super block
	uint64_t addr = uint64_shl(uint64_make(0, ext2_superblock()->first_data_block + 1),
							   EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	// Offset to the correct block group description
	addr = uint64_add(addr, uint64_make(0, sizeof(ext_block_group_descriptor_t) * block_group));
	
	// Write the info
	ata_partition_lock(ext2_fs());
	ata_partition_llseek(ext2_fs(), addr, SEEK_SET);
	ata_partition_write(ext2_fs(), data, sizeof(ext_block_group_descriptor_t));
	ata_partition_unlock(ext2_fs());
	
	// Backup blocks are located at groups 1, and then powers of 3, 5, and 7
	uint32_t num_groups = 1 + ((ext2_superblock()->block_count - 1) / ext2_superblock()->blocks_per_group);
	uint32_t group;
	for (group = 1; group < num_groups; group++) {
		if (!(group == 1 || (group % 3) == 0 || (group % 5) == 0 || (group % 7) == 0))
			continue;
		
		// Update the backup
		addr = uint64_shl(uint64_make(0, ext2_superblock()->first_data_block + 1 + group * ext2_superblock()->blocks_per_group),
						  EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
		addr = uint64_add(addr, uint64_make(0, sizeof(ext_block_group_descriptor_t) * block_group));
		ata_partition_lock(ext2_fs());
		ata_partition_llseek(ext2_fs(), addr, SEEK_SET);
		ata_partition_write(ext2_fs(), data, sizeof(ext_block_group_descriptor_t));
		ata_partition_unlock(ext2_fs());
	}
}

// Return the nth block index in the block array for an inode.
// This makes it so you don't have to deal with indirect blocks
uint32_t ext2_get_block_id_at_index(ext_inode_info_t* inode, uint32_t index) {
	uint32_t total_blocks = EXT2_BUFFERS_NUM_DIRECT;
	// Direct blocks are easy
	if (index < total_blocks)
		return inode->blocks[index];
	
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	uint32_t num_entries = (block_size >> INDEX_SIZE_BITS);
	
	// Get the index ready
	index -= total_blocks - 1;
	total_blocks = 1;
	
	// Check for indirect blocks;
	uint32_t depth;
	for (depth = 1; depth <= 3; depth++) {
		index -= total_blocks;
		total_blocks *= num_entries;
		if (index < total_blocks) {
			// Read the index
			uint32_t z;
			uint32_t divisor = total_blocks / num_entries;
			uint32_t block = index;
			uint32_t read_block = inode->blocks[EXT2_BUFFERS_NUM_DIRECT + depth - 1];
			for (z = 0; z < depth; z++) {
				// Read the next block index
				uint64_t addr = ext2_get_block_address(read_block);
				addr = uint64_add(addr, uint64_shl(uint64_make(0, block / divisor), INDEX_SIZE_BITS));
				ata_partition_lock(ext2_fs());
				ata_partition_llseek(ext2_fs(), addr, SEEK_SET);
				ata_partition_read(ext2_fs(), &read_block, sizeof(uint32_t));
				ata_partition_unlock(ext2_fs());
				
				// Update to the next index
				block %= divisor;
				divisor /= num_entries;
			}
			
			return read_block;
		}
	}
	
	// No valid block found
	return EXT2_BLOCK_ID_INVALID;
}

// Set the nth block index in the block array for an inode.
// Note: this will allocate new blocks for indirect blocks.
// Note 2: this does not deallocate previous blocks in use
// Returns true upon success (false if no blocks could be allocated)
bool ext2_set_block_id_at_index(ext_inode_info_t* inode, uint32_t index, uint32_t block_id) {
	uint32_t total_blocks = EXT2_BUFFERS_NUM_DIRECT;
	// Direct blocks are easy
	if (index < total_blocks) {
		inode->blocks[index] = block_id;
		return true;
	}
	
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	uint32_t num_entries = (block_size >> INDEX_SIZE_BITS);
	uint32_t original_index = index;
	
	// Get the index ready
	index -= total_blocks - 1;
	total_blocks = 1;
	
	// Indirect blocks
	uint32_t depth;
	for (depth = 1; depth <= 3; depth++) {
		index -= total_blocks;
		total_blocks *= num_entries;
		if (index < total_blocks) {
			// Guarantee the blocks we need are allocated
			int z;
			uint32_t divisor = num_entries;
			uint32_t last_block = block_id;
			if (block_id != EXT2_BLOCK_ID_INVALID) {
				for (z = 0; z < depth; z++) {
					if ((index % divisor) == 0) {
						// Allocate the block
						if (!ext2_allocate_indirect_block(last_block, &last_block)) {
							// This call will free up any memory we may have allocated
							ext2_set_block_id_at_index(inode, original_index, EXT2_BLOCK_ID_INVALID);
							return false;
						}
					}
					
					// Go to one level higher of indirection
					divisor *= num_entries;
				}
			}
			
			// Assign the first new block if needed
			uint32_t prev = inode->blocks[EXT2_BUFFERS_NUM_DIRECT + depth - 1];
			if (index == 0) {
				inode->blocks[EXT2_BUFFERS_NUM_DIRECT + depth - 1] = last_block;
				if (block_id == EXT2_BLOCK_ID_INVALID)
					last_block = prev;
			}
			else
				last_block = inode->blocks[EXT2_BUFFERS_NUM_DIRECT + depth - 1];
			
			// Update the correct entry
			divisor = total_blocks / num_entries;
			for (z = 0; z < depth; z++) {
				ata_partition_lock(ext2_fs());
				ata_partition_llseek(ext2_fs(), uint64_add(ext2_get_block_address(last_block),
														   uint64_make(0, (index / divisor) * sizeof(uint32_t))), SEEK_SET);
				
				uint32_t old_block = last_block;
				
				// Write instead of read on the last one
				if (z == (depth - 1)) {
					ata_partition_write(ext2_fs(), &block_id, sizeof(uint32_t));
				}
				else
					ata_partition_read(ext2_fs(), &last_block, sizeof(uint32_t));
				ata_partition_unlock(ext2_fs());
				
				// Delete the indirect block if we need to
				if ((index / divisor) == 0 && block_id == EXT2_BLOCK_ID_INVALID)
					ext2_dealloc_block(old_block);
				
				// Go to one level lower of indirection
				index %= divisor;
				divisor /= num_entries;
			}
			
			return true;
		}
	}
	
	// We've run out of possible space
	return false;
}

// Get the byte address of a block
uint64_t ext2_get_block_address(uint32_t block_id) {
	return uint64_shl(uint64_make(0, block_id),
					  EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
}

/* Block Allocation */

// Allocates a block and set's the first entry of that block to first_block_id
bool ext2_allocate_indirect_block(uint32_t first_block_id, uint32_t* output_block_id) {
	// Allocate a new block
	uint32_t block = ext2_allocate_block();
	if (block == EXT2_BLOCK_ID_INVALID)
		return false;
	
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	
	uint32_t* data = (uint32_t*)kmalloc(block_size);
	if (!data) {
		ext2_dealloc_block(block);
		return false;
	}
	
	// Set this single indirect block
	memset(data, 0, block_size);
	data[0] = first_block_id;
	ata_partition_lock(ext2_fs());
	ata_partition_llseek(ext2_fs(), ext2_get_block_address(block), SEEK_SET);
	ata_partition_write(ext2_fs(), data, block_size);
	ata_partition_unlock(ext2_fs());
	
	*output_block_id = block;
	
	kfree(data);
	return true;
}

// Allocate a new block for use in an inode.
// Returns the block id of this block or an invalid block id if nothing could be allocated
uint32_t ext2_allocate_block() {
	// We can't allocate any if no blocks are free
	if (ext2_superblock()->free_block_count == 0)
		return EXT2_BLOCK_ID_INVALID;
	
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	uint32_t* bitmap = (uint32_t*)kmalloc(block_size);
	if (!bitmap)
		return EXT2_BLOCK_ID_INVALID;
	
	// Find the first free block by looping over all the descriptors
	uint32_t group;
	uint32_t num_groups = 1 + ((ext2_superblock()->block_count - 1) / ext2_superblock()->blocks_per_group);
	uint32_t blocks_per_bitmap_group = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size + 3);	// block_size * 8
	uint32_t bitmap_blocks_per_group = ((ext2_superblock()->blocks_per_group - 1) >>	// (blocks per group - 1) / (block size * 8) + 1
										(EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size + 3)) + 1;
	for (group = 0; group < num_groups; group++) {
		ext_block_group_descriptor_t desc = ext2_get_block_group_info(group);
		if (desc.free_blocks == 0)
			continue;
		
		// Search for the block in this group that is free
		uint32_t bitmap_block;
		for (bitmap_block = desc.block_bitmap; bitmap_block < desc.block_bitmap + bitmap_blocks_per_group; bitmap_block++) {
			// Read the bitmap
			ata_partition_lock(ext2_fs());
			ata_partition_llseek(ext2_fs(), ext2_get_block_address(bitmap_block), SEEK_SET);
			if (ata_partition_read(ext2_fs(), bitmap, block_size) == -1) {
				// Error
				ata_partition_unlock(ext2_fs());
				kfree(bitmap);
				return EXT2_BLOCK_ID_INVALID;
			}
			ata_partition_unlock(ext2_fs());
			
			// Check each block in the bitmap
			uint32_t blocks;
			for (blocks = 0; blocks < ext2_superblock()->blocks_per_group / (sizeof(uint32_t) * 8); blocks++) {
				// If these blocks are taken, don't consider
				if (bitmap[blocks] == (uint32_t)-1)
					continue;
				
				// Find the first free block
				uint32_t z;
				for (z = 0; z < sizeof(uint32_t) * 8; z++) {
					if ((bitmap[blocks] >> z) & 0x1)
						continue;
					
					// Get which block id this corresponds to
					uint32_t ret = ext2_superblock()->blocks_per_group * group +
					(bitmap_block - desc.block_bitmap) * blocks_per_bitmap_group +
					blocks * sizeof(uint32_t) * 8 + z + ext2_superblock()->first_data_block;
					
					// Mark this block as used
					bitmap[blocks] |= 1 << z;
					ata_partition_lock(ext2_fs());
					ata_partition_llseek(ext2_fs(), ext2_get_block_address(bitmap_block), SEEK_SET);
					ata_partition_write(ext2_fs(), bitmap, block_size);
					ata_partition_unlock(ext2_fs());
					
					// Update the block group descriptor
					desc.free_blocks--;
					ext2_set_block_group_info(group, &desc);
					
					// Update the super block
					ext2_superblock()->free_block_count--;
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
	return EXT2_BLOCK_ID_INVALID;
}

// Mark a block as free for use
bool ext2_dealloc_block(uint32_t block_id) {
	// Get the block_group info
	uint32_t real_id = (block_id - ext2_superblock()->first_data_block);
	uint32_t group = real_id / ext2_superblock()->blocks_per_group;
	uint32_t blocks_per_bitmap_group = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size + 3);	// block_size * 8
	
	// Allocate space for the bitmap
	uint32_t block_size = 1 << (EXT2_BASE_BLOCK_SIZE_BITS + ext2_superblock()->log_block_size);
	uint8_t* bitmap = (uint8_t*)kmalloc(block_size);
	if (!bitmap)
		return false;
	
	// Read the group
	ext_block_group_descriptor_t desc = ext2_get_block_group_info(group);
	
	// Figure out which block the block we want to free's bitmap is in
	uint32_t relative_block = real_id % ext2_superblock()->blocks_per_group;
	uint32_t bitmap_block = desc.block_bitmap + relative_block / blocks_per_bitmap_group;
	uint32_t relative_bitmap_block = relative_block % blocks_per_bitmap_group;
	
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
	desc.free_blocks++;
	ext2_set_block_group_info(group, &desc);
	
	// Update the superblock
	ext2_superblock()->free_block_count++;
	ext2_set_superblock();
	
	return true;
}
