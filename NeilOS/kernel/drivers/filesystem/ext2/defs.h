//
//  defs.h
//  NeilOS
//
//  Created by Neil Singh on 1/7/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef DEFS_H
#define DEFS_H

// Unique inodes
#define EXT2_INODE_INVALID						0x1		// Bad inode
#define EXT2_INODE_ROOT							0x2		// Root ("/") inode

// Unique blocks
#define EXT2_BLOCK_ID_INVALID					0x0		// End of block list

#define EXT2_MAGIC								0xEF53

#define SUPERBLOCK_ADDRESS						1024
#define EXT2_BASE_BLOCK_SIZE_BITS				10
#define INDEX_SIZE_BITS							2		// 4 bytes in an uint32_t
#define EXT2_INODE_BLOCK_COUNT_SIZE				9		// 512 byte blocks
#define EXT2_MAX_NAME_SIZE						255

// States
#define EXT2_VALID_FS							0x1
#define EXT2_ERROR_FS							0x2

// Error (how to handle errors)
#define EXT2_ERROR_CONTINUE						0x1		// Ignore error
#define EXT2_ERROR_RO							0x2		// Remount as read only
#define EXT2_ERROR_PANIC						0x3		// Cause a kernel panic

// Compatable Features
#define EXT2_FEATURES_COMPAT_DIR_PREALLOC		0x1		// Block preallocation for new directories
#define EXT2_FEATURES_COMPAT_IMAGIC_INODES		0x2
#define EXT2_FEATURES_COMPAT_HAS_JOURNAL		0x4		// Has a journal
#define EXT2_FEATURES_COMPAT_EXT_ATTR			0x8		// Extended inode attributes
#define EXT2_FEATURES_COMPAT_RESIZE_INO			0x10	// Non standard inode size used
#define EXT2_FEATURES_COMPAT_DIR_INDEX			0x20	// Directory indexing (HTree)

// Incompatable Features
#define EXT2_FEATUERS_INCOMPAT_COMPRESSION		0x1		// Disk/File compression is used
#define EXT2_FEATUERS_INCOMPAT_FILETYPE			0x2
#define EXT2_FEATUERS_INCOMPAT_RECOVER			0x4
#define EXT2_FEATUERS_INCOMPAT_JOURNAL_DEV		0x8
#define EXT2_FEATUERS_INCOMPAT_META_BG			0x10

// Readonly Features
#define EXT2_FEATURES_RO_SPARSE_SUPER			0x1		// Sparse Superblock
#define EXT2_FEATURES_RO_LARGE_FILE				0x2		// 64-bit file sizes
#define EXT2_FEATURES_RO_BTREE_DIR				0x4		// Binary tree sorted directory files

// Dentry file types
#define EXT2_FT_UNKNOWN							0
#define EXT2_FT_REG_FILE						1
#define EXT2_FT_DIR								2
#define EXT2_FT_CHRDIR							3		// Character device
#define EXT2_FT_BLKDEV							4		// Block device
#define EXT2_FT_FIFO							5		// Buffer (Pipe) file
#define EXT2_FT_SOCK							6		// Socket file
#define EXT2_FT_SYMLINK							7

#define EXT2_BASE_DENTRY_SIZE					8

// Inode buffers
#define	EXT2_BUFFERS_NUM_DIRECT					12
#define EXT2_BUFFERS_SINGLE_INDIRECT			12
#define EXT2_BUFFERS_DOUBLE_INDIRECT			13
#define EXT2_BUFFERS_TRIPLE_INDIRECT			14

// Inode modes
#define EXT2_INODE_MODE_SOCK					0xC000	// Socket
#define EXT2_INODE_MODE_LINK					0xA000	// Symlink
#define EXT2_INODE_MODE_REG						0x8000	// Regular file
#define EXT2_INODE_MODE_BLK						0x6000	// Block device
#define EXT2_INODE_MODE_DIR						0x4000	// Directory
#define EXT2_INODE_MODE_CHR						0x2000	// Character device
#define EXT2_INODE_MODE_FIFO					0x1000	// Fifo file (pipe)
#define EXT2_INODE_MODE_UID						0x0800	// Set process User ID
#define EXT2_INODE_MODE_GID						0x0400	// Set process Group ID
#define EXT2_INODE_MODE_VTX						0x0200	// Sticky bit (?)
#define EXT2_INODE_MODE_USER_READ				0x0100	// User read
#define EXT2_INODE_MODE_USER_WRITE				0x0080	// User write
#define EXT2_INODE_MODE_USER_EXECUTE			0x0040	// User execute
#define EXT2_INODE_MODE_GROUP_READ				0x0020	// Group read
#define EXT2_INODE_MODE_GROUP_WRITE				0x0010	// Group write
#define EXT2_INODE_MODE_GROUP_EXECUTE			0x0008	// Group execute
#define EXT2_INODE_MODE_OTHER_READ				0x0004	// Other read
#define EXT2_INODE_MODE_OTHER_WRITE				0x0002	// Other write
#define EXT2_INODE_MODE_OTHER_EXECUTE			0x0001	// Other execute
#define EXT2_INODE_MODE_ATTR_ALL				0x01FF	// All R/W/R

