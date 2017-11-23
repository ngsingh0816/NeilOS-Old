#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <common/types.h>
#include <syscalls/descriptor.h>
#include <syscalls/impl/sysfs.h>

// Add a device file
bool device_file_add(char* name, file_descriptor_t* (*open)(const char*, uint32_t));
// Return the device file open function for the given inode (null if none exists)
file_descriptor_t* (*device_file_get_for_inode(uint32_t inode))(const char*, uint32_t);

// Initialize the filesystem.
bool filesystem_init(char* filesystem);

// For internal use
bool fopen(const char* filename, uint32_t mode, file_descriptor_t* desc);

// Read, write, seek
uint32_t fread(void* buffer, uint32_t size, uint32_t count, file_descriptor_t* file);
uint32_t fwrite(const void* buffer, uint32_t size, uint32_t count, file_descriptor_t* file);
uint64_t fseek(file_descriptor_t* file, uint64_t offset, int whence);

// Change size
uint64_t ftruncate(file_descriptor_t* file, uint64_t size);

// Make directory (delete directory is just delete on close)
bool fmkdir(const char* filename);

// Hard link
bool flink(file_descriptor_t* file, const char* link_name);

// Close
bool fclose(file_descriptor_t* file);

// Times
void fsettime(file_descriptor_t* file, uint32_t atime, uint32_t mtime);

// Syscalls
file_descriptor_t* filesystem_open(const char* filename, uint32_t mode);
uint32_t filesystem_read_file(int32_t fd, void* buf, uint32_t length);
uint32_t filesystem_write_file(int32_t fd, const void* buf, uint32_t length);
uint64_t filesystem_llseek_file(int32_t fd, uint64_t offset, int whence);
uint32_t filesystem_stat(int32_t fd, sys_stat_type* data);
uint64_t filesystem_truncate(int32_t fd, uint64_t nsize);

uint32_t filesystem_read_directory(int32_t fd, void* buf, uint32_t length);
uint32_t filesystem_write_directory(int32_t fd, const void* buf, uint32_t length);
uint64_t filesystem_llseek_directory(int32_t fd, uint64_t offset, int whence);

uint32_t filesystem_readdir(int32_t fd, void* buf, uint32_t length, dirent_t* dirent);

file_descriptor_t* filesystem_duplicate(file_descriptor_t* f);

uint32_t filesystem_close(file_descriptor_t* fd);

#endif
