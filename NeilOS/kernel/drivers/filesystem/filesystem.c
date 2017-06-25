#include "filesystem.h"
#include <common/lib.h>
#include <memory/memory.h>
#include <drivers/ATA/ata.h>
#include "ext2/ext2.h"
#include "path.h"

// Used because it saves 24 bytes
typedef struct {
	uint8_t filename[32];
	uint32_t file_type;
	uint32_t inode;
} dentry_internal;

// Internal information for a file
typedef struct {
	uint64_t offset;
	ext_inode_t inode;
} file_info_t;

// Internal information for a directory
typedef struct {
	uint64_t offset;			// Dentry absolute byte address
	uint32_t index;				// Dentry index
	uint32_t bank_index;		// Dentry's bank index in the inode
	ext_inode_t inode;
} directory_info_t;

dentry_internal dentries[63];

// Filesystem disk partition
disk_info_t filesystem_fs;

// Initialize the filesystem
bool filesystem_init(char* filesystem) {
	if (!filesystem)
		return false;
	
	// Get the disk and the desired partition (of the form disk0s1)
	uint8_t disk = filesystem[4] - '0', partition = 0;
	if (strlen(filesystem) == 7)
		partition = filesystem[6] - '0';

	filesystem_fs = ata_open_partition(disk, partition);
	if (filesystem_fs.bus == (uint8_t)-1)
		return false;
		
	// See if it is an ext2 filesystem
	if (ext2_init(&filesystem_fs))
		return true;
	
	// No filesystem driver found
	return false;
}

