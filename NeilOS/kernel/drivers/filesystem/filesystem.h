#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <common/types.h>
#include <syscalls/descriptor.h>

// Each block is 4096
#define BLOCK_SIZE			4096

// Initialize the filesystem.
bool filesystem_init(char* filesystem);

// For internal use
bool fopen(const char* filename, uint32_t mode, file_descriptor_t* desc);

// Get file info
sys_stat_type fstat(file_descriptor_t* desc);

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


// Syscalls
file_descriptor_t* filesystem_open(const char* filename, uint32_t mode);
uint32_t filesystem_read_file(int32_t fd, void* buf, uint32_t length);
uint32_t filesystem_write_file(int32_t fd, const void* buf, uint32_t length);
uint64_t filesystem_llseek_file(int32_t fd, uint64_t offset, int whence);
uint64_t filesystem_truncate(int32_t fd, uint64_t nsize);

uint32_t filesystem_read_directory(int32_t fd, void* buf, uint32_t length);
uint32_t filesystem_write_directory(int32_t fd, const void* buf, uint32_t length);
uint64_t filesystem_llseek_directory(int32_t fd, uint64_t offset, int whence);

file_descriptor_t* filesystem_duplicate(file_descriptor_t* f);

uint32_t filesystem_close(file_descriptor_t* fd);

#endif
