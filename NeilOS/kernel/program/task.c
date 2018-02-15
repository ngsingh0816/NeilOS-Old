
#include "task.h"
#include <boot/x86_desc.h>
#include <common/lib.h>
#include <memory/memory.h>
#include <drivers/terminal/terminal.h>
#include <drivers/pic/i8259.h>
#include <program/loader/elf.h>
#include <syscalls/impl/sysproc.h>
#include <drivers/filesystem/path.h>
#include <common/concurrency/semaphore.h>

#define PIT_IRQ						0

// Start with no tasks
task_list_t* tasks = NULL;
mutex_t task_lock = MUTEX_UNLOCKED;

// Current pcb / thread
pcb_t* current_pcb = NULL;
thread_t* current_thread = NULL;

extern void context_switch_asm(thread_t* from, thread_t* to);

// Cleanup memory for threads
void thread_list_dealloc(thread_t** list) {
	thread_t* t = *(list);
	*(list) = NULL;
	if (t)
		down(&t->lock);
	while (t) {
		thread_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
		kfree(prev);
	}
}

// Copy a thread
thread_t* thread_copy(thread_t* t, pcb_t* new_pcb, bool exact) {
	thread_t* n = (thread_t*)kmalloc(USER_KERNEL_STACK_SIZE);
	if (!n)
		return NULL;
	
	memcpy(n, t, exact ? USER_KERNEL_STACK_SIZE : sizeof(thread_t));
	n->lock = MUTEX_UNLOCKED;
	n->next = NULL;
	n->prev = NULL;
	n->pcb = new_pcb;
	n->sse_registers = (uint8_t*)((uint32_t)n->sse_registers_unaligned + 16 -
								  ((uint32_t)n->sse_registers_unaligned % 16));
	
	return n;
}

// Copy a list of threads
bool thread_list_copy(thread_t** new, thread_t* old, pcb_t* new_pcb, bool exact) {
	thread_t* head = NULL;
	thread_t* tail = NULL;
	thread_t* t = old;
	down(&t->lock);
	while (t) {
		thread_t* n = thread_copy(t, new_pcb, exact);
		if (!n) {
			thread_list_dealloc(&head);
			return NULL;
		}
		
		thread_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
		
		if (!head) {
			head = n;
			tail = n;
		} else {
			tail->next = n;
			n->prev = tail;
			tail = n;
		}
	}
	
	(*new) = head;
	return true;
}

thread_t* thread_create(pcb_t* pcb) {
	thread_t* t = (thread_t*)kmalloc(USER_KERNEL_STACK_SIZE);
	if (!t)
		return NULL;
	memset(t, 0, sizeof(thread_t));
	uint32_t tid = 0;
	
	// Find an available tid
	thread_t* p = pcb->threads;
	if (p)
		down(&p->lock);
	while (p) {
		if (p->tid > tid)
			tid = p->tid;
		
		thread_t* prev = p;
		p = p->next;
		if (p)
			down(&p->lock);
		up(&prev->lock);
	}
	
	t->tid = tid + 1;
	t->pcb = pcb;
	t->lock = MUTEX_UNLOCKED;
	t->sse_registers = (uint8_t*)((uint32_t)t->sse_registers_unaligned + 16 -
								  ((uint32_t)t->sse_registers_unaligned % 16));
	t->state = READY;
	
	return t;
}

// Create the default, main thread
thread_t* thread_create_main(pcb_t* pcb) {
	thread_t* t = (thread_t*)kmalloc(USER_KERNEL_STACK_SIZE);
	if (!t)
		return NULL;
	memset(t, 0, sizeof(thread_t));
	t->tid = MAIN_THREAD_TID;
	t->pcb = pcb;
	t->lock = MUTEX_UNLOCKED;
	t->sse_registers = (uint8_t*)((uint32_t)t->sse_registers_unaligned + 16 -
								  ((uint32_t)t->sse_registers_unaligned % 16));
	t->state = READY;
	
	return t;
}

