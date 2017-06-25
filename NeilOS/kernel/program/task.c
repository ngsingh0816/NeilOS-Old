
#include "task.h"
#include <boot/x86_desc.h>
#include <common/lib.h>
#include <memory/memory.h>
#include <drivers/terminal/terminal.h>
#include <drivers/video/video.h>
#include <drivers/pic/i8259.h>
#include <program/loader/elf.h>
#include <syscalls/impl/sysproc.h>

#define PIT_IRQ						0

// Start with no tasks
task_list_t* tasks = NULL;

extern void context_switch_asm(pcb_t* from, pcb_t* to);

// Get the pcb for a specific pid
pcb_t* pcb_from_pid(uint32_t pid) {
	// Walk through the task list
	task_list_t* t = NULL;
	for (t = tasks; t != NULL; t = t->next) {
		// Check for a match and return the corresponding pcb
		if (t->pid == pid)
			return t->pcb;
	}
	
	// No active pid was found
	return NULL;
}

// Vend the next avaiable pid as a task structure
task_list_t* vend_pid() {
	uint32_t max = 0;
	
	// Special case when there are no tasks running
	if (!tasks) {
		tasks = (task_list_t*)kmalloc(sizeof(task_list_t));
		tasks->next = NULL;
		tasks->prev = NULL;
		tasks->pid = 0;
		tasks->pcb = NULL;
		
		return tasks;
	}
	
	// Find the max of the list
	task_list_t* t = NULL;
	task_list_t* tail = t;
	for (t = tasks; t != NULL; t = t->next) {
		if (t->pid > max)
			max = t->pid;
		
		// Keep track of the last item in the list
		tail = t;
	}
	
	// Add a task to the list
	task_list_t* new_task = (task_list_t*)kmalloc(sizeof(task_list_t));
	new_task->next = NULL;
	new_task->prev = tail;
	new_task->pid = max + 1;
	new_task->pcb = NULL;
	tail->next = new_task;
	return new_task;
}

// Returns the current process control block for the current task
pcb_t* get_current_pcb() {
	/*// Get the esp
	uint32_t esp;
	asm volatile ("movl %%esp, %0\n"
				  : "=a"(esp));
	
	// Find the pcb from all the tasks
	task_list_t* t = NULL;
	for (t = tasks; t != NULL; t = t->next) {
		if (esp >= (uint32_t)t->pcb && esp < (uint32_t)t->pcb + USER_KERNEL_STACK_SIZE)
			return t->pcb;
	}
	
	// Should never happen (unless in kernel so we could just return any task)
	if (tasks)
		return tasks->pcb;
	else
		return NULL;*/
	
	// Find the pcb from all the tasks
	task_list_t* t;
	for (t = tasks; t != NULL; t = t->next) {
		if (!t->pcb)
			continue;
		if (t->pcb->state == RUNNING)
			return t->pcb;
	}
	
	// Should never happen
	//if (tasks)
	//	return tasks->pcb;
	return NULL;
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

// Map a tasks address space into memory
void map_task_into_memory(pcb_t* pcb) {
	// Unmap whatever is there currently
	uint32_t z;
	for (z = 0; z < 2 * ONE_GB_SIZE; z += FOUR_MB_SIZE)
		vm_unmap_page(z + USER_ADDRESS);
	// Loop through all the pages in the memory list
	memory_list_t* t = pcb->memory_map;
	while (t) {
		vm_map_page(t->vaddr, t->paddr, USER_PAGE_DIRECTORY_ENTRY);
		t = t->next;
	}
	flush_tlb();
}

// Load the argv and envp portions of a pcb
bool load_argv_and_envp(pcb_t* pcb, const char** argv, const char** envp, uint32_t* argc_out) {
	bool flags = set_multitasking_enabled(false);

	map_task_into_memory(pcb);
	
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
		if (!pcb->argv[z]) {
			set_multitasking_enabled(flags);
			return false;
		}
		memcpy(pcb->argv[z], argv[z], len + 1);
	}
	
	// Copy over the environment
	uint32_t envc = 0;
	const char** envp_ptr = envp;
	if (envp) {
		while (*envp_ptr) {
			envc++;
			envp_ptr++;
		}
		envp_ptr = envp;
	}
	pcb->envp = (char**)total_ptr;
	memset(pcb->envp, 0, sizeof(char*) * (envc + 1));
	
	total_ptr += sizeof(char*) * (envc + 1);
	
	for (z = 0; z < envc; z++) {
		uint32_t len = strlen(envp[z]);
		pcb->envp[z] = (char*)total_ptr;
		total_ptr += len + 1;
		if (!pcb->envp[z]) {
			set_multitasking_enabled(flags);
			return false;
		}
		memcpy(pcb->envp[z], envp[z], len + 1);
	}
	
	if (pcb->parent)
		map_task_into_memory(pcb->parent);
	
	set_multitasking_enabled(flags);

	return true;
}

