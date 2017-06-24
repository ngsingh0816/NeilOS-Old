#ifndef SYSCALL_H
#define SYSCALL_H

#include <drivers/filesystem/filesystem.h>
#include <common/types.h>
#include <common/time.h>
#include <program/task.h>

// Devices
void initialize_syscall_devices();

// Helpers for execute
int32_t queue_task_load(const char* cmd, const char** argv, const char** envp, pcb_t** ret);
int32_t run(pcb_t* pcb);
int32_t run_with_fake_parent(pcb_t* pcb, pcb_t* parent);

// Program level syscalls

// Duplicate a program and memory space
uint32_t fork();

// Load a new program into the current memory space
uint32_t execve(const char* filename, const char* argv[], const char* envp[]);

// Get the pid of the current process
uint32_t getpid();

// Wait for a child to change state (returns the status that the child returned with)
uint32_t waitpid(uint32_t pid);

// Wait for any child to finish
uint32_t wait(uint32_t pid);

// Exit a program with a specific status
uint32_t exit(int status);

// File syscalls

// Helper for open
file_descriptor_t* open_handle(const char* filename, uint32_t mode);
// Open a file descriptor
uint32_t open(const char* filename, uint32_t mode);

// Read from a file descriptor
uint32_t read(int32_t fd, void* buf, uint32_t nbytes);

// Write to a file descriptor
uint32_t write(int32_t fd, const void* buf, uint32_t nbytes);

// Seek to an offset
uint32_t llseek(int32_t fd, uint64_t offset, int whence);

// Change the size of a file
uint32_t truncate(int32_t fd, uint64_t length);

// Get information about a file descriptor
uint32_t stat(int32_t fd, sys_stat_type* data);

// Close a file descriptor
uint32_t close(int32_t fd);

// Is the file descriptor a terminal?
uint32_t isatty(int32_t fd);

// Filesystem syscalls

// Make a directory
uint32_t mkdir(const char* name);

// Hard link a file
uint32_t link(const char* filename, const char* new_name);

// Unlink a file (note: deletes the file if it is the last remaining link)
uint32_t unlink(const char* filename);

// Memory operations

// Set the program break to a specific address
void* brk(uint32_t addr);

// Offset the current program break by a specific ammount
void* sbrk(int32_t offset);

// File descriptors

// Duplicate a file descriptor
uint32_t dup(uint32_t fd);

// Duplicate a file descriptor into the specific file descriptor
uint32_t dup2(uint32_t fd, uint32_t new_fd);

// Time
// Get the timing information for a process
uint32_t times(sys_time_type* data);

// Returns the time of day in seconds
uint32_t gettimeofday();

// Signals

// Send a signal to a process
uint32_t kill(uint32_t pid, uint32_t signum);

// Set the signal handler for a certain signal
uint32_t signal(uint32_t signum, sighandler_t handler);

// Mask signals
uint32_t sigsetmask(uint32_t signum, bool masked);

// Unmask signals
uint32_t siggetmask(uint32_t signum);

// Pipes

// Sockets

// Misc

#endif