// Restore the state of all threads
void thread_list_restore_state(thread_t* list) {
	thread_t* t = list;
	if (t)
		down(&t->lock);
	while (t) {
		t->state = t->backup_state;
		thread_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
}

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
		tasks->pid = INITIAL_TASK_PID;
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
	page_list_map_list(pcb->page_list, false);
	page_list_map_list(pcb->temporary_mappings, false);
	
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
	bool has_path = false, has_term = false;
	if (envp) {
		while (*envp_ptr) {
			if (strncmp(*envp_ptr, "PATH=", sizeof("PATH=") - 1) == 0)
				has_path = true;
			else if (strncmp(*envp_ptr, "TERM=", sizeof("TERM=") - 1) == 0)
				has_term = true;
			envc++;
			envp_ptr++;
		}
		envp_ptr = envp;
	}
	uint32_t added_evnp = 0;
	if (!has_path) {
		envc++;
		added_evnp++;
	}
	if (!has_term) {
		envc++;
		added_evnp++;
	}
	pcb->envp = (char**)total_ptr;
	memset(pcb->envp, 0, sizeof(char*) * (envc + 1));
	
	total_ptr += sizeof(char*) * (envc + 1);
	
	for (z = 0; z < envc; z++) {
		const char* str = "";
		if (!has_path && z == envc - added_evnp) {
			str = "PATH=/bin:/usr/bin:/usr/local/bin";
			added_evnp--;
			has_path = true;
		}
		else if (!has_term && z == envc - added_evnp) {
			str = "TERM=" TERMINAL_NAME;
			added_evnp--;
			has_term = true;
		}
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
	page_list_unmap_list(pcb->page_list, false);
	page_list_unmap_list(pcb->temporary_mappings, false);
	
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

// Copy a task into memory (warning: modifies current_pcb = pcb)
bool load_task_into_memory(pcb_t* pcb, char* filename) {
	// Try different file formats
	bool ret = elf_load(filename, pcb);
	
	return ret;
}

// Helper to copy over the data in a pcb
bool copy_pcb(pcb_t* in, pcb_t* out) {
	memcpy(out, in, sizeof(pcb_t));
	
	out->working_dir = path_copy(in->working_dir);
	if (!out->working_dir)
		return false;
	
	// Copy over descriptors
	down(&in->descriptor_lock);
	uint32_t z;
	for (z = 0; z < NUMBER_OF_DESCRIPTORS; z++) {
		if (in->descriptors[z]) {
			down(&in->descriptors[z]->lock);
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
					up(&in->descriptors[z]->lock);
					continue;
				}
			}
			// Otherwise, duplicate it
			out->descriptors[z] = (file_descriptor_t*)in->descriptors[z]->duplicate((void*)in->descriptors[z]);
			up(&in->descriptors[z]->lock);
			if (!out->descriptors[z]) {
				kfree(out->working_dir);
				out->working_dir = NULL;
				up(&current_pcb->descriptor_lock);
				return false;
			}
		}
	}
	up(&in->descriptor_lock);
		
	return true;
}

// Helper for error function
void pcb_error(pcb_t* pcb, task_list_t* task) {
	// Task
	down(&task_lock);
	if (task->next)
		task->next->prev = task->prev;
	if (task->prev)
		task->prev->next = task->next;
	up(&task_lock);
	kfree(task);
	
	if (pcb->working_dir)
		kfree(pcb->working_dir);
	
	// Threads
	thread_list_dealloc(&pcb->threads);
	
	// Memory map
	if (pcb->page_list)
		page_list_dealloc(pcb->page_list);
	if (pcb->temporary_mappings)
		page_list_dealloc(pcb->temporary_mappings);
	if (pcb->user_mappings)
		mmap_list_dealloc_list(pcb->user_mappings);
	
	// Dylibs
	if (pcb->dylibs)
		dylib_list_dealloc(pcb->dylibs);
	
	// Descriptors
	pcb_free_descriptors(pcb);
	
	// PCB
	kfree(pcb);
}

// Duplicate current task
pcb_t* duplicate_current_task() {
	pcb_t* current = current_pcb;
	
	// Get a pid and task
	task_list_t* task = vend_pid();
	
	// Reserve a kernel stack and have the pcb be at the top of it
	pcb_t* pcb = (pcb_t*)kmalloc(sizeof(pcb_t));
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
	pcb->should_terminate = false;
	pcb->signal_occurred = false;
	
	add_child_task(current, task->pid, pcb);
	
	pcb->page_list = NULL;
	pcb->temporary_mappings = NULL;
	pcb->dylibs = NULL;
	pcb->threads = NULL;
	task->pcb = pcb;
	
	// Copy over the memory used
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
	
	n = current->temporary_mappings;
	while (n) {
		if (!page_list_add_copy(&pcb->temporary_mappings, n)) {
			up(&current->lock);
			pcb_error(pcb, task);
			return NULL;
		}
		
		n = n->next;
	}
	
	if (current->user_mappings && !(pcb->user_mappings = mmap_list_copy(current->user_mappings))) {
		up(&current->lock);
		pcb_error(pcb, task);
		return NULL;
	}
	
	// Copy the dylib lists too
	dylib_list_t* l = current->dylibs;
	while (l) {
		if (!dylib_list_copy_for_pcb(l, pcb)) {
			up(&current->lock);
			pcb_error(pcb, task);
			return NULL;
		}
		
		l = l->next;
	}
	
	// Copy over threads
	thread_t* t = current->threads;
	if (!thread_list_copy(&pcb->threads, t, pcb, true)) {
		up(&current->lock);
		pcb_error(pcb, task);
		return NULL;
	}
	
	up(&current->lock);
	
	return pcb;
}