// Helper to free descriptors of a pcb
void pcb_free_descriptors(pcb_t* pcb) {
	uint32_t z;
	for (z = 0; z < NUMBER_OF_DESCRIPTORS; z++) {
		if (pcb->descriptors[z]) {
			if ((--pcb->descriptors[z]->ref_count) == 0) {
				pcb->descriptors[z]->close((void*)pcb->descriptors[z]);
				kfree(pcb->descriptors[z]);
			}
			pcb->descriptors[z] = NULL;
		}
	}
}

// Add a new child to the parent's children list
bool add_child_task(pcb_t* parent, uint32_t pid, pcb_t* pcb) {
	task_list_t* child = parent->children;
	task_list_t* prev = NULL;
	while (child) {
		prev = child;
		child = child->next;
	}
	task_list_t* entry = (task_list_t*)kmalloc(sizeof(task_list_t));
	if (!entry)
		return false;
	
	memset(entry, 0, sizeof(task_list_t));
	entry->next = NULL;
	entry->prev = prev;
	entry->pid = pid;
	entry->pcb = pcb;
	
	if (!prev)
		parent->children = entry;
	else
		prev->next = entry;
	
	return true;
}

// Copy a task into memory
bool load_task_into_memory(pcb_t* pcb, pcb_t* parent, char* filename) {
	bool flags = set_multitasking_enabled(false);
	map_task_into_memory(pcb);
	
	// Try different file formats
	if (!elf_load_into_memory(filename, pcb)) {
		if (parent)
			map_task_into_memory(parent);
		return false;
	}
	
	if (parent)
		map_task_into_memory(parent);
	set_multitasking_enabled(flags);
	
	return true;
}

// Helper to copy over the data in a pcb
bool copy_pcb(pcb_t* in, pcb_t* out) {
	memcpy(out, in, USER_KERNEL_STACK_SIZE);
	
	// Copy over descriptors
	uint32_t z;
	for (z = 0; z < NUMBER_OF_DESCRIPTORS; z++) {
		if (in->descriptors[z]) {
			out->descriptors[z] = (file_descriptor_t*)in->descriptors[z]->duplicate((void*)in->descriptors[z]);
			if (!out->descriptors[z])
				return false;
		}
	}
		
	return true;
}

// Helper for error function
void pcb_error(pcb_t* pcb, task_list_t* task) {
	// Memory map
	memory_list_t* t = pcb->memory_map;
	while (t) {
		memory_list_t* prev = t;
		physical_page_free((void*)t->paddr);
		t = t->next;
		kfree(prev);
	}
	
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
	pcb_t* current = get_current_pcb();
	
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
	// Clear the current pending signals
	pcb->signal_pending = 0;
	task->pcb = pcb;
	
	add_child_task(current, task->pid, pcb);
	
	// Copy over the memory used
	// TODO: make this point to the same thing, and copy on write
	bool flags = set_multitasking_enabled(false);
	
	pcb->memory_map = NULL;
	memory_list_t** t = &pcb->memory_map;
	memory_list_t* prev = NULL;
	memory_list_t* n = current->memory_map;
	while (n) {
		*t = kmalloc(sizeof(memory_list_t));
		if (!(*t)) {
			set_multitasking_enabled(flags);
			pcb_error(pcb, task);
			return NULL;
		}
		
		memset(*t, 0, sizeof(memory_list_t));
		
		void* page = page_get_four_mb(1, VIRTUAL_MEMORY_USER);
		if (!page) {
			kfree(*t);
			set_multitasking_enabled(flags);
			pcb_error(pcb, task);
			return NULL;
		}
		memcpy(page, (void*)n->vaddr, FOUR_MB_SIZE);
		(*t)->vaddr = n->vaddr;
		(*t)->paddr = (uint32_t)vm_virtual_to_physical((uint32_t)page);
		vm_unmap_page((uint32_t)page);
		
		(*t)->prev = prev;
		prev = (*t);
		t = &((*t)->next);
		n = n->next;
	}
		   
	set_multitasking_enabled(flags);
	
	return pcb;
}

