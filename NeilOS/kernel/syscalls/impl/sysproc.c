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
#include <syscalls/interrupt.h>
#include <drivers/filesystem/path.h>
#include <boot/x86_desc.h>

typedef struct {
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t esp;
	uint32_t ss;
} iret_t;


// Returns to a user space program
extern void return_to_user(thread_t* thread, pcb_t* pcb, thread_t* parent_thread);
// Gets the context of the machine right before this function call is executed
extern void get_context(context_state_t* context);
// What to return to from a child forking
extern void fork_return(context_state_t context);

// Helper code to perform an implicit call to thread_exit()
extern void implicit_thread_exit_start();
extern void implicit_thread_exit_end();

// Initialize the descriptors for a pcb
void init_descriptors(pcb_t* pcb) {
	// Open default descriptors
	descriptors = pcb->descriptors;
	down(&pcb->descriptor_lock);
	pcb->descriptors[STDIN] = terminal_open("stdin", FILE_MODE_READ);
	pcb->descriptors[STDIN]->ref_count = 1;
	pcb->descriptors[STDOUT] = terminal_open("stdout", FILE_MODE_WRITE);
	pcb->descriptors[STDOUT]->ref_count = 1;
	pcb->descriptors[STDERR] = terminal_open("stderr", FILE_MODE_WRITE);
	pcb->descriptors[STDERR]->ref_count = 1;
	up(&pcb->descriptor_lock);
	
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
	// Critical section that ends with return to user
	cli();
	set_current_task(pcb, pcb->threads);
	
	pcb->state = RUNNING;
	pcb->threads->state = RUNNING;
	// Execute the program (and enable interrupts)
	return_to_user(pcb->threads, pcb, parent ? parent->threads : NULL);
	
	// Should never get here
	return 0;
}

// Helper to save the current thread context
void thread_set_context(context_state_t state) {
	down(&current_thread->lock);
	current_thread->context = state;
	up(&current_thread->lock);
}

// Duplicate a program and memory space
uint32_t fork() {
	LOG_DEBUG_INFO();
	
	pcb_t* pcb = NULL;
	if ((pcb = duplicate_current_task()) == NULL)
		return -ENOMEM;
	
	// Set the context to return to fork_return() with eax set to 0 so
	// the child returns 0. Also move the esp from the current threads' kernel stacks
	// to this new threads' kernel stacks and copy over the context for fork_return().
	
	// Find the equivalent thread to the current thread in this new task
	thread_t* t = pcb->threads;
	thread_t* o = current_pcb->threads;
	thread_t* thread = NULL;
	while (t) {
		t->saved_esp = t->context.esp + (uint32_t)t - (uint32_t)o;
		// This points to where the iret info is
		// We need info for the context switch (popa, ret)
		// This ret should point to fork_return, which is just iret.
		// So just add in a context state followed by fork_return's address
		if (t->tid == current_thread->tid) {
			t->context.eax = 0;
			thread = t;
		} else
			t->context.eax = -1;
		t->saved_esp -= sizeof(uint32_t) + sizeof(context_state_t);
		char* esp = (char*)t->saved_esp;
		memcpy(esp, &t->context, sizeof(context_state_t));
		uint32_t addr = (uint32_t)fork_return;
		memcpy(&esp[sizeof(context_state_t)], &addr, sizeof(uint32_t));
		t->in_syscall = false;
		t->state = READY;
		t = t->next;
		o = o->next;
	}
		
	// Enable the child to be scheduled
	pcb->state = READY;
	
	// Jump into the new task
	context_switch(current_thread, thread);
	
	return pcb->task->pid;
}

// Load a new program into the current memory space
uint32_t execve(const char* filename, const char* argv[], const char* envp[]) {
	LOG_DEBUG_INFO_STR("(%s, 0x%x, 0x%x)", filename, argv, envp);
	
	char* path = path_absolute(filename, current_pcb->working_dir);
	
	// Load the task
	pcb_t* pcb = NULL;
	if (queue_task(path, argv, envp, &pcb) != 0) {
		kfree(path);
		return -ENOENT;
	}
	kfree(path);
	
	run_with_fake_parent(pcb, NULL);
	return 0;
}