bool setup_stack_and_heap(pcb_t* pcb, uint32_t argc) {
	// Find the last memory mapped region and set the stack to that
	page_list_t* t = pcb->page_list;
	while (t) {
		if (t->vaddr > pcb->threads->stack_address)
			pcb->threads->stack_address = t->vaddr;
		t = t->next;
	}
	// Map another block in for the stack
	page_list_t* stack_block = page_list_get(&pcb->page_list,
											 pcb->threads->stack_address + FOUR_MB_SIZE, MEMORY_RW, true);
	if (!stack_block)
		return false;
	pcb->threads->stack_address += FOUR_MB_SIZE;
	page_list_map(stack_block, false);
	
	// The stack grows downward
	pcb->threads->stack_address += USER_STACK_SIZE;
	// The heap grows upwards
	pcb->brk = pcb->threads->stack_address;
	
	// Copy over the argv, argc, envp
	uint32_t* esp = (uint32_t*)(pcb->threads->stack_address - sizeof(uint32_t) * 3);
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
	pcb_t* pcb = (pcb_t*)kmalloc(sizeof(pcb_t));
	memset(pcb, 0, sizeof(pcb_t));
	pcb->threads = thread_create_main(pcb);
	if (!pcb->threads) {
		kfree(pcb);
		return NULL;
	}
	
	set_current_task(pcb, pcb->threads);
	pcb->state = UNLOADED;
	pcb->task = task;
	pcb->parent = parent;
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
		set_current_task(parent, parent ? parent->threads : NULL);
		return NULL;
	}
	memcpy(&pcb->working_dir[start_path], path, wd_len);
	pcb->working_dir[start_path + wd_len] = 0;
	if (start_path != 0)
		pcb->working_dir[0] = '/';
	
	if (parent) {
		if (!add_child_task(parent, task->pid, pcb)) {
			pcb_error(pcb, task);
			set_current_task(parent, parent ? parent->threads : NULL);
			return NULL;
		}
	}
	
	if (!load_task_into_memory(pcb, filename)) {
		pcb_error(pcb, task);
		set_current_task(parent, parent ? parent->threads : NULL);
		return NULL;
	}
	current_pcb = parent;
	current_thread = parent ? parent->threads : NULL;
	
	uint32_t argc = 0;
	if (!load_argv_and_envp(pcb, argv, envp, &argc)) {
		pcb_error(pcb, task);
		set_current_task(parent, parent ? parent->threads : NULL);
		return NULL;
	}
	
	if (!setup_stack_and_heap(pcb, argc)) {
		pcb_error(pcb, task);
		set_current_task(parent, parent ? parent->threads : NULL);
		return NULL;
	}
	
	// Initialize the dylibs
	pcb_t* backup_pcb = current_pcb;
	thread_t* backup_thread = current_thread;
	current_pcb = pcb;
	current_thread = pcb->threads;
	page_list_map_list(pcb->page_list, false);
	dylib_list_perform_init(pcb->dylibs->next);
	current_pcb = backup_pcb;
	current_thread = backup_thread;
	
	if (parent)
		map_task_into_memory(pcb->parent);
	
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
	// Save the task arguments as they get lost when we map out the memory
	char* kfile = NULL;
	char** kargv = NULL, **kenvp = NULL;
	if (!copy_task_arguments(filename, argv, envp, &kfile, &kargv, &kenvp))
		return NULL;
	
	down(&current_pcb->lock);
	pcb_t backup = *current_pcb;
	thread_t backup_thread = *current_thread;
	
	// Suspend all threads but this one
	thread_t* t = current_pcb->threads;
	if (t)
		down(&t->lock);
	while (t) {
		t->backup_state = t->state;
		if (t != current_thread)
			t->state = SUSPENDED;
		
		thread_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	
	current_pcb->signal_pending = 0;
	current_pcb->signal_mask = 0;
	current_pcb->signal_waiting = false;
	current_pcb->signal_occurred = false;
	current_pcb->descriptor_lock = MUTEX_UNLOCKED;
	current_pcb->lock = MUTEX_LOCKED;
	memset(current_pcb->signal_handlers, 0, sizeof(sigaction_t) * NUMBER_OF_SIGNALS);
	page_list_t* list = current_pcb->page_list;
	current_pcb->page_list = NULL;
	page_list_t* tlist = current_pcb->temporary_mappings;
	current_pcb->temporary_mappings = NULL;
	mmap_list_t* maps = current_pcb->user_mappings;
	current_pcb->user_mappings = NULL;
	dylib_list_t* dylibs = current_pcb->dylibs;
	current_pcb->dylibs = NULL;
	thread_t* threads = current_pcb->threads;
	
	if (!load_task_into_memory(current_pcb, kfile)) {
		*current_pcb = backup;
		thread_list_restore_state(current_pcb->threads);
		copy_task_arguments_free(kfile, kargv, kenvp);
		up(&current_pcb->lock);
		return NULL;
	}
	
	uint32_t argc = 0;
	if (!load_argv_and_envp(current_pcb, (const char**)kargv, (const char**)kenvp, &argc)) {
		kfree(current_pcb->threads);
		page_list_dealloc(current_pcb->page_list);
		*current_pcb = backup;
		*current_thread = backup_thread;
		thread_list_restore_state(current_pcb->threads);
		copy_task_arguments_free(kfile, kargv, kenvp);
		up(&current_pcb->lock);
		return NULL;
	}
	
	copy_task_arguments_free(kfile, kargv, kenvp);
	
	current_thread->stack_address = 0;
	if (!setup_stack_and_heap(current_pcb, argc)) {
		kfree(current_pcb->threads);
		page_list_dealloc(current_pcb->page_list);
		*current_pcb = backup;
		*current_thread = backup_thread;
		thread_list_restore_state(current_pcb->threads);
		up(&current_pcb->lock);
		return NULL;
	}
	
	// Initialize the dylibs (TODO: perform this in userspace)
	page_list_map_list(current_pcb->page_list, false);
	up(&current_pcb->lock);
	dylib_list_perform_init(current_pcb->dylibs->next);
	down(&current_pcb->lock);
	
	// Make the current thread the only thread
	thread_t* next = current_thread->next, *prev = current_thread->prev;
	current_thread->next = NULL;
	current_pcb->threads = current_thread;
	current_thread->prev = NULL;
	if (prev)
		prev->next = next;
	else
		threads = next;
	
	// Free old memory
	thread_list_dealloc(&threads);
	page_list_dealloc(list);
	page_list_dealloc(tlist);
	mmap_list_dealloc_list(maps);
	dylib_list_dealloc(dylibs);
	
	up(&current_pcb->lock);
	
	map_task_into_memory(current_pcb);
	
	return current_pcb;
}