bool add_memory_block(memory_list_t* list, memory_list_t* prev, uint32_t vaddr) {
	list->next = NULL;
	list->vaddr = (uint32_t)page_get_four_mb(1, VIRTUAL_MEMORY_USER);
	if (!list->vaddr)
		return false;
	
	list->paddr = (uint32_t)vm_virtual_to_physical(list->vaddr);
	vm_unmap_page(list->vaddr);
	list->vaddr = vaddr;
	list->prev = prev;
	if (prev)
		prev->next = list;
	
	return true;
}

bool setup_stack_and_heap(pcb_t* pcb, uint32_t argc) {
	// Find the last memory mapped region and set the stack to that
	memory_list_t* t = pcb->memory_map;
	memory_list_t* prev = NULL;
	while (t) {
		if (t->vaddr > pcb->stack_address)
			pcb->stack_address = t->vaddr;
		prev = t;
		t = t->next;
	}
	// Map another block in for the stack
	memory_list_t* stack_block = (memory_list_t*)kmalloc(sizeof(memory_list_t));
	if (!stack_block) {
		kfree(stack_block);
		return false;
	}
	pcb->stack_address += FOUR_MB_SIZE;
	bool flags = set_multitasking_enabled(false);
	if (!add_memory_block(stack_block, prev, pcb->stack_address)) {
		set_multitasking_enabled(flags);
		return false;
	}
	set_multitasking_enabled(flags);
	
	vm_map_page(stack_block->vaddr, stack_block->paddr, USER_PAGE_DIRECTORY_ENTRY);
	
	// The stack grows downward
	pcb->stack_address += USER_STACK_SIZE;
	// The heap grows upwards
	pcb->brk = pcb->stack_address;
	
	// Copy over the argv and argc
	uint32_t* esp = (uint32_t*)(pcb->stack_address - sizeof(uint32_t) * 2);
	esp[1] = (uint32_t)pcb->argv;
	esp[0] = argc;
	
	return true;
}

