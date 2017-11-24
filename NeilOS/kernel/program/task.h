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
#include <program/dylib.h>

#define USER_KERNEL_STACK_SIZE		(1024 * 8)			// 8 KB
#define USER_STACK_SIZE				(1024 * 16)			// 16 KB
#define USER_ARGV_LOC				0x8000000
#define USER_ADDRESS				0x8000000

#define INITIAL_TASK_PID			0

typedef enum {
	UNLOADED = 0,					// Context is not on the stack
	SUSPENDED,						// Not ready to execute (contents on stack)
	READY,							// Ready to execute (contents on stack)
	RUNNING,						// Currently executing
} task_state;

struct pcb;

// Linked List for keeping track of all the tasks
typedef struct task_list {
	struct task_list* next;
	struct task_list* prev;
	
	// Info about the task
	uint32_t pid;
	struct pcb* pcb;
	uint32_t return_value;
} task_list_t;

// List for all the running tasks
extern task_list_t* tasks;

// Structure to hold information per process
// This contains all the info that a task needs to operated correctly.
typedef struct pcb {
	// The esp to return to when resume a task (must be first parameter, 0)
	uint32_t saved_esp;
	// Where to start execution from (must be second, 4)
	uint32_t entry;
	// Stack address (third, 8)
	uint32_t stack_address;
	// Currently in a syscall (fourth, 12)
	bool in_syscall;
	// If this is set, the task will terminate next time it is loaded (fifth, 13)
	bool should_terminate;
	
	// The saved context of the task
	context_state_t	context;
	
	// Each entry in the list holds the physical address of the
	// corresponding 4MB page starting at 0x8000000
	page_list_t* page_list;
	uint32_t brk;
	
	// The file descriptors for this task
	file_descriptor_t* descriptors[NUMBER_OF_DESCRIPTORS];
	
	// Parent and child process
	struct pcb* parent;
	// Note: an entry with pcb == NULL means the child has finished
	// but the result has not been waited for by waitpid (i.e. in
	// zombie state).
	task_list_t* children;
	
	// Arguments passed to main
	char** argv;
	char** envp;
	
	// Associated task
	task_list_t* task;
	
	// Whether this task is currently running or ready to run
	task_state state;
	
	// Signal things
	sigset_t signal_pending;
	sigset_t signal_mask;
	sigset_t signal_save_mask[NUMBER_OF_SIGNALS];
	sigaction_t signal_handlers[NUMBER_OF_SIGNALS];
	volatile bool signal_waiting;
	time_t alarm;
	
	// Dynamic objects
	dylib_list_t* dylibs;
	
	// SSE / FPU instructions
	uint8_t sse_registers_unaligned[512 + 16];
	uint8_t* sse_registers;
	bool sse_used;
	bool sse_init;
} pcb_t;

// The current pcb
extern pcb_t* current_pcb;

// Gets the childmost pcb for the active terminal
pcb_t* get_pcb_for_terminal(uint32_t terminal);

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
void set_current_task(pcb_t* pcb);

// Switch from one task to another
void context_switch(pcb_t* from, pcb_t* to);

// Run the scheduler (gets called up PIT interrupt)
void schedule();

// Alternative to cli/sti (for using files still)
// Returns whether multitasking was enabled
bool set_multitasking_enabled(bool enable);

#endif