#define TERMINATE_TASK		1
#define HALT_SYSTEM			0

// Kill a task and return execution to its parent (enables interrupts)
void terminate_task(uint32_t ret) {
#if TERMINATE_TASK
	pcb_t* current = current_pcb;
	
	// We can't terminate a task in a syscall, so allow the syscall to finish
	// We suspend all threads besides this one so we can safely deallocate everything in this current thread
	bool in_syscall = false;
	thread_t* t = current->threads;
	if (t)
		down(&t->lock);
	while (t) {
		if (t->in_syscall)
			in_syscall = true;
		
		thread_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	if (in_syscall) {
		current->should_terminate = true;
		return;
	}
	
	down(&current_pcb->lock);
	
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
	
	// Free the task memory and dylibs allocated by the deleted task
	page_list_t* page_list = current->page_list;
	page_list_t* temp_list = current->temporary_mappings;
	mmap_list_t* maps = current->user_mappings;
	current->page_list = NULL;
	current->temporary_mappings = NULL;
	page_list_dealloc(page_list);
	page_list_dealloc(temp_list);
	mmap_list_dealloc_list(maps);
	
	dylib_list_t* dylibs = current->dylibs;
	current->dylibs = NULL;
	dylib_list_dealloc(dylibs);
	
	// Descriptors
	pcb_free_descriptors(current);
	
	current->argv = NULL;
	current->envp = NULL;
	
	// Unlink the task from the task list (so therefore cannot be scheduled)
	task_list_t* task = current->task;
	if (task->prev)
		down(&task->prev->lock);
	down(&task->lock);
	if (task->next)
		down(&task->next->lock);
	heap_perform_lock();
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
	
	// Delete the task and threads
	heap_perform_unlock();
	thread_list_dealloc(&current->threads);
	if (current)
		kfree(current);
	kfree(task);
	
	current_pcb = NULL;
	current_thread = NULL;
	
	schedule();
	
	// TODO: temp should be a run(tasks->pcb)
	if (tasks && tasks->pcb)
		context_switch(NULL, tasks->pcb->threads);
	
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
void set_current_task(pcb_t* pcb, thread_t* thread) {
	pcb_t* backup = current_pcb;
	current_pcb = pcb;
	current_thread = thread;
	
	if (pcb) {
		set_kernel_stack((uint32_t)thread + USER_KERNEL_STACK_SIZE);
		if (pcb != backup) {
			map_task_into_memory(pcb);
			descriptors = pcb->descriptors;
		}
	}
}

extern void disable_sse();

// Switch from one thread to another (automatically enables interrupts)
void context_switch(thread_t* from, thread_t* to) {
	// Don't bother switching if they are the same
	if (from && from == to && !signal_pending(from->pcb)) {
		enable_irq(PIT_IRQ);
		sti();
		return;
	}
	
	// Save SSE registers if needed
	if (from && from->sse_used) {
		asm volatile(" fxsave (%0); "::"r"(from->sse_registers));
		disable_sse();
		from->sse_used = false;
	}
	
	// Map the "to" task back into memory and set its parameters
	set_current_task(to->pcb, to);
	
	if (from) {
		if (from->state != FINISHED)
			from->state = READY;
		from->pcb->state = READY;
	}
	// TODO: this is just for kernel shell test, also will never happen in real life
	if (to->state != UNLOADED) {
		to->state = RUNNING;
		to->pcb->state = RUNNING;
	}
	
	// This part auto enables interrupts
	cli();
	enable_irq(PIT_IRQ);
	context_switch_asm(from, to);
}

// Find the next runnable thread
thread_t* get_next_runnable_thread(thread_t* hint) {
	thread_t* t = hint;
	
	if (t) {
		// Make sure that out initial thread is still valid
		task_list_t* s = tasks;
		bool found = false;
		while (s) {
			if (s->pcb == t->pcb) {
				found = true;
				break;
			}
			s = s->next;
		}
		if (!found) {
			t = NULL;
			if (tasks && tasks->pcb)
				t = tasks->pcb->threads;
			if (!t)
				return NULL;
		}
	}
	else {
		if (tasks && tasks->pcb)
			t = tasks->pcb->threads;
		if (!t)
			return NULL;
	}
	
	pcb_t* pcb = t->pcb;
	thread_t* initial = t;
	do {
		if (t->state == READY && (t->pcb->state == READY || t->pcb->state == RUNNING))
			return t;
		thread_t* prev = t;
		t = t->next;
		task_list_t* next_task = prev->pcb->task->next;
		while (!t) {
			pcb = (next_task && next_task->pcb) ? next_task->pcb : tasks->pcb;
			t = pcb->threads;
			if (next_task)
				next_task = next_task->next;
		}
	} while (t != initial);
	
	if ((t->state == READY || t->state == RUNNING) &&
		  (t->pcb->state == READY || t->pcb->state == RUNNING))
		return t;
	return NULL;
}

// Handle a single scheduler tick
void scheduler_tick() {
	static int counter = 0;
	// Increment the current time and perform scheduling if needed
	time_increment_ms(1);
	
	counter++;
	if (counter == 20) {
		counter = 0;
		schedule();
	}
}

// Run the scheduler (gets called up PIT interrupt)
void schedule() {
	// If we have disabled multitasking, don't do anything
	if (!irq_enabled(PIT_IRQ))
		return;
	disable_irq(PIT_IRQ);
	
	// If this task is supposed to terminate, terminate it
	if (current_pcb && current_pcb->should_terminate) {
		uint32_t flags = 0;
		cli_and_save(flags);
		enable_irq(PIT_IRQ);
		if (down_trylock(&current_pcb->lock)) {
			current_pcb->should_terminate = false;
			terminate_task(1);
			// Will only return here if we failed
			up(&current_pcb->lock);
		}
		restore_flags(flags);
	}
	
	// If the only runnable task is the same one, do nothing (unless this task has signals pending)
	thread_t* next_thread = get_next_runnable_thread(current_thread);
	if (next_thread == current_thread) {
		if (current_pcb && signal_pending(current_pcb))
			next_thread = current_thread;
		else {
			enable_irq(PIT_IRQ);
			return;
		}
	} else if (!next_thread) {
		// TODO: If there are no threads that can run, we can turn off the cpu and save energy
		enable_irq(PIT_IRQ);
		return;
	}
	
	// Swtich to the new thread (automatically reenables interrupts)
	context_switch(current_thread, next_thread);
	
	// This is where we will get context switched into
	// Handle signals if needed
	signal_handle(current_pcb);
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