// Load a task into memory and initialize its pcb
pcb_t* load_task(char* filename, const char** argv, const char** envp) {
	// Initialize the pcb
	task_list_t* task = vend_pid();
	pcb_t* parent = get_current_pcb();//(task->pid == INITIAL_TASK_PID) ? NULL : get_current_pcb();
	
	// Reserve a kernel stack and have the pcb be at the top of it
	pcb_t* pcb = (pcb_t*)kmalloc(USER_KERNEL_STACK_SIZE);
	memset(pcb, 0, sizeof(pcb_t));
	pcb->state = UNLOADED;
	pcb->task = task;
	pcb->parent = parent;
	
	if (parent) {
		if (!add_child_task(parent, task->pid, pcb)) {
			pcb_error(pcb, task);
			return NULL;
		}
	}
	
	// Create the tasks's memory map
	bool flags = set_multitasking_enabled(false);
	pcb->memory_map = kmalloc(sizeof(memory_list_t));
	if (!pcb->memory_map) {
		pcb_error(pcb, task);
		set_multitasking_enabled(flags);
		return NULL;
	}
	if (!add_memory_block(pcb->memory_map, NULL, USER_ADDRESS)) {
		pcb_error(pcb, task);
		set_multitasking_enabled(flags);
		return NULL;
	}
	set_multitasking_enabled(flags);
	
	uint32_t argc = 0;
	if (!load_argv_and_envp(pcb, argv, envp, &argc)) {
		pcb_error(pcb, task);
		return NULL;
	}
	
	if (!load_task_into_memory(pcb, parent, filename)) {
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

// Load a task into memory, replacing the current task
pcb_t* load_task_replace(char* filename, const char** argv, const char** envp) {
	pcb_t* pcb = get_current_pcb();
	// Unlink
	pcb->task->pcb = NULL;
	
	pcb->state = UNLOADED;
	
	uint32_t argc = 0;
	if (!load_argv_and_envp(pcb, argv, envp, &argc))
		return NULL;
	
	if (!load_task_into_memory(pcb, pcb->parent, filename))
		return NULL;
	
	if (!setup_stack_and_heap(pcb, argc))
		return NULL;
	
	// Relink
	pcb->task->pcb = pcb;
	
	return pcb;
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
	pcb_t* current = get_current_pcb();
	
	// Don't allow us to kill the first task
	if (!current->parent) {
		run(current);
		return;
	}
	
	// We can't terminate a task in a syscall, so allow the syscall to finish
	if (current->in_syscall) {
		current->should_terminate = true;
		return;
	}
	
	// Critical section
	cli();
	
	task_list_t* hint = current->task->next;
	
	// Free the task memory allocated by this task
	memory_list_t* t = current->memory_map;
	while (t) {
		page_free((void*)t->vaddr);
		memory_list_t* temp = t->next;
		kfree(t);
		t = temp;
	}
	
	// Unload the task and get its parent
	current->state = UNLOADED;
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
		signal_send(parent, SIGCHILD);
	}
	
	// Unlink the task from the task list
	task_list_t* task = current->task;
	if (task->prev)
		task->prev->next = task->next;
	else
		tasks = task->next;
	
	kfree(task);
	
	// Delete the children
	task_list_t* child = current->children;
	while (child) {
		task_list_t* prev = child;
		child = child->next;
		
		kfree(prev);
	}
	
	// Descriptors
	pcb_free_descriptors(current);
	
	current->argv = NULL;
	current->envp = NULL;
	
	// Free the kernel stack and pcb
	kfree(current);
	
	// Find next possible task to run
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
		next_pcb->state = RUNNING;
			
	// Pass along the return code and resume the parent task
	asm ("pushl %0\n"
		 "pushl %1\n"
		 "jmp return_from_user"
		 :
		 : "d"(next_pcb->saved_esp), "a"(ret));
	
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
	
	descriptors = pcb->descriptors;
}

// Switch from one task to another (automatically enables interrupts)
void context_switch(pcb_t* from, pcb_t* to) {
	// Don't bother switching if they are the same
	if (from == to && !signal_pending(from)) {
		sti();
		return;
	}
	
	// Crticial section
	cli();
	
	from->state = READY;
	to->state = RUNNING;
	
	// Map the "to" task back into memory and set its parameters
	set_current_task(to);
	
	// This part auto enables interrupts
	context_switch_asm(from, to);
}

// Run the scheduler (gets called up PIT interrupt)
void schedule() {
	// This whole function is a critical section
	cli();
	
	pcb_t* current = get_current_pcb();
	if (!current) {
		sti();
		return;
	}
	
	// If this task is supposed to terminate, terminate it
	if (current->should_terminate) {
		sti();
		current->should_terminate = false;
		terminate_task(1);
		return;
	}
	
	pcb_t* next_pcb = get_next_runnable_task(current->task->next);
	
	// If there are no runnable tasks, do nothing (unless this task has signals pending)
	if (!next_pcb) {
		if (signal_pending(current))
			next_pcb = current;
		else {
			sti();
			return;
		}
	}
	
	// Swtich to the new task (automatically reenables interrupts)
	if (next_pcb->state != UNLOADED)
		context_switch(current, next_pcb);
	else {
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

