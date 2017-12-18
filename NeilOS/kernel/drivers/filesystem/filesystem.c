#include "filesystem.h"
#include <common/lib.h>
#include <memory/memory.h>
#include <drivers/ATA/ata.h>
#include <drivers/pipe/fifo.h>
#include "ext2/ext2.h"
#include "path.h"
#include <syscalls/interrupt.h>

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

// Filesystem disk partition
disk_info_t filesystem_fs;

// Device files
typedef struct device_file {
	uint32_t inode;
	file_descriptor_t* (*open)(const char* name, uint32_t mode);
	struct device_file* next;
} device_file_t;

device_file_t* device_files = NULL;

// Add a device file
bool device_file_add(char* name, file_descriptor_t* (*open)(const char*, uint32_t)) {
	// Append the name to /dev/
	static const char* prefix = "/dev/";
	char* path = path_append(prefix, name);
	if (!path)
		return false;
	
	// Create a file with this name
	file_descriptor_t file;
	if (!fopen(path, FILE_MODE_WRITE | FILE_MODE_CREATE, &file)) {
		kfree(path);
		return false;
	}
	
	uint32_t inode = ((file_info_t*)file.info)->inode.inode;
	fclose(&file);
	
	device_file_t* t = kmalloc(sizeof(device_file_t));
	if (!t) {
		file_descriptor_t f;
		if (!fopen(path, FILE_MODE_READ | FILE_MODE_DELETE_ON_CLOSE, &f)) {
			kfree(path);
			return false;
		}
		fclose(&f);
		kfree(path);
		return false;
	}
	t->inode = inode;
	t->open = open;
	t->next = device_files;
	device_files = t;
	
	kfree(path);
	
	return true;
}

