//
//  sysproc.c
//  NeilOS
//
//  Created by Neil Singh on 6/25/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "sysproc.h"
#include <common/types.h>
#include <program/task.h>
#include <common/log.h>

// Returns to a user space program
extern void return_to_user(pcb_t* pcb, pcb_t* parent);
// Gets the context of the machine right before this function call is executed
extern void get_context(context_state_t* context);
// What to return to from a child forking
extern void fork_return(context_state_t context);

// Initialize the descriptors for a pcb
void init_descriptors(pcb_t* pcb) {
	// Open default descriptors
	descriptors = pcb->descriptors;
	pcb->descriptors[STDIN] = terminal_open("stdin", FILE_MODE_READ);
	pcb->descriptors[STDIN]->ref_count = 1;
	pcb->descriptors[STDOUT] = terminal_open("stdout", FILE_MODE_WRITE);
	pcb->descriptors[STDOUT]->ref_count = 1;
	pcb->descriptors[STDERR] = terminal_open("stderr", FILE_MODE_WRITE);
	pcb->descriptors[STDERR]->ref_count = 1;
	
	descriptors = pcb->descriptors;
}

// Queue a task in the currently loaded task's spot
int32_t queue_task(const char* cmd, const char** argv, const char** envp, pcb_t** ret) {
	pcb_t* pcb = load_task_replace((char*)cmd, argv, envp);
	
	// If we couldn't load the task, let the caller know
	if (!pcb)
		return -1;
	
	// Pass along this value
	if (ret)
		*ret = pcb;
	
	return 0;
}

// Queue a task in a new spot
int32_t queue_task_load(const char* cmd, const char** argv, const char** envp, pcb_t** ret) {
	pcb_t* pcb = load_task((char*)cmd, argv, envp);
	
	// If we couldn't load the task, let the caller know
	if (!pcb)
		return -1;
	
	init_descriptors(pcb);
	
	// Pass along this value
	if (ret)
		*ret = pcb;
	
	return 0;
}

// Run a program
int32_t run(pcb_t* pcb) {
	return run_with_fake_parent(pcb, pcb->parent);
}

// Run a program but save the state of this "parent" program
int32_t run_with_fake_parent(pcb_t* pcb, pcb_t* parent) {
	bool flags = set_multitasking_enabled(false);
	set_current_task(pcb);
	
	// Critical section that ends with return to user
	cli();
	set_multitasking_enabled(flags);
	
	pcb->state = RUNNING;
	// Execute the program (and enable interrupts)
	return_to_user(pcb, parent);
	
	// We return here when the program finishes executing
	
	// Get the value of eax as our return (passed from the halt command)
	uint32_t ret = 0;
	asm volatile ("movl %%eax, %0"
				  : "=d"(ret));
	
	return ret;
}

// Helper to save the current pcb context
void pcb_set_context(context_state_t state) {
	current_pcb->context = state;
}

// Duplicate a program and memory space
uint32_t fork() {
	LOG_DEBUG_INFO();

	pcb_t* pcb = NULL;
	if ((pcb = duplicate_current_task()) == NULL)
		return -1;
	
	// Set the context to return to fork_return() with eax set to 0 so
	// the child returns 0. Also move the esp from the current pcb kernel stack
	// to this new process's kernel stack and copy over the context for fork_return().
	pcb->context.eax = 0;
	// This points to where the iret info is
	// We need info for the context switch (popa, ret)
	// This ret should point to fork_return, which is just iret.
	// So just add in a context state followed by fork_return's address
	pcb->saved_esp = pcb->context.esp + (uint32_t)pcb - (uint32_t)current_pcb -
						sizeof(uint32_t) - sizeof(context_state_t);
	char* esp = (char*)pcb->saved_esp;
	memcpy(esp, &pcb->context, sizeof(context_state_t));
	uint32_t addr = (uint32_t)fork_return;
	memcpy(&esp[sizeof(context_state_t)], &addr, sizeof(uint32_t));
	pcb->in_syscall = false;
		
	// Enable the child to be scheduled
	pcb->state = READY;
	
	return pcb->task->pid;
}

// Load a new program into the current memory space
uint32_t execve(const char* filename, const char* argv[], const char* envp[]) {
	LOG_DEBUG_INFO_STR("(%s, 0x%x, 0x%x)", filename, argv, envp);
	
	// Load the task
	pcb_t* pcb = NULL;
	if (queue_task(filename, argv, envp, &pcb) != 0)
		return -1;
	
	run_with_fake_parent(pcb, NULL);
	return 0;
}

// Get the pid of the current process
uint32_t getpid() {
	LOG_DEBUG_INFO();

	return current_pcb->task->pid;
}

// Options for waitpid
#define WNOHANG					1
#define WUNTRACED				2

// Wait for a child to change state (returns the status that the child returned with)
uint32_t waitpid(uint32_t pid, int* status, int options) {
	LOG_DEBUG_INFO_STR("(%d)", pid);

	pcb_t* pcb = current_pcb;
	
	// Wait until the child has finished
	while (!pcb->should_terminate) {
		bool found = false;
		task_list_t* child = pcb->children;
		while (child) {
			if (child->pid == pid || pid == -1 || pid == 0) {
				found = true;
				// Child is a zombie if child->pcb == NULL
				if (!child->pcb) {
					// Remove child
					uint32_t ret = child->return_value;
					if (child->prev)
						child->prev->next = child->next;
					if (child->next)
							child->next->prev = child->prev;
					if (child == pcb->children)
						pcb->children = child->next;
					kfree(child);
					
					// Fill out the status argument if needed
					if (status) {
						// TODO: add signal stopping and core dump stopping, etc
						*status = ((ret & 0xFF) << 8);
					}
					
					return ret;
				}
			}
			child = child->next;
		}
		// If the pid does not exist, return error
		if (!found || (options & WNOHANG))
			return -1;
		
		schedule();
	}
	
	// Should terminate
	return -1;
}

// Exit a program with a specific status
uint32_t exit(int status) {
	LOG_DEBUG_INFO_STR("(%d)", status);

	pcb_t* pcb = current_pcb;
	
	// Close all open file handles
	int z;
	for (z = 0; z < NUMBER_OF_DESCRIPTORS; z++) {
		if (descriptors[z]) {
			if ((--descriptors[z]->ref_count) == 0) {
				descriptors[z]->close(descriptors[z]);
				kfree(descriptors[z]);
			}
			pcb->descriptors[z] = NULL;
		}
	}
	
	descriptors = pcb->descriptors;
	
	// Terminate the task
	pcb->in_syscall = false;
	terminate_task(status);
	
	return status;
}

// Get the current working directory
uint32_t getwd(char* buf) {
	LOG_DEBUG_INFO_STR("(0x%x)", buf);

	// TODO: implement working directories
	if (!buf)
		return -1;
	memcpy(buf, "/", strlen("/") + 1);
	return 0;
}

// Change the current working directory
uint32_t chdir(const char* path) {
	LOG_DEBUG_INFO_STR("(%s)", path);

	return 0;
}
