#ifndef TASK_H
#define TASK_H

#include <common/types.h>
#include <syscalls/descriptor.h>
#include <drivers/filesystem/filesystem.h>
#include <memory/memory.h>
#include <drivers/terminal/terminal.h>
#include <drivers/filesystem/ext2/ext2.h>
#include <program/signal.h>
#include <memory/page_list.h>
#include <memory/mmap_list.h>
#include <program/dylib.h>

#define USER_KERNEL_STACK_SIZE		(1024 * 8)			// 8 KB
#define USER_STACK_SIZE				(1024 * 16)			// 16 KB
#define USER_ARGV_LOC				0x8000000
#define USER_ADDRESS				0x8000000

#define INITIAL_TASK_PID			1
#define MAIN_THREAD_TID				1

typedef enum {
	UNLOADED = 0,					// Context is not on the stack
	SUSPENDED,						// Not ready to execute (contents on stack)
	READY,							// Ready to execute (contents on stack)
	RUNNING,						// Currently executing
	FINISHED,						// Done executing and waiting for info to be read
} thread_state;

struct pcb;

// Linked List for keeping track of all the tasks
typedef struct task_list {
	struct task_list* next;
	struct task_list* prev;
	
	// Info about the task
	uint32_t pid;
	struct pcb* pcb;
	uint32_t return_value;
	
	mutex_t lock;
} task_list_t;

// List for all the running tasks
extern task_list_t* tasks;

typedef struct thread {
	// The esp to return to when resuming a thread (must be first parameter, 0)
	uint32_t saved_esp;
	// Stack address (second, 4)
	uint32_t stack_address;
	// Currently in a syscall (third, 8)
	bool in_syscall;
	
	context_state_t	context;
	struct pcb* pcb;
	uint32_t tid;
	thread_state state;
	thread_state backup_state;
	
	semaphore_t lock;
	
	struct thread* next;
	struct thread* prev;
	
	// SSE / FPU instructions
	uint8_t sse_registers_unaligned[512 + 16];
	uint8_t* sse_registers;
	bool sse_used;
	bool sse_init;
} thread_t;

// The current thread
extern thread_t* current_thread;

// Structure to hold information per process
// This contains all the info that a task needs to operated correctly.
typedef struct pcb {
	// Where to start execution from (must be first, 0)
	uint32_t entry;
	// If this is set, the task will terminate next time it is loaded (second, 4)
	bool should_terminate;
	// Set if a signal has occurred (third, 5)
	bool signal_occurred;
	
	// The threads
	thread_t* threads;
	
	// Overall state of the task
	thread_state state;
	
	// Each entry in the list holds the physical address of the
	// corresponding 4MB page starting at 0x8000000
	page_list_t* page_list;
	uint32_t brk;
	// For use with preserving context mappings (see vm_map_page)
	page_list_t* temporary_mappings;
	// List of user mapped mmap regions
	mmap_list_t* user_mappings;
	
	// The file descriptors for this task
	file_descriptor_t* descriptors[NUMBER_OF_DESCRIPTORS];
	mutex_t descriptor_lock;
	mutex_t lock;
	
	// Parent and child process
	struct pcb* parent;
	// Note: an entry with pcb == NULL means the child has finished
	// but the result has not been waited for by waitpid (i.e. in
	// zombie state).
	task_list_t* children;
	
	// Arguments passed to main
	char** argv;
	char** envp;
	
	// Working directory
	char* working_dir;
	
	// Associated task
	task_list_t* task;
	
	// Signal things
	sigset_t signal_pending;
	sigset_t signal_mask;
	sigset_t signal_save_mask[NUMBER_OF_SIGNALS];
	sigaction_t signal_handlers[NUMBER_OF_SIGNALS];
	volatile bool signal_waiting;
	time_t alarm;
	
	// Dynamic objects
	dylib_list_t* dylibs;
} pcb_t;

// The current pcb
extern pcb_t* current_pcb;

// Create a thread for the given pcb
thread_t* thread_create(pcb_t* pcb);

// Create a copy of a thread
thread_t* thread_copy(thread_t* t, pcb_t* new_pcb, bool exact);

// Gets the pcb for a pid
pcb_t* pcb_from_pid(uint32_t pid);

// Sets the kernel stack in the TSS
void set_kernel_stack(uint32_t address);

// Duplicate current task (fork)
pcb_t* duplicate_current_task();

// Load a task into memory
pcb_t* load_task(char* filename, const char** argv, const char** envp);

// Load a task into memory, replacing the current task
pcb_t* load_task_replace(char* filename, const char** argv, const char** envp);

// Unload a task from page mappings
pcb_t* unload_current_task();

// Terminate the task and free its memory
void terminate_task(uint32_t ret);

// Maps a task's page directory into memory
// Note: must be called with multitasking disabled
void map_task_into_memory(pcb_t* pcb);

// Maps memory so that this task is loaded (optionally disables interrupts)
void set_current_task(pcb_t* pcb, thread_t* thread);

// Switch from one task to another
void context_switch(thread_t* from, thread_t* to);

// Handle a single scheduler tick
void scheduler_tick();

// Run the scheduler
void schedule();

// Alternative to cli/sti (for using files still)
// Returns whether multitasking was enabled
bool set_multitasking_enabled(bool enable);

#endif