// Return the device file open function for the given inode (null if none exists)
file_descriptor_t* (*device_file_get_for_inode(uint32_t inode))(const char*, uint32_t) {
	device_file_t* t = device_files;
	while (t) {
		if (t->inode == inode)
			return t->open;
		t = t->next;
	}
	return NULL;
}

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
			ext_inode_t parent = ext2_get_parent_inode(filename);
			if (parent.inode == EXT2_INODE_INVALID)
				return false;
			// Get the name
			char* name = get_last_path_component(filename);
			if (!name)
				return false;
			
			// Create it
			inode = ext2_create(&parent, name, mode & ~FILE_TYPE_DIRECTORY);
			kfree(name);
			if (inode.inode == EXT2_INODE_INVALID)
				return false;
		} else
			return false;
	}
	if ((inode.info.mode & EXT2_INODE_MODE_FIFO)) {
		file_descriptor_t* fifo = fifo_open(filename, mode);
		if (!fifo)
			return false;
		memcpy(desc, fifo, sizeof(file_descriptor_t));
		kfree(fifo);
		return true;
	}
	
	// If this is a device file, use that open instead
	file_descriptor_t* (*open)(const char* name, uint32_t mode) = device_file_get_for_inode(inode.inode);
	if (open) {
		file_descriptor_t* ret = open(path_last_component(filename, NULL), mode);
		if (!ret)
			return false;
		
		memcpy(desc, ret, sizeof(file_descriptor_t));
		kfree(ret);
		return true;
	}
	
	// Copy the filename
	uint32_t len = strlen(filename);
	desc->filename = (char*)kmalloc(len + 1);
	if (!desc->filename)
		return false;
	strcpy(desc->filename, filename);
	
	desc->mode = mode & ~(FILE_MODE_CREATE);
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
		dir->offset = ext2_get_block_address(ext2_get_block_id_at_index(&dir->inode, 0));
		
		desc->read = filesystem_read_directory;
		desc->write = filesystem_write_directory;
		desc->llseek = filesystem_llseek_directory;
		desc->truncate = NULL;
		desc->mode = (desc->mode & ~FILE_TYPE_ALL) | FILE_TYPE_DIRECTORY;
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
		desc->mode = desc->mode & ~FILE_TYPE_ALL;
		if (inode.info.mode & EXT2_INODE_MODE_FIFO)
			desc->mode |= FILE_TYPE_PIPE;
		else
			desc->mode |= FILE_TYPE_REGULAR;
		
		// Delete the file contents if needed
		if ((desc->mode & FILE_MODE_TRUNCATE) &&
			((desc->mode & FILE_MODE_WRITE) || (desc->mode & FILE_MODE_APPEND))) {
			ftruncate(desc, uint64_make(0, 0));
		}
	}
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
uint32_t fread_directory(void* buffer, uint32_t size, uint32_t count, file_descriptor_t* f, dirent_t* dirent) {
	if (size * count == 0)
		return 0;
	
	directory_info_t* dir = (directory_info_t*)f->info;
	
	// If we've read all the files, do nothing
	if (uint64_equal(dir->offset, uint64_make(0, 0)))
		return 0;
	
	// Get the next offset
	uint32_t ret = 0;
	while (ret == 0) {
		ext_dentry_t dentry;
		uint64_t value = ext2_read_directory(&dir->inode, dir->offset, buffer, size * count, &ret, &dentry);
		if (dirent) {
			dirent->ino = dentry.inode;
			dirent->off = dir->index;
			dirent->reclen = dentry.rec_len;
			dirent->type = dentry.file_type;
		}
		dir->offset = value;
		// We've reached the end of the block
		if (uint64_equal(dir->offset, uint64_make(0, 0))) {
			// Go to the next block
			dir->bank_index++;
			uint32_t block_id = ext2_get_block_id_at_index(&dir->inode, dir->bank_index);
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
		return -EACCES;
	
	if (file->mode & FILE_TYPE_REGULAR)
		return fread_file(buffer, size, count, file);
	if (file->mode & FILE_TYPE_DIRECTORY)
		return fread_directory(buffer, size, count, file, NULL);
	
	return -ENOENT;
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
	if (!(file->mode & FILE_MODE_WRITE) && !(file->mode & FILE_MODE_APPEND))
		return -EACCES;
	
	if (file->mode & FILE_MODE_APPEND)
		fseek(file, uint64_make(0, 0), SEEK_END);
	
	if (file->mode & FILE_TYPE_REGULAR)
		return fwrite_file(buffer, size, count, file);
	else if (file->mode & FILE_TYPE_DIRECTORY)
		return fwrite_directory(buffer, size, count, file);
	
	return -ENOENT;
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
	if (uint64_greater(file->offset, file_size)) {
		if (f->mode & FILE_MODE_WRITE)
			file->offset = ext2_truncate_inode(&file->inode, file->offset);
		else
			file->offset = file_size;
	}
	
	return file->offset;
}

// Seek a directory
uint64_t fseek_directory(file_descriptor_t* f, uint64_t offset, int whence) {
	directory_info_t* dir = (directory_info_t*)f->info;
	
	// Find the correct relative offset if we need to
	if (whence == SEEK_CUR) {
		if ((offset.high >> 31) & 0x1) {
			whence = SEEK_SET;
			offset.low = dir->index + offset.low;
			offset.high = 0;
		} else if (offset.high == 0 && offset.low == 0) {
			// Just return the current index
			return uint64_make(0, dir->index);
		}
	}
	
	// Set the new index
	if (whence == SEEK_SET) {
		// Load the first entry
		dir->index = 0;
		dir->bank_index = 0;
		dir->offset = ext2_get_block_address(ext2_get_block_id_at_index(&dir->inode, 0));
	} else if (whence == SEEK_END)
		offset.low = ~0;				// Go as many times as possible until we hit an error
	
	// Follow the linked list the desired number of times
	uint32_t z;
	uint32_t block_id = ext2_get_block_id_at_index(&dir->inode, dir->bank_index);
	for (z = 0; z < offset.low; z++) {		
		// Get the next offset
		uint32_t ret = 0;
		while (ret == 0) {
			uint64_t value = ext2_read_directory(&dir->inode, dir->offset, NULL, 256, &ret, NULL);
			dir->offset = value;
			// We've reached the end of the block
			if (uint64_equal(dir->offset, uint64_make(0, 0))) {
				// Go to the next block
				dir->bank_index++;
				block_id = ext2_get_block_id_at_index(&dir->inode, dir->bank_index);
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
	if (file->mode & FILE_TYPE_REGULAR)
		return fseek_file(file, offset, whence);
	else if (file->mode & FILE_TYPE_DIRECTORY)
		return fseek_directory(file, offset, whence);
	
	return uint64_make(-1, -ENOENT);
}

// Truncate a file to a specific size
uint64_t ftruncate(file_descriptor_t* f, uint64_t size) {
	if (!(f->mode & FILE_TYPE_REGULAR))
		return uint64_make(-1, -EACCES);
	
	file_info_t* file = (file_info_t*)f->info;
	
	return ext2_truncate_inode(&file->inode, size);
}

// Truncate a file descriptor
uint64_t filesystem_truncate(int32_t fd, uint64_t nsize) {
	return ftruncate(descriptors[fd], nsize);
}

// Make a directory
int fmkdir(const char* filename) {
	// If the inode already exists, don't do anything
	if (ext2_open(filename).inode != EXT2_INODE_INVALID)
		return -EEXIST;
	
	// Get the parent
	ext_inode_t parent = ext2_get_parent_inode(filename);
	if (parent.inode == EXT2_INODE_INVALID)
		return -ENOENT;
	
	// Get the name
	char* name = get_last_path_component(filename);
	if (!name)
		return -ENOMEM;
	
	// Create the directory
	ext_inode_t inode = ext2_create(&parent, name, FILE_TYPE_DIRECTORY);
	kfree(name);
	
    if (inode.inode == EXT2_INODE_INVALID)
		return -ENOSPC;
	return 0;
}

// Hard link a file or directory
int flink(file_descriptor_t* file, const char* link_name) {
	ext_inode_t* inode = NULL;
	if (file->mode & FILE_TYPE_DIRECTORY)
		inode = &((directory_info_t*)file->info)->inode;
	else
		inode = &((file_info_t*)file->info)->inode;
	
	// Get the parent
	ext_inode_t parent = ext2_get_parent_inode(link_name);
	if (parent.inode == EXT2_INODE_INVALID)
		return -ENOENT;
	
	// Get the names
	char* name = get_last_path_component(link_name);
	if (!name)
		return -ENOMEM;
	
	// Link
	bool ret = ext2_link(inode, &parent, name);
	
	kfree(name);
	
    return (ret ? -ENOSPC : 0);
}

// Helper to unlink
int fsunlinkalways(const char* filename) {
	// Get the parent directory
	ext_inode_t parent = ext2_get_parent_inode(filename);
	if (parent.inode == EXT2_INODE_INVALID)
		return -ENOENT;
	
	// Get the name
	char* name = get_last_path_component(filename);
	if (!name)
		return -ENOMEM;
	
	// Ensure that the name is not either "." or ".."
	if (strncmp(name, ".", 2) == 0 || strncmp(name, "..", 3) == 0) {
		kfree(name);
		return -EINVAL;
	}
	
	// Delete the file
	bool ret = ext2_delete(&parent, name);
	kfree(name);
	
    return (ret ? -ENOMEM : 0);
}

// Unlink
int fsunlink(const char* filename) {
	ext_inode_t inode = ext2_open(filename);
	if (inode.inode == EXT2_INODE_INVALID)
		return -ENOENT;
	
	if (inode.info.mode & FILE_TYPE_DIRECTORY) {
		directory_info_t info;
		info.inode = inode;
		file_descriptor_t f;
		memset(&f, 0, sizeof(f));
		f.info = &info;
		
		// Check that the directory is empty
		uint32_t num_entries = fseek_directory(&f, uint64_make(0, 0), SEEK_END).low;
		if (num_entries > 2)		// . and .. are fine
			return -ENOTEMPTY;
	}
	
	return fsunlinkalways(filename);
}

// Helper to delete a file
bool fdelete(file_descriptor_t* file) {
	if (file->mode & FILE_TYPE_DIRECTORY) {
		// Check that the directory is empty
		uint32_t num_entries = fseek_directory(file, uint64_make(0, 0), SEEK_END).low;
		if (num_entries > 2)		// . and .. are fine
			return false;
	}
	
	return fsunlinkalways(file->filename);
}

// Close a file object
bool fclose(file_descriptor_t* file) {
	uint32_t ret = true;
	// Delete it on close if we have to
	if (file->mode & FILE_MODE_DELETE_ON_CLOSE)
		ret = fdelete(file);
	
	// The argument checking is done by the syscall
	kfree(file->filename);
	
	if (file->mode & FILE_TYPE_DIRECTORY)
		ext_free_inode(&((directory_info_t*)file->info)->inode);
	else
		ext_free_inode(&((file_info_t*)file->info)->inode);
	
	kfree(file->info);
	return ret;
}

// Returns true if path is directory
bool fisdir(const char* filename) {
	ext_inode_t inode = ext2_open(filename);
	if (inode.inode == EXT2_INODE_INVALID)
		return false;
	
	return (inode.info.mode & FILE_TYPE_DIRECTORY) != 0;
}

// Set times
void fsettime(file_descriptor_t* file, uint32_t atime, uint32_t mtime) {
	ext_inode_t inode;
	if (file->mode & FILE_TYPE_DIRECTORY)
		inode = ((directory_info_t*)file->info)->inode;
	else
		inode = ((file_info_t*)file->info)->inode;

	// Update the times of this inode
	inode.info.atime = atime;
	inode.info.mtime = mtime;
	
	// Write this inode data
	ext2_set_inode_info(inode.inode, &inode.info);
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
	return fread_directory(buf, 1, length, descriptors[fd], NULL);
}

uint32_t filesystem_readdir(int32_t fd, void* buf, uint32_t length, dirent_t* dirent) {
	return fread_directory(buf, 1, length, descriptors[fd], dirent);
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
	if (desc->mode & FILE_TYPE_REGULAR) {
		info = &(((file_info_t*)desc->info)->inode.info);
		ret->inode = ((file_info_t*)desc->info)->inode.inode;
	}
	else if (desc->mode & FILE_TYPE_DIRECTORY) {
		info = &(((directory_info_t*)desc->info)->inode.info);
		ret->inode = ((directory_info_t*)desc->info)->inode.inode;
	}
	if (!info)
		return -1;
	
	ret->dev_id = 0;
	ret->mode = desc->mode;
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
	
	if (f->mode & FILE_TYPE_REGULAR) {
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
	return fclose(fd) ? 0 : -1;
}
