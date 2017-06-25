#ifndef TERMINAL_H
#define TERMINAL_H

#include "common/types.h"
#include "syscalls/descriptor.h"

// File descriptors
#define STDIN		0
#define STDOUT		1
#define STDERR		2

// Maximum read size
#define MAX_TERMINAL_BUFFER_LENGTH	128

// Initialize terminal
void terminal_init();

// Get a specific screen
uint8_t* get_screen(uint32_t s);

// Initialize the terminal
file_descriptor_t* terminal_open(const char* filename, uint32_t mode);

// Returns a buffered string of input
uint32_t terminal_read(int32_t fd, void* buf, uint32_t bytes);

// Writes a string to the terminal
uint32_t terminal_write(int32_t fd, const void* buf, uint32_t nbytes);

// Get info
uint32_t terminal_stat(int32_t fd, sys_stat_type* data);

// Duplicate the file handle
file_descriptor_t* terminal_duplicate(file_descriptor_t* f);

// Close the terminal (the user shouldn't even call this)
uint32_t terminal_close(file_descriptor_t* fd);

#endif