// Get the pid of the current process
uint32_t getpid() {
	LOG_DEBUG_INFO();

	return current_pcb->task->pid;
}

// Get the pid of the parent process
uint32_t getppid() {
	LOG_DEBUG_INFO();
	
	if (!current_pcb->parent)
		return current_pcb->task->pid;
	return current_pcb->parent->task->pid;
}

// Options for waitpid
#define WNOHANG					1
#define WUNTRACED				2

// Wait for a child to change state (returns the status that the child returned with)
uint32_t waitpid(uint32_t pid, int* status, int options) {
	LOG_DEBUG_INFO_STR("(%d)", pid);

	pcb_t* pcb = current_pcb;
	
	down(&pcb->lock);

	// If there are no children, return that
	if (!pcb->children) {
		up(&pcb->lock);
		return -ECHILD;
	}
	
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
					uint32_t cpid = child->pid;
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
					
					up(&pcb->lock);
					
					return cpid;
				}
			}
			child = child->next;
		}
		up(&pcb->lock);

		// If the pid does not exist, return error
		if (!found)
			return -ECHILD;
		else if (options & WNOHANG)
			return 0;
		
		schedule();
		
		down(&pcb->lock);
	}
	
	up(&pcb->lock);
	
	// Should terminate
	return -ECHILD;
}

// Exit a program with a specific status
uint32_t exit(int status) {
	LOG_DEBUG_INFO_STR("(%d)", status);

	pcb_t* pcb = current_pcb;
	
	down(&pcb->descriptor_lock);
	
	// Close all open file handles
	int z;
	for (z = 0; z < NUMBER_OF_DESCRIPTORS; z++) {
		if (descriptors[z]) {
			down(&descriptors[z]->lock);
			if ((--descriptors[z]->ref_count) == 0) {
				descriptors[z]->close(descriptors[z]);
				up(&descriptors[z]->lock);
				kfree(descriptors[z]);
			} else
				up(&descriptors[z]->lock);
			pcb->descriptors[z] = NULL;
		}
	}
	up(&pcb->descriptor_lock);
	
	// Terminate the task
	current_thread->in_syscall = false;
	terminate_task(status);
	
	return status;
}

// Get the current working directory
uint32_t getwd(char* buf) {
	LOG_DEBUG_INFO_STR("(0x%x)", buf);

	if (!buf)
		return -EFAULT;
	down(&current_pcb->lock);
	memcpy(buf, current_pcb->working_dir, strlen(current_pcb->working_dir) + 1);
	up(&current_pcb->lock);
	
	return 0;
}

// Change the current working directory
uint32_t chdir(const char* path) {
	LOG_DEBUG_INFO_STR("(%s)", path);

	pcb_t* pcb = current_pcb;
	down(&pcb->lock);
	char* p = path_absolute(path, pcb->working_dir);
	if (pcb->working_dir)
		kfree(pcb->working_dir);
	pcb->working_dir = p;
	if (!pcb->working_dir) {
		up(&pcb->lock);
		return -ENOMEM;
	}
	up(&pcb->lock);
	
	return 0;
}