// Helper function to get parent inode of a path
ext_inode_t get_parent_inode(const char* filename) {
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

// Helper function to get the last path component of a path (must be freed!)
char* get_last_path_component(const char* filename) {
	// Get the name
	uint32_t len = 0;
	const char* lp = path_last_component(filename, &len);
	char* name = kmalloc(len + 1);
	if (!name)
		return NULL;
	
	memcpy(name, lp, len);
	name[len] = 0;
	
	return name;
}

// Open a file object
bool fopen(const char* filename, uint32_t mode, file_descriptor_t* desc) {
	memset(desc, 0, sizeof(file_descriptor_t));
	
	// Open the file
	ext_inode_t inode = ext2_open(filename);
	if (inode.inode == EXT2_INODE_INVALID) {
		// Try to create it if it doesn't exist
		if (mode & FILE_MODE_CREATE) {
			// Get the parent
			ext_inode_t parent = get_parent_inode(filename);
			if (parent.inode == EXT2_INODE_INVALID)
				return false;
			// Get the name
			char* name = get_last_path_component(filename);
			if (!name)
				return false;
			
			// Create it
			inode = ext2_create(&parent, name, false);
			kfree(name);
			if (inode.inode == EXT2_INODE_INVALID)
				return false;
		} else
			return false;
	}
	
	// Copy the filename
	uint32_t len = strlen(filename);
	desc->filename = (char*)kmalloc(len + 1);
	if (!desc->filename)
		return false;
	strcpy(desc->filename, filename);
	
	if (ext2_inode_is_directory(&inode)) {
		// Set up a directory
		desc->info = (directory_info_t*)kmalloc(sizeof(directory_info_t));
		if (!desc->info) {
			kfree(desc->filename);
			return false;
		}
		directory_info_t* dir = (directory_info_t*)desc->info;
		memset(dir, 0, sizeof(directory_info_t));
		dir->inode = inode;
		dir->offset = ext2_get_block_address(ext2_get_block_id_at_index(&dir->inode.info, 0));
		
		desc->read = filesystem_read_directory;
		desc->write = filesystem_write_directory;
		desc->llseek = filesystem_llseek_directory;
		desc->truncate = NULL;
		desc->type = DIRECTORY_FILE_TYPE;
	} else {
		// Set up a file
		desc->info = (file_info_t*)kmalloc(sizeof(file_info_t));
		if (!desc->info) {
			kfree(desc->filename);
			return false;
		}
		file_info_t* file = (file_info_t*)desc->info;
		memset(file, 0, sizeof(file_info_t));
		file->inode = inode;
		
		desc->read = filesystem_read_file;
		desc->write = filesystem_write_file;
		desc->llseek = filesystem_llseek_file;
		desc->truncate = filesystem_truncate;
		desc->type = FILE_FILE_TYPE;
	}
	desc->mode = mode | FILE_TYPE_REGULAR;
	desc->stat = filesystem_stat;
	desc->duplicate = filesystem_duplicate;
	desc->close = filesystem_close;
	
	return true;
}

// Read a file
uint32_t fread_file(void* buffer, uint32_t size, uint32_t count, file_descriptor_t* f) {
	file_info_t* file = (file_info_t*)f->info;
	uint32_t length = size * count;
	
	// Don't read past the end of the file
	uint64_t file_length = uint64_make(file->inode.info.size_high, file->inode.info.size);
	if (uint64_greater(uint64_add(file->offset, uint64_make(0, length)), file_length))
		length = uint64_sub(file_length, file->offset).low;
	
	// Don't read anything if we aren't reading anything
	if (length == 0)
		return 0;
	
	// Read the data and set the new position
	uint32_t ret = ext2_read_data(&file->inode, file->offset, buffer, length);
	file->offset = uint64_add(file->offset, uint64_make(0, ret));
	
	return ret;
}

// Read a directory
uint32_t fread_directory(void* buffer, uint32_t size, uint32_t count, file_descriptor_t* f) {
	if (size * count == 0)
		return 0;
	
	directory_info_t* dir = (directory_info_t*)f->info;
	
	// If we've read all the files, do nothing
	if (uint64_equal(dir->offset, uint64_make(0, 0)))
		return 0;
	
	// Get the next offset
	uint32_t ret = 0;
	while (ret == 0) {
		uint64_t value = ext2_read_directory(&dir->inode, dir->offset, buffer, size * count, &ret);
		dir->offset = value;
		// We've reached the end of the block
		if (uint64_equal(dir->offset, uint64_make(0, 0))) {
			// Go to the next block
			dir->bank_index++;
			uint32_t block_id = ext2_get_block_id_at_index(&dir->inode.info, dir->bank_index);
			// Get the real next offset
			if (block_id == EXT2_BLOCK_ID_INVALID) {
				dir->offset = uint64_make(0, 0);
				break;
			}
			
			dir->offset = ext2_get_block_address(block_id);
		}
	}
	dir->index++;
	
	// Return the length of the filename
	return ret;
}

// Read a file object
uint32_t fread(void* buffer, uint32_t size, uint32_t count, file_descriptor_t* file) {
	if (!(file->mode & FILE_MODE_READ))
		return -1;
	
	if (file->type == FILE_FILE_TYPE)
		return fread_file(buffer, size, count, file);
	else if (file->type == DIRECTORY_FILE_TYPE)
		return fread_directory(buffer, size, count, file);
	
	return -1;
}

// Write to a file
uint32_t fwrite_file(const void* buffer, uint32_t size, uint32_t count, file_descriptor_t* f) {
	file_info_t* file = (file_info_t*)f->info;
	uint32_t length = size * count;
	
	// Don't write anything if we aren't writing anything
	if (length == 0)
		return 0;
	
	// Write the data and set the new position
	uint32_t ret = ext2_write_data(&file->inode, file->offset, buffer, length);
	file->offset = uint64_add(file->offset, uint64_make(0, ret));
	
	return ret;
}

// Write to a directory
uint32_t fwrite_directory(const void* buffer, uint32_t size, uint32_t count, file_descriptor_t* f) {
	return -1;
}

// Write to a file object
uint32_t fwrite(const void* buffer, uint32_t size, uint32_t count, file_descriptor_t* file) {
	if (!(file->mode & FILE_MODE_WRITE))
		return -1;
	
	if (file->type & FILE_MODE_APPEND)
		fseek(file, uint64_make(0, 0), SEEK_END);
	
	if (file->type == FILE_FILE_TYPE)
		return fwrite_file(buffer, size, count, file);
	else if (file->type == DIRECTORY_FILE_TYPE)
		return fwrite_directory(buffer, size, count, file);
	
	return -1;
}

// Seek a file
uint64_t fseek_file(file_descriptor_t* f, uint64_t offset, int whence) {
	file_info_t* file = (file_info_t*)f->info;
	uint64_t file_size = uint64_make(file->inode.info.size_high, file->inode.info.size);
	
	// Place the new offset
	if (whence == SEEK_SET)
		file->offset = offset;
	else if (whence == SEEK_CUR)
		file->offset = uint64_add(file->offset, offset);
	else
		file->offset = file_size;
	
	// Check we still have a valid range
	if (uint64_greater(file->offset, file_size))
		file->offset = file_size;
	
	return file->offset;
}

// Seek a directory
uint64_t fseek_directory(file_descriptor_t* f, uint64_t offset, int whence) {
	directory_info_t* dir = (directory_info_t*)f->info;
	
	// Negative seeking is not supported
	if ((offset.high >> 31) & 0x1)
		return uint64_make(0, dir->index);
	else if (offset.high != 0)
		whence = SEEK_END;
	
	// Find the correct relative offset if we need to
	if (whence == SEEK_CUR) {
		if (offset.low < dir->index)
			whence = SEEK_SET;
		else
			offset.low -= dir->index;
	}
	
	// Set the new index
	if (whence == SEEK_SET) {
		// Load the first entry
		dir->index = 0;
		dir->bank_index = 0;
		dir->offset = ext2_get_block_address(ext2_get_block_id_at_index(&dir->inode.info, 0));
	} else if (whence == SEEK_END)
		offset.low = ~0;				// Go as many times as possible until we hit an error
	
	// Follow the linked list the desired number of times
	uint32_t z;
	uint32_t block_id = ext2_get_block_id_at_index(&dir->inode.info, dir->bank_index);
	for (z = 0; z < offset.low; z++) {		
		// Get the next offset
		uint32_t ret = 0;
		while (ret == 0) {
			uint64_t value = ext2_read_directory(&dir->inode, dir->offset, NULL, 256, &ret);
			dir->offset = value;
			// We've reached the end of the block
			if (uint64_equal(dir->offset, uint64_make(0, 0))) {
				// Go to the next block
				dir->bank_index++;
				block_id = ext2_get_block_id_at_index(&dir->inode.info, dir->bank_index);
				// Check if we've gone too far
				if (block_id == EXT2_BLOCK_ID_INVALID) {
					dir->offset = uint64_make(0, 0);
					if (ret != 0)
						dir->index++;
					return uint64_make(0, dir->index);
				}
				
				// Get the real next offset
				dir->offset = ext2_get_block_address(block_id);
			}
		}
		dir->index++;

	}
	
	return uint64_make(0, dir->index);
}

// Seek a file object
uint64_t fseek(file_descriptor_t* file, uint64_t offset, int whence) {
	if (file->type == FILE_FILE_TYPE)
		return fseek_file(file, offset, whence);
	else if (file->type == DIRECTORY_FILE_TYPE)
		return fseek_directory(file, offset, whence);
	
	return uint64_make(-1, -1);
}

// Truncate a file to a specific size
uint64_t ftruncate(file_descriptor_t* f, uint64_t size) {
	if (f->type != FILE_FILE_TYPE)
		return uint64_make(-1, -1);
	
	file_info_t* file = (file_info_t*)f->info;
	
	return ext2_truncate_inode(&file->inode, size);
}

// Truncate a file descriptor
uint64_t filesystem_truncate(int32_t fd, uint64_t nsize) {
	return ftruncate(descriptors[fd], nsize);
}

// Make a directory
bool fmkdir(const char* filename) {
	// If the inode already exists, don't do anything
	if (ext2_open(filename).inode != EXT2_INODE_INVALID)
		return false;
	
	// Get the parent
	ext_inode_t parent = get_parent_inode(filename);
	if (parent.inode == EXT2_INODE_INVALID)
		return false;
	
	// Get the name
	char* name = get_last_path_component(filename);
	if (!name)
		return false;
	
	// Create the directory
	ext_inode_t inode = ext2_create(&parent, name, true);
	kfree(name);
	
	return (inode.inode != EXT2_INODE_INVALID);
}

// Hard link a file or directory
bool flink(file_descriptor_t* file, const char* link_name) {
	ext_inode_t* inode = NULL;
	if (file->type == FILE_FILE_TYPE)
		inode = &((file_info_t*)file->info)->inode;
	else
		inode = &((directory_info_t*)file->info)->inode;
	
	// Get the parent
	ext_inode_t parent = get_parent_inode(link_name);
	if (parent.inode == EXT2_INODE_INVALID)
		return false;
	
	// Get the names
	char* name = get_last_path_component(link_name);
	if (!name)
		return false;
	
	// Link
	bool ret = ext2_link(inode, &parent, name);
	
	kfree(name);
	
	return ret;
}

// Helper to delete a file
bool fdelete(file_descriptor_t* file) {
	if (file->type & DIRECTORY_FILE_TYPE) {
		// Check that the directory is empty
		uint32_t num_entries = fseek_directory(file, uint64_make(0, 0), SEEK_END).low;
		if (num_entries > 2)		// . and .. are fine
			return false;
	}
	
	// Get the parent directory
	ext_inode_t parent = get_parent_inode(file->filename);
	if (parent.inode == EXT2_INODE_INVALID)
		return false;
	
	// Get the name
	char* name = get_last_path_component(file->filename);
	if (!name)
		return false;
	
	// Ensure that the name is not either "." or ".."
	if (strncmp(name, ".", 2) == 0 || strncmp(name, "..", 3) == 0)
		return false;
	
	// Delete the file
	bool ret = ext2_delete(&parent, name);
	kfree(name);
	
	return ret;
}

// Close a file object
bool fclose(file_descriptor_t* file) {
	uint32_t ret = true;
	// Delete it on close if we have to
	if (file->mode & FILE_MODE_DELETE_ON_CLOSE)
		ret = fdelete(file);
	
	// The argument checking is done by the syscall
	kfree(file->filename);
	kfree(file->info);
	return ret;
}

// Open a file or directory
file_descriptor_t* filesystem_open(const char* filename, uint32_t mode) {
	// Open the file
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	if (!fopen(filename, mode, d)) {
		kfree(d);
		return NULL;
	}
	
	return d;
}

// Read from a file and return the number of bytes read
uint32_t filesystem_read_file(int32_t fd, void* buf, uint32_t length) {
	return fread_file(buf, 1, length, descriptors[fd]);
}

// Write to a file (it's readonly so this call always fails)
uint32_t filesystem_write_file(int32_t fd, const void* buf, uint32_t length) {
	return fwrite_file(buf, 1, length, descriptors[fd]);
}

// Seek to an offset in the file
uint64_t filesystem_llseek_file(int32_t fd, uint64_t offset, int whence) {
	return fseek_file(descriptors[fd], offset, whence);
}

// Read from a directory. Each subsequent call to read will return the next
// entry in the directory until we've read all the entries and then it will return 0.
uint32_t filesystem_read_directory(int32_t fd, void* buf, uint32_t length) {
	return fread_directory(buf, 1, length, descriptors[fd]);
}

// Write to a directory (which means link a new file in)
uint32_t filesystem_write_directory(int32_t fd, const void* buf, uint32_t length) {
	return fwrite_directory(buf, 1, length, descriptors[fd]);
}

// Seek to an offset in the directory (the index of the directory entry)
uint64_t filesystem_llseek_directory(int32_t fd, uint64_t offset, int whence) {
	return fseek_directory(descriptors[fd], offset, whence);
}

// Get file info
uint32_t filesystem_stat(int32_t fd, sys_stat_type* ret) {
	file_descriptor_t* desc = descriptors[fd];
	memset(ret, 0, sizeof(sys_stat_type));
	
	ext_inode_info_t* info = NULL;
	if (desc->type == FILE_FILE_TYPE) {
		info = &(((file_info_t*)desc->info)->inode.info);
		ret->inode = ((file_info_t*)desc->info)->inode.inode;
	}
	else if (desc->type == DIRECTORY_FILE_TYPE ){
		info = &(((directory_info_t*)desc->info)->inode.info);
		ret->inode = ((directory_info_t*)desc->info)->inode.inode;
	}
	if (!info)
		return -1;
	
	ret->dev_id = 0;
	ret->mode = desc->mode | (desc->type << 16);
	ret->num_links = info->link_count;
	ret->size = info->size;
	ret->block_size = ext2_get_block_size();
	if (ret->size == 0)
		ret->num_512_blocks = 0;
	else
		ret->num_512_blocks = (ret->size >> 9) + 1;
	
	ret->atime.val = info->atime;
	ret->ctime.val = info->ctime;
	ret->mtime.val = info->mtime;
	
	return 0;
}

// Duplicate the file handle
file_descriptor_t* filesystem_duplicate(file_descriptor_t* f) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memcpy(d, f, sizeof(file_descriptor_t));
	
	uint32_t len = strlen(f->filename);
	d->filename = kmalloc(len + 1);
	if (!d->filename) {
		kfree(d);
		return NULL;
	}
	memcpy(d->filename, f->filename, len + 1);
	
	if (f->type == FILE_FILE_TYPE) {
		d->info = kmalloc(sizeof(file_info_t));
		if (!d->info) {
			kfree(d->filename);
			kfree(d);
			return NULL;
		}
		memcpy(d->info, f->info, sizeof(file_info_t));
	} else {
		d->info = kmalloc(sizeof(directory_info_t));
		if (!d->info) {
			kfree(d->filename);
			kfree(d);
			return NULL;
		}
		memcpy(d->info, f->info, sizeof(directory_info_t));
	}
	
	return d;
}

// Close a file handle
uint32_t filesystem_close(file_descriptor_t* fd) {
	return fclose(fd) ? -1 : 0;
}
