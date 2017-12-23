
#include "task.h"
#include <boot/x86_desc.h>
#include <common/lib.h>
#include <memory/memory.h>
#include <drivers/terminal/terminal.h>
#include <drivers/video/video.h>
#include <drivers/pic/i8259.h>
#include <program/loader/elf.h>
#include <syscalls/impl/sysproc.h>
#include <drivers/filesystem/path.h>
#include <common/concurrency/semaphore.h>

#define PIT_IRQ						0

// Start with no tasks
task_list_t* tasks = NULL;
task_list_t* delete_task_list = NULL;
mutex_t task_lock = MUTEX_UNLOCKED;

// Current pcb
pcb_t* current_pcb = NULL;

extern void context_switch_asm(pcb_t* from, pcb_t* to);

// Get the pcb for a specific pid
pcb_t* pcb_from_pid(uint32_t pid) {
	// Walk through the task list
	down(&task_lock);
	task_list_t* t = tasks;
	if (t)
		down(&t->lock);
	up(&task_lock);
	while (t) {
		// Check for a match and return the corresponding pcb
		if (t->pid == pid) {
			up(&t->lock);
			return t->pcb;
		}
		
		task_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	
	// No active pid was found
	return NULL;
}

// Vend the next avaiable pid as a task structure
task_list_t* vend_pid() {
	uint32_t max = 0;
	
	// Special case when there are no tasks running
	down(&task_lock);
	if (!tasks) {
		tasks = (task_list_t*)kmalloc(sizeof(task_list_t));
		tasks->next = NULL;
		tasks->prev = NULL;
		tasks->pid = 0;
		tasks->pcb = NULL;
		tasks->lock = MUTEX_UNLOCKED;
		
		up(&task_lock);
		return tasks;
	}
	
	// Find the max of the list
	task_list_t* t = tasks;
	if (t)
		down(&t->lock);
	up(&task_lock);
	task_list_t* tail = NULL;
	while (t) {
		if (t->pid > max)
			max = t->pid;
		
		// Keep track of the last item in the list
		tail = t;
		
		t = t->next;
		if (t) {
			down(&t->lock);
			up(&tail->lock);
		}
	}
	
	// Add a task to the list
	task_list_t* new_task = (task_list_t*)kmalloc(sizeof(task_list_t));
	new_task->next = NULL;
	new_task->prev = tail;
	new_task->pid = max + 1;
	new_task->pcb = NULL;
	new_task->lock = MUTEX_UNLOCKED;
	tail->next = new_task;
	up(&tail->lock);
	return new_task;
}

// Set the kernel stack for the next task to be switched to
void set_kernel_stack(uint32_t address) {
	tss.esp0 = address;
}

// Copies a task from random blocks into contiguous memory
void copy_task_into_memory(uint32_t start_address, file_descriptor_t* file) {
	// Copy the task into memory
	uint64_t size = fseek(file, uint64_make(0, 0), SEEK_END);
	fseek(file, uint64_make(0, 0), SEEK_SET);
	uint32_t offset = 0;
	uint32_t num = 0;
	while ((num = fread((void*)start_address + offset, 1, size.low, file)) != 0) {
		// Error
		if (num == -1)
			return;
		
		offset += num;
		size = uint64_sub(size, uint64_make(0, num));
	}
}

// Map a task's address space into memory
void map_task_into_memory(pcb_t* pcb) {
	// Unmap whatever is there currently
	vm_unmap_pages(FOUR_MB_SIZE, VM_KERNEL_ADDRESS);
	
	// Map in the process's pages
	page_list_map_list(pcb->page_list, false);
	page_list_map_list(pcb->temporary_mappings, false);
}

// Load the argv and envp portions of a pcb
bool load_argv_and_envp(pcb_t* pcb, const char** argv, const char** envp, uint32_t* argc_out) {
	// Map the new pcb's pages into memory
	page_list_map_list(pcb->page_list, true);
	page_list_map_list(pcb->temporary_mappings, true);
	
	// Copy over the arguments
	uint32_t argc = 0;
	const char** argv_ptr = argv;
	if (argv) {
		while (*argv_ptr) {
			argc++;
			argv_ptr++;
		}
		argv_ptr = argv;
	}
	pcb->argv = (char**)USER_ARGV_LOC;
	memset(pcb->argv, 0, sizeof(char*) * (argc + 1));
	if (argc_out)
		*argc_out = argc;
	
	uint32_t total_ptr = USER_ARGV_LOC + sizeof(char*) * (argc + 1);
	
	int z;
	for (z = 0; z < argc; z++) {
		uint32_t len = strlen(argv[z]);
		pcb->argv[z] = (char*)total_ptr;
		total_ptr += len + 1;
		if (!pcb->argv[z])
			return false;
		memcpy(pcb->argv[z], argv[z], len + 1);
	}
	
	// Copy over the environment
	uint32_t envc = 0;
	const char** envp_ptr = envp;
	bool has_path = false;
	if (envp) {
		while (*envp_ptr) {
			if (strncmp(*envp_ptr, "PATH=", sizeof("PATH=") - 1) == 0)
				has_path = true;
			envc++;
			envp_ptr++;
		}
		envp_ptr = envp;
	}
	if (!has_path)
		envc++;
	pcb->envp = (char**)total_ptr;
	memset(pcb->envp, 0, sizeof(char*) * (envc + 1));
	
	total_ptr += sizeof(char*) * (envc + 1);
	
	for (z = 0; z < envc; z++) {
		const char* str = "";
		if (!has_path && z == envc - 1)
			str = "PATH=/bin:/usr/bin:/usr/local/bin";
		else
			str = envp[z];
		uint32_t len = strlen(str);
		pcb->envp[z] = (char*)total_ptr;
		total_ptr += len + 1;
		if (!pcb->envp[z])
			return false;
		memcpy(pcb->envp[z], str, len + 1);
	}
	
	// Unmap the pcb's pages
	page_list_unmap_list(pcb->page_list, true);
	page_list_unmap_list(pcb->temporary_mappings, true);
	
	if (pcb->parent)
		map_task_into_memory(pcb->parent);
	
	return true;
}

// Helper to free descriptors of a pcb
void pcb_free_descriptors(pcb_t* pcb) {
	uint32_t z;
	down(&pcb->descriptor_lock);
	for (z = 0; z < NUMBER_OF_DESCRIPTORS; z++) {
		if (pcb->descriptors[z]) {
			if ((--pcb->descriptors[z]->ref_count) == 0) {
				pcb->descriptors[z]->close((void*)pcb->descriptors[z]);
				kfree(pcb->descriptors[z]);
			}
			pcb->descriptors[z] = NULL;
		}
	}
	up(&pcb->descriptor_lock);
}

// Add a new child to the parent's children list
bool add_child_task(pcb_t* parent, uint32_t pid, pcb_t* pcb) {
	task_list_t* entry = (task_list_t*)kmalloc(sizeof(task_list_t));
	if (!entry)
		return false;
	
	memset(entry, 0, sizeof(task_list_t));
	entry->next = NULL;
	entry->pid = pid;
	entry->pcb = pcb;
	
	down(&parent->lock);
	task_list_t* child = parent->children;
	task_list_t* prev = NULL;
	while (child) {
		prev = child;
		child = child->next;
	}
	
	entry->prev = prev;
	if (!prev)
		parent->children = entry;
	else
		prev->next = entry;
	up(&parent->lock);
	
	return true;
}

// Copy a task into memory
bool load_task_into_memory(pcb_t* pcb, pcb_t* parent, char* filename) {
	// Map the new pcb's pages into memory
	page_list_map_list(pcb->page_list, true);
	page_list_map_list(pcb->temporary_mappings, true);
	
	// Try different file formats
	bool ret = elf_load(filename, pcb);
	
	// Unmap the pcb's pages
	page_list_unmap_list(pcb->page_list, true);
	page_list_unmap_list(pcb->temporary_mappings, true);
	
	if (parent)
		set_current_task(parent);
	
	return ret;
}

// Helper to copy over the data in a pcb
bool copy_pcb(pcb_t* in, pcb_t* out) {
	memcpy(out, in, USER_KERNEL_STACK_SIZE);
	
	out->working_dir = path_copy(in->working_dir);
	if (!out->working_dir)
		return false;
	
	// Copy over descriptors
	uint32_t z;
	for (z = 0; z < NUMBER_OF_DESCRIPTORS; z++) {
		if (in->descriptors[z]) {
			// Check if this is has already been duplicated
			if (in->descriptors[z]->ref_count > 1) {
				// Check if we've already seen it
				uint32_t i;
				bool found = false;
				for (i = 0; i < z; i++) {
					if (in->descriptors[i] == in->descriptors[z]) {
						found = true;
						break;
					}
				}
				if (found) {
					out->descriptors[z] = out->descriptors[i];
					continue;
				}
			}
			// Otherwise, duplicate it
			out->descriptors[z] = (file_descriptor_t*)in->descriptors[z]->duplicate((void*)in->descriptors[z]);
			if (!out->descriptors[z]) {
				kfree(out->working_dir);
				out->working_dir = NULL;
				return false;
			}
		}
	}
	
	// Update sse registers pointer to be aligned to new registers
	out->sse_registers = (uint8_t*)((uint32_t)out->sse_registers_unaligned + 16 -
									((uint32_t)out->sse_registers_unaligned % 16));
	memcpy(out->sse_registers, in->sse_registers, 512);
		
	return true;
}

// Helper for error function
void pcb_error(pcb_t* pcb, task_list_t* task) {
	if (pcb->working_dir)
		kfree(pcb->working_dir);
	
	// Memory map
	page_list_dealloc(pcb->page_list);
	
	// Dylibs
	dylib_list_dealloc(pcb->dylibs);
	
	// Descriptors
	pcb_free_descriptors(pcb);
	
	// PCB
	kfree(pcb);
	
	// Task
	if (task->prev)
		task->prev->next = task->next;
	kfree(task);
}

// Duplicate current task
pcb_t* duplicate_current_task() {
	pcb_t* current = current_pcb;
	
	// Get a pid and task
	task_list_t* task = vend_pid();
	
	// Reserve a kernel stack and have the pcb be at the top of it
	pcb_t* pcb = (pcb_t*)kmalloc(USER_KERNEL_STACK_SIZE);
	copy_pcb(current, pcb);
	
	// Overwrite the needed things
	pcb->task = task;
	pcb->parent = current;
	pcb->children = NULL;
	pcb->state = SUSPENDED;
	pcb->alarm.val = 0;
	pcb->descriptor_lock = MUTEX_UNLOCKED;
	pcb->lock = MUTEX_UNLOCKED;
	// Clear the current pending signals
	pcb->signal_pending = 0;
	pcb->in_syscall = false;
	pcb->should_terminate = false;
	task->pcb = pcb;
	
	add_child_task(current, task->pid, pcb);
	
	// Copy over the memory used
	pcb->page_list = NULL;
	down(&current->lock);
	page_list_t* n = current->page_list;
	while (n) {
		if (!page_list_add_copy(&pcb->page_list, n)) {
			up(&current->lock);
			pcb_error(pcb, task);
			return NULL;
		}
		
		n = n->next;
	}
	
	pcb->temporary_mappings = NULL;
	n = current->temporary_mappings;
	while (n) {
		if (!page_list_add_copy(&pcb->temporary_mappings, n)) {
			up(&current->lock);
			pcb_error(pcb, task);
			return NULL;
		}
		
		n = n->next;
	}
	
	// Copy the dylib lists too
	dylib_list_t* l = pcb->dylibs;
	pcb->dylibs = NULL;
	while (l) {
		up(&current->lock);
		if (!dylib_list_copy_for_pcb(l, pcb)) {
			pcb_error(pcb, task);
			return NULL;
		}
		
		down(&current->lock);
		l = l->next;
	}
	up(&current->lock);
	
	return pcb;
}

bool setup_stack_and_heap(pcb_t* pcb, uint32_t argc) {
	// Find the last memory mapped region and set the stack to that
	page_list_t* t = pcb->page_list;
	while (t) {
		if (t->vaddr > pcb->stack_address)
			pcb->stack_address = t->vaddr;
		t = t->next;
	}
	// Map another block in for the stack
	page_list_t* stack_block = page_list_get(&pcb->page_list, pcb->stack_address + FOUR_MB_SIZE, MEMORY_WRITE, true);
	if (!stack_block)
		return false;
	pcb->stack_address += FOUR_MB_SIZE;
	page_list_map(stack_block, false);
	
	// The stack grows downward
	pcb->stack_address += USER_STACK_SIZE;
	// The heap grows upwards
	pcb->brk = pcb->stack_address;
	
	// Copy over the argv, argc, envp
	uint32_t* esp = (uint32_t*)(pcb->stack_address - sizeof(uint32_t) * 3);
	esp[2] = (uint32_t)pcb->envp;
	esp[1] = (uint32_t)pcb->argv;
	esp[0] = argc;
	
	return true;
}

// Load a task into memory and initialize its pcb
pcb_t* load_task(char* filename, const char** argv, const char** envp) {
	// Initialize the pcb
	task_list_t* task = vend_pid();
	pcb_t* parent = current_pcb;//(task->pid == INITIAL_TASK_PID) ? NULL : current_pcb;
	
	// Reserve a kernel stack and have the pcb be at the top of it
	pcb_t* pcb = (pcb_t*)kmalloc(USER_KERNEL_STACK_SIZE);
	memset(pcb, 0, sizeof(pcb_t));
	pcb->state = UNLOADED;
	pcb->task = task;
	pcb->parent = parent;
	pcb->sse_registers = (uint8_t*)((uint32_t)pcb->sse_registers_unaligned + 16 -
									((uint32_t)pcb->sse_registers_unaligned % 16));
	pcb->descriptor_lock = MUTEX_UNLOCKED;
	pcb->lock = MUTEX_UNLOCKED;
	
	// Get parent path but ensure we have a leading "/"
	uint32_t wd_len = 0;
	const char* path = path_get_parent(filename, &wd_len);
	int start_path = 0;
	if (path[0] != '/')
		start_path = 1;
	pcb->working_dir = kmalloc(wd_len + 1 + start_path);
	if (!pcb->working_dir) {
		pcb_error(pcb, task);
		return NULL;
	}
	memcpy(&pcb->working_dir[start_path], path, wd_len);
	pcb->working_dir[start_path + wd_len] = 0;
	if (start_path != 0)
		pcb->working_dir[0] = '/';
	
	if (parent) {
		if (!add_child_task(parent, task->pid, pcb)) {
			pcb_error(pcb, task);
			return NULL;
		}
	}
	
	if (!load_task_into_memory(pcb, parent, filename)) {
		pcb_error(pcb, task);
		return NULL;
	}
	
	uint32_t argc = 0;
	if (!load_argv_and_envp(pcb, argv, envp, &argc)) {
		pcb_error(pcb, task);
		return NULL;
	}
	
	if (!setup_stack_and_heap(pcb, argc)) {
		pcb_error(pcb, task);
		return NULL;
	}
	
	// Allow it to be used
	task->pcb = pcb;
	
	return pcb;
}

void copy_task_arguments_free(char* kfile, char** kargv, char** kenvp) {
	if (kfile)
		kfree(kfile);
	
	char** argv_ptr = (char**)kargv;
	if (argv_ptr) {
		while (*argv_ptr) {
			kfree(*argv_ptr);
			argv_ptr++;
		}
	}
	if (kargv)
		kfree(kargv);
	
	char** envp_ptr = (char**)kenvp;
	if (envp_ptr) {
		while (*envp_ptr) {
			kfree(*envp_ptr);
			envp_ptr++;
		}
	}
	if (kenvp)
		kfree(kenvp);
}

// Helper to copy data over to kernel
bool copy_task_arguments(char* filename, const char** argv, const char** envp,
						 char** file_out, char*** argv_out, char*** envp_out) {
	char* kfile = NULL;
	char** kargv = NULL;
	char** kenvp = NULL;
	
	// Filename
	uint32_t filename_len = strlen(filename);
	kfile = (char*)kmalloc(filename_len + 1);
	if (!kfile) {
		copy_task_arguments_free(kfile, kargv, kenvp);
		return false;
	}
	memcpy(kfile, filename, filename_len + 1);
	
	// Argv
	uint32_t argc = 0;
	char** argv_ptr = (char**)argv;
	if (argv) {
		while (*argv_ptr) {
			argc++;
			argv_ptr++;
		}
	}
	kargv = (char**)kmalloc((argc + 1) * sizeof(char*));
	if (!kargv) {
		copy_task_arguments_free(kfile, kargv, kenvp);
		return false;
	}
	memset(kargv, 0, sizeof(char*) * (argc + 1));
	for (uint32_t z = 0; z < argc; z++) {
		uint32_t len = strlen(argv[z]);
		kargv[z] = (char*)kmalloc(len + 1);
		if (!kargv[z]) {
			copy_task_arguments_free(kfile, kargv, kenvp);
			return false;
		}
		memcpy(kargv[z], argv[z], len + 1);
	}
	
	// Envp
	uint32_t envc = 0;
	char** envp_ptr = (char**)envp;
	if (envp) {
		while (*envp_ptr) {
			envc++;
			envp_ptr++;
		}
	}
	kenvp = (char**)kmalloc((envc + 1) * sizeof(char*));
	if (!kenvp) {
		copy_task_arguments_free(kfile, kargv, kenvp);
		return false;
	}
	memset(kenvp, 0, sizeof(char*) * (envc + 1));
	for (uint32_t z = 0; z < envc; z++) {
		uint32_t len = strlen(envp[z]);
		kenvp[z] = (char*)kmalloc(len + 1);
		if (!kenvp[z]) {
			copy_task_arguments_free(kfile, kargv, kenvp);
			return false;
		}
		memcpy(kenvp[z], envp[z], len + 1);
	}
	
	
	// Assign
	*file_out = kfile;
	*argv_out = kargv;
	*envp_out = kenvp;
	
	return true;
}

// Load a task into memory, replacing the current task
pcb_t* load_task_replace(char* filename, const char** argv, const char** envp) {
	bool flags = set_multitasking_enabled(false);

	pcb_t* current = current_pcb;
	
	// Save the task arguments as they get lost when we map out the memory
	char* kfile = NULL;
	char** kargv = NULL, **kenvp = NULL;
	if (!copy_task_arguments(filename, argv, envp, &kfile, &kargv, &kenvp))
		return NULL;
	
	pcb_t pcb = *current;
	
	pcb.page_list = NULL;
	pcb.temporary_mappings = NULL;
	pcb.dylibs = NULL;
	pcb.stack_address = 0;
	pcb.signal_pending = 0;
	pcb.signal_mask = 0;
	pcb.signal_waiting = false;
	pcb.descriptor_lock = MUTEX_UNLOCKED;
	pcb.lock = MUTEX_UNLOCKED;
	memset(pcb.signal_handlers, 0, sizeof(sigaction_t) * NUMBER_OF_SIGNALS);
	
	if (!load_task_into_memory(&pcb, current, kfile)) {
		set_multitasking_enabled(flags);
		copy_task_arguments_free(kfile, kargv, kenvp);
		return NULL;
	}
	
	uint32_t argc = 0;
	if (!load_argv_and_envp(&pcb, (const char**)kargv, (const char**)kenvp, &argc)) {
		set_multitasking_enabled(flags);
		copy_task_arguments_free(kfile, kargv, kenvp);
		return NULL;
	}
	
	copy_task_arguments_free(kfile, kargv, kenvp);
	
	if (!setup_stack_and_heap(&pcb, argc)) {
		set_multitasking_enabled(flags);
		return NULL;
	}
	
	// Free the task memory allocated by this task
	set_current_task(current);
	page_list_dealloc(current->page_list);
	dylib_list_dealloc(current->dylibs);
	
	*current = pcb;
	set_current_task(current);
	
	set_multitasking_enabled(flags);
	
	return current;
}

// Find the next runnable task
pcb_t* get_next_runnable_task(task_list_t* hint) {
	task_list_t* t = hint;
	if (!t) {
		t = tasks;
		if (!t)
			return NULL;
	}
	
	task_list_t* initial = t;
	do {
		pcb_t* pcb = t->pcb;
		if (pcb && pcb->state == READY) {
			// We've found one
			return pcb;
		}
		
		// Loop around
		t = t->next;
		if (!t)
			t = tasks;
	} while (t && t != initial);
	
	return NULL;
}

#define TERMINATE_TASK		1
#define HALT_SYSTEM			0

// Kill a task and return execution to its parent (enables interrupts)
void terminate_task(uint32_t ret) {
#if TERMINATE_TASK
	pcb_t* current = current_pcb;
	down(&current->lock);
	
	// We can't terminate a task in a syscall, so allow the syscall to finish
	if (current->in_syscall) {
		current->should_terminate = true;
		up(&current->lock);
		return;
	}
	current->should_terminate = false;
	
	// Unload the task and get its parent
	pcb_t* parent = current->parent;
	
	// Mark this task as a zombie
	if (parent) {
		task_list_t* child = parent->children;
		while (child) {
			if (child->pcb == current) {
				child->pcb = NULL;
				child->return_value = ret;
				break;
			}
			child = child->next;
		}
		
		// Send the child finish signal
		if (!(parent->signal_handlers[SIGCHLD].flags & SA_NOCLDSTOP))
			signal_send(parent, SIGCHLD);
	}
	
	// Delete the children
	task_list_t* child = current->children;
	current->children = NULL;
	while (child) {
		task_list_t* prev = child;
		if (child->pcb)
			child->pcb->parent = NULL;	// TODO: maybe this should be the parent's parent?
		child = child->next;
		
		kfree(prev);
	}
	
	if (current->working_dir)
		kfree(current->working_dir);
	
	// Free the task memory and dylibsallocated by the deleted task
	page_list_t* page_list = current->page_list;
	page_list_t* temp_list = current->temporary_mappings;
	current->page_list = NULL;
	current->temporary_mappings = NULL;
	page_list_dealloc(page_list);
	page_list_dealloc(temp_list);
	
	dylib_list_t* dylibs = current->dylibs;
	current->dylibs = NULL;
	dylib_list_dealloc(dylibs);
	
	// Descriptors
	pcb_free_descriptors(current);
	
	current->argv = NULL;
	current->envp = NULL;
	
	up(&current->lock);
	
	/*// Find next possible task to run
	pcb_t* next_pcb = get_next_runnable_task(hint);
	if (!next_pcb) {
		// Resume the parent as the last resort
		next_pcb = parent;
#if DEBUG
		if (!next_pcb) {
			blue_screen("No runnable tasks. :(");
			return;
		}
#endif

	}
	
	set_current_task(next_pcb);
	if (next_pcb->state == READY)
		next_pcb->state = RUNNING;*/
	
	// Unlink the task from the task list (so therefore cannot be scheduled)
	task_list_t* task = current->task;
	if (task->prev)
		down(&task->prev->lock);
	down(&task->lock);
	if (task->next)
		down(&task->next->lock);
	cli();
	if (task->prev)
		task->prev->next = task->next;
	if (task->next)
		task->next->prev = task->prev;
	if (task == tasks)
		tasks = task->next;
	if (task->prev)
		up(&task->prev->lock);
	up(&task->lock);
	if (task->next)
		up(&task->next->lock);
	
	// Queue this task to be deleted
	task->next = delete_task_list;
	task->prev = NULL;
	if (delete_task_list)
		delete_task_list->prev = task;
	delete_task_list = task;
	
	schedule();
	
	// TODO: temp should be a run(tasks->pcb)
	if (tasks && tasks->pcb)
		context_switch(NULL, tasks->pcb);
	
#ifdef DEBUG
	// Should never get here
	blue_screen("No runnable tasks. :(");
#else
	// TODO: temp - Load a shell
	pcb_t* next = NULL;
	if (queue_task_load("shell", NULL, NULL, &next) == -1)
		blue_screen("No runnable tasks. :(");
	run(next);
#endif
#elif HALT_SYSTEM
	cli();
	printf("Halting due to an exception.\n");
	asm volatile(".stop: hlt; jmp .stop;");
#endif
}

// Maps memory so that this task is loaded
void set_current_task(pcb_t* pcb) {
	map_task_into_memory(pcb);
	set_kernel_stack((uint32_t)pcb + USER_KERNEL_STACK_SIZE);
	
	current_pcb = pcb;
	descriptors = pcb->descriptors;
}

extern void disable_sse();

// Switch from one task to another (automatically enables interrupts)
void context_switch(pcb_t* from, pcb_t* to) {
	// Don't bother switching if they are the same
	if (from && from == to && !signal_pending(from)) {
		sti();
		return;
	}
	
	cli();
	
	// Save SSE registers if needed
	if (from && from->sse_used) {
		asm volatile(" fxsave (%0); "::"r"(from->sse_registers));
		disable_sse();
		from->sse_used = false;
	}
	
	// Map the "to" task back into memory and set its parameters
	set_current_task(to);
	
	if (from)
		from->state = READY;
	// TODO: this is just for kernel shell test, also will never happen in real life
	if (to->state != UNLOADED)
		to->state = RUNNING;
	
	// This part auto enables interrupts
	context_switch_asm(from, to);
}

// Run the scheduler (gets called up PIT interrupt)
void schedule() {
	// This whole function is a critical section
	uint32_t flags;
	cli_and_save(flags);

	// Check for any tasks that need to be deallocated
	task_list_t* d = delete_task_list;
	while (d) {
		if (d->pcb)
			kfree(d->pcb);
		task_list_t* next = d->next;
		kfree(d);
		d = next;
	}
	delete_task_list = NULL;
	
	// If we have disabled multitasking, don't do anything
	if (!irq_enabled(PIT_IRQ)) {
		restore_flags(flags);
		return;
	}
	
	pcb_t* current = current_pcb;
	
	// If this task is supposed to terminate, terminate it
	if (current && current->should_terminate) {
		restore_flags(flags);
		current->should_terminate = false;
		terminate_task(1);
		return;
	}
	
	task_list_t* hint = NULL;
	if (current)
		hint = current->task->next;
	pcb_t* next_pcb = get_next_runnable_task(hint);
	
	// If there are no runnable tasks, do nothing (unless this task has signals pending)
	if (!next_pcb) {
		if (current && signal_pending(current))
			next_pcb = current;
		else {
			restore_flags(flags);
			return;
		}
	}
	
	// Swtich to the new task (automatically reenables interrupts)
	if (next_pcb->state != UNLOADED)
		context_switch(current, next_pcb);
	else {
		if (current)
			current->state = READY;
		run_with_fake_parent(next_pcb, current);
	}
}

// Returns whether multitasking was enabled
bool set_multitasking_enabled(bool enable) {
	// This is a critical section
	uint32_t flags;
	cli_and_save(flags);
	
	// We can enable or disable interrupts by masking the PIT interrupt on the pic
	bool ret = irq_enabled(PIT_IRQ);
	if (enable)
		enable_irq(PIT_IRQ);
	else
		disable_irq(PIT_IRQ);
	
	restore_flags(flags);
	return ret;
}