typedef struct {
	uint32_t inode_count;				// inode count
	uint32_t block_count;				// Block count
	uint32_t rblock_count;				// Reserved block count
	uint32_t free_block_count;			// Free block count
	uint32_t free_inode_count;			// Free inodes
	uint32_t first_data_block;			// ID of block containing superblock
	uint32_t log_block_size;			// Block size = 1024 << log_block_size
	int32_t log_frag_size;				// fragment_size = (frag_size > 0) ? (1024 << frag_size) : (1024 >> -frag_size)
	uint32_t blocks_per_group;			// Number of blocks per group
	uint32_t frags_per_group;			// Number of fragments per group
	uint32_t inodes_per_group;			// Number of inodes per group
	uint32_t mtime;						// Last mount time (UNIX time)
	uint32_t wtime;						// Last write time
	uint16_t mnt_count;					// Number of times mounted since last verification
	uint16_t max_mnt_count;				// Number of times a mount can occur before a full check
	uint16_t magic;						// EXT Magic Number (0xEF53)
	uint16_t state;						// Denotes filesystem state (set to ERROR on mount, VALID on unmount)
	uint16_t error;						// What the filesystem should do upon an error
	uint16_t minor_rev_level;			// Minor reversion level
	uint32_t last_check_time;			// Time of last check
	uint32_t check_interval;			// Max time interval between checks
	uint32_t creator_os;				// Creater OS value
	uint32_t rev_level;					// Revision Level
	uint16_t def_resuid;				// Default User ID for reserved blocks
	uint16_t def_resgid;				// Default Group ID for reserved blocks
	uint32_t first_inode;				// First available inode for standard files
	uint16_t inode_size;				// Size of inode structure
	uint16_t block_group_nr;			// Block group hosting superblock
	uint32_t features_compat;			// (Optional) Features bitmap
	uint32_t features_incompat;			// (Required) Features bitmap
	uint32_t features_ro;				// (Read-only if unsupported) Features bitmap
	uint32_t uuid[4];					// Unique volume ID
	uint8_t volume_name[16];			// Volume Name
	uint8_t last_mounted[64];			// Path where volume was last mounted
	uint32_t algo_bitmap;				// Bitmap compression algorithm
} ext_superblock_t;

typedef struct {
	uint32_t block_bitmap;				// Block ID of block bitmap start
	uint32_t inode_bitmap;				// Block ID of inode bitmap start
	uint32_t inode_table;				// Block ID of inode table start
	uint16_t free_blocks;				// Number of free blocks in group
	uint16_t free_inodes;				// Number of free inodes in group
	uint16_t used_dirs_count;			// Number of inodes used for directories in group
	uint16_t padding;
	uint32_t reserved[3];
} ext_block_group_descriptor_t;

typedef struct {
	uint32_t inode;						// Inode of this dentry
	uint16_t rec_len;					// Size of this record
	uint8_t name_len;					// Length of the filename
	uint8_t file_type;					// Type of file
	int8_t name[EXT2_MAX_NAME_SIZE];	// Name of the file
} ext_dentry_t;

// Inode
typedef struct {
	uint16_t mode;						// Inode mode
	uint16_t uid;						// User ID of the inode
	uint32_t size;						// Lower 32-bits of the file size
	uint32_t atime;						// Access time
	uint32_t ctime;						// Creation time
	uint32_t mtime;						// Modified time
	uint32_t dtime;						// Deletion time
	uint16_t gid;						// POSIX group having access to this file
	uint16_t link_count;				// Number of hard links to file (usually 1 for files)
	uint32_t num_blocks;				// Number of 512-byte blocks reserved
	uint32_t flags;						// Flags
	uint32_t osd1;						// OS-Dependent
	uint32_t blocks[0xF];				// Block ID array for data
	uint32_t generation;				// File version
	uint32_t file_acl;					// Block ID of extended attributes
	uint32_t size_high;					// High 32 bits of file size
	uint32_t faddr;						// File fragment location
	uint32_t osd2[3];					// OS-Dependent
} ext_inode_info_t;

// Structure that contains the data and the inode number
typedef struct {
	ext_inode_info_t info;
	uint32_t inode;
	
	// Caching info
	uint32_t* cached_block;
	uint32_t cached_block_starting_index;
} ext_inode_t;

#endif