// Create a new thread
uint32_t sys_thread_create(void (*func)(), void* user_stack) {
	// Create an exact copy of this thread
	thread_t* t = thread_create(current_pcb);
	if (!t)
		return -ENOMEM;
	
	// Add this thread in
	t->state = SUSPENDED;
	t->stack_address = (uint32_t)user_stack;
	t->context = current_thread->context;
	
	// This points to where the iret info is
	// We need info for the context switch (popa, ret)
	// This ret should point to fork_return, which is just an iret.
	// So just add in a context state followed by fork_return's address, followed by iret info
	// We also need to modify the iret's esp to be the user stack.
	t->context.eax = 0;
	t->saved_esp = (uint32_t)t + USER_KERNEL_STACK_SIZE;
	t->saved_esp -= sizeof(uint32_t) + sizeof(context_state_t) + sizeof(iret_t);
	char* esp = (char*)t->saved_esp;
	memcpy(esp, &t->context, sizeof(context_state_t));
	uint32_t addr = (uint32_t)fork_return;
	memcpy(&esp[sizeof(context_state_t)], &addr, sizeof(uint32_t));
	iret_t* iret = (iret_t*)&esp[sizeof(context_state_t) + sizeof(uint32_t)];
	iret->ss = *(uint32_t*)(t->context.esp+0x10);
	// Also push an implicit call to thread_exit() on the stack
	uint32_t func_size = (uint32_t)implicit_thread_exit_end - (uint32_t)implicit_thread_exit_start;
	iret->esp = (uint32_t)user_stack - sizeof(uint32_t) - func_size;
	uint32_t implicit_eip = iret->esp + sizeof(uint32_t);
	memcpy((void*)iret->esp, &implicit_eip, sizeof(uint32_t));
	memcpy((void*)(iret->esp + sizeof(uint32_t)), (void*)implicit_thread_exit_start, func_size);
	iret->eflags = *(uint32_t*)(t->context.esp+0x8);
	iret->cs = *(uint32_t*)(t->context.esp+0x4);
	iret->eip = (uint32_t)func;
	t->in_syscall = false;
	
	// Link in the thread
	thread_t* p = current_pcb->threads;
	if (p)
		down(&p->lock);
	thread_t* prev = NULL;
	while (p) {
		prev = p;
		p = p->next;
		if (p) {
			down(&p->lock);
			up(&prev->lock);
		}
	}
	if (prev) {
		t->prev = prev;
		prev->next = t;
		up(&prev->lock);
	}
	
	// Enable the new thread to be scheduled
	t->state = READY;
	
	// Jump into the new thread
	context_switch(current_thread, t);
	
	return t->tid;
}

// Get current thread id
uint32_t gettid() {
	return current_thread->tid;
}

// Wait for a thread to finish
uint32_t thread_wait(uint32_t tid) {
	// Loop through all the threads and find a match
	down(&current_pcb->lock);
	while (!current_pcb->should_terminate) {
		bool found = false;
		thread_t* t = current_pcb->threads;
		if (t)
			down(&t->lock);
		up(&current_pcb->lock);
		thread_t* prev = NULL;
		while (t) {
			if (t->tid == tid) {
				found = true;
				if (t->state == FINISHED) {
					// Remove this thread
					thread_t* next = t->next;
					if (next)
						down(&next->lock);
					if (prev) {
						prev->next = next;
						up(&prev->lock);
					}
					if (next) {
						next->prev = prev;
						up(&next->lock);
					}
					up(&t->lock);
					kfree(t);
					return 0;
				}
				
				if (t->prev)
					up(&t->prev->lock);
				up(&t->lock);
				
				schedule();
				break;
			}
			
			// This is to prevent two threads waiting on the same thread to remove the thread
			// at the same time
			thread_t* dprev = t->prev;
			prev = t;
			t = t->next;
			if (t)
				down(&t->lock);
			else
				up(&prev->lock);
			if (dprev)
				up(&dprev->lock);
		}
		if (!found)
			return -ECHILD;
		
		down(&current_pcb->lock);
	}
	up(&current_pcb->lock);
	
	return 0;
}

// Exit a thread
uint32_t thread_exit() {
	down(&current_thread->lock);
	current_thread->state = FINISHED;
	current_thread->in_syscall = false;
	up(&current_thread->lock);
	
	// If this is the last thread, then exit
	down(&current_pcb->lock);
	thread_t* t = current_pcb->threads;
	if (t)
		down(&t->lock);
	up(&current_pcb->lock);
	bool valid = false;
	while (t) {
		if (t->state != FINISHED) {
			valid = true;
			up(&t->lock);
			break;
		}
		
		thread_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	if (!valid)
		exit(0);
	
	// Schedule out of here
	for (;;)
		schedule();
	
	return 0;
}
