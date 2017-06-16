#include "syscall.h"
#include <common/types.h>
#include <common/lib.h>
#include <drivers/terminal/terminal.h>
#include <drivers/rtc/rtc.h>
#include <drivers/ATA/ata.h>

#define EXECUTABLE_MAGIC_NUMBER		0x464c457f

// Device for syscall's purposes
typedef struct syscall_device {
	char* name;
	file_descriptor_t* (*open)(const char*, uint32_t mode);
} syscall_device_t;

file_descriptor_t** descriptors = NULL;
syscall_device_t syscall_devices[] = {
	{ "stdin", terminal_open },
	{ "stdout", terminal_open },
	{ "rtc", rtc_open },
	{ "disk0", ata_open },
	{ "disk0s1", ata_open },
};

extern void return_to_user(pcb_t* pcb, pcb_t* parent);

void* syscalls[] = { fork, execve, getpid, waitpid, exit,
					open, read, write, llseek, truncate, stat, close,
					mkdir, link, unlink,
					brk, sbrk,
					dup, dup2,
					times,
					kill, signal, sigsetmask, siggetmask,
};

// Helpers
int32_t queue_task(const char* cmd, const char** argv, const char** envp, pcb_t** ret);

// Program level syscalls

// Gets the context of the machine right before this function call is executed
extern void get_context(context_state_t* context);

// Duplicate a program and memory space
uint32_t fork() {
	context_state_t context;
	get_context(&context);
	
	pcb_t* pcb = NULL;
	if ((pcb = duplicate_current_task()) == NULL)
		return -1;
		
	// Pusha (with eax set to 0 so the child returns 0)
	context.esp = context.esp - (uint32_t)get_current_pcb() + (uint32_t)pcb;
	pcb->saved_esp = context.esp - sizeof(context_state_t);
	context.eax = 0;
	memcpy((void*)pcb->saved_esp, &context, sizeof(context_state_t));
	
	// Enable the child to be scheduled
	pcb->state = READY;
	
	return pcb->task->pid;
}

// Load a new program into the current memory space
uint32_t execve(const char* filename, const char* argv[], const char* envp[]) {
	// Load the task
	pcb_t* pcb = NULL;
	if (queue_task(filename, argv, envp, &pcb) != 0)
		return -1;
	
	return run(pcb);
}

// Get the pid of the current process
uint32_t getpid() {
	return get_current_pcb()->task->pid;
}

// Wait for a child to change state (returns the status that the child returned with)
uint32_t waitpid(uint32_t pid) {
	pcb_t* pcb = get_current_pcb();
	
	// Wait until the child has finished
	while (!signal_pending(pcb)) {
		bool found = false;
		task_list_t* child = pcb->children;
		while (child) {
			if (child->pid == pid) {
				found = true;
				// Child is a zombie if child->pcb == NULL
				if (!child->pcb)
					return child->return_value;
			}
		}
		// If the pid does not exist, return error
		if (!found)
			return -1;
		
		schedule();
	}
	
	// Signal was caught
	return -1;
}

// Exit a program with a specific status
uint32_t exit(int status) {
	pcb_t* pcb = get_current_pcb();
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

// File syscalls

file_descriptor_t* open_handle(const char* filename, uint32_t mode) {
	file_descriptor_t* f = NULL;
	// Loop through the devices
	int z;
	for (z = 0; z < sizeof(syscall_devices) / sizeof(syscall_device_t); z++) {
		if (strncmp(filename, syscall_devices[z].name, strlen(syscall_devices[z].name) + 1) == 0) {
			f = syscall_devices[z].open(filename, mode);
			if (f) {
				f->ref_count = 1;
				return f;
			}
			return NULL;
		}
	}
	
	// Default to the filesystem if no device was found
	f = filesystem_open(filename, mode);
	if (f) {
		f->ref_count = 1;
		return f;
	}
	
	return NULL;
}

// Open a file descriptor
uint32_t open(const char* filename, uint32_t mode) {
	// Find and open descriptor
	uint32_t current_fd = -1;
	int z;								// Looping over descriptors
	for (z = 0; z < NUMBER_OF_DESCRIPTORS; z++) {
		if (!descriptors[z]) {
			current_fd = z;
			break;
		}
	}
	
	// We have no more descriptors available so we can't open anything
	if (current_fd == -1)
		return -1;

	pcb_t* pcb = get_current_pcb();
	pcb->descriptors[current_fd] = open_handle(filename, mode);
	if (!pcb->descriptors[current_fd])
		return -1;
	
	descriptors = pcb->descriptors;
	return current_fd;
}

// Read from a file descriptor
uint32_t read(int32_t fd, void* buf, uint32_t nbytes) {
	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS || fd < 0 || nbytes == 0 || !buf)
		return -1;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->read)
		return -1;
	
	// Call to the driver specific call
	return descriptors[fd]->read(fd, buf, nbytes);
}

// Write to a file descriptor
uint32_t write(int32_t fd, const void* buf, uint32_t nbytes) {
	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS || fd < 0 || nbytes == 0 || !buf)
		return -1;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->write)
		return -1;
	
	// Call to the driver specific call
	return descriptors[fd]->write(fd, buf, nbytes);
}

// Seek to an offset
uint32_t llseek(int32_t fd, uint64_t offset, int whence) {
	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS || fd < 0 || !(whence >= SEEK_SET && whence <= SEEK_END))
		return -1;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->llseek)
		return -1;
	
	// Call to the driver specific call
	// TODO: for now only return the lower 32 bits
	return descriptors[fd]->llseek(fd, offset, whence).low;
}

// Change the size of a file
uint32_t truncate(int32_t fd, uint64_t length) {
	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS || fd < 0)
		return -1;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !descriptors[fd]->truncate)
		return -1;
	
	// Call to the driver specific call
	// TODO: for now just return the lower 32 bits
	return descriptors[fd]->truncate(fd, length).low;
}

// Get information about a file descriptor
uint32_t stat(int32_t fd, sys_stat_type* data) {
	// Check if the arguments are in range
	if (fd >= NUMBER_OF_DESCRIPTORS || fd < 0 || !data)
		return -1;
	
	// If we are trying to use an invalid descriptor, return failure
	if (!descriptors[fd] || !(descriptors[fd]->type == FILE_FILE_TYPE ||
							  descriptors[fd]->type == DIRECTORY_FILE_TYPE))
		return -1;
	
	*data = fstat(descriptors[fd]);
	return 0;
}

// Close a file descriptor
uint32_t close(int32_t fd) {
	// Check if this descriptor is within the bounds
	if (fd >= NUMBER_OF_DESCRIPTORS || fd < 0)
		return -1;
	
	// If we are trying to close an invalid descriptor, return failure
	if (!descriptors[fd])
		return -1;
	
	uint32_t ret = 0;
	if ((--descriptors[fd]->ref_count) == 0) {
		ret = descriptors[fd]->close((void*)descriptors[fd]);
		kfree(descriptors[fd]);
	}
	pcb_t* pcb = get_current_pcb();
	pcb->descriptors[fd] = NULL;
	
	descriptors = pcb->descriptors;
	
	return ret;
}

// Filesystem syscalls

// Make a directory
uint32_t mkdir(const char* name) {
	return (fmkdir(name) ? 0 : -1);
}

// Hard link a file
uint32_t link(const char* filename, const char* new_name) {
	// Open the file and assert valid
	file_descriptor_t f;
	if (!fopen(filename, FILE_MODE_READ, &f))
		return -1;
	
	// Link and close
	bool ret = flink(&f, new_name);
	ret &= fclose(&f);
	
	return (ret ? 0 : -1);
}

// Unlink a file (note: deletes the file if it is the last remaining link)
uint32_t unlink(const char* filename) {
	// Open the file and assert valid
	file_descriptor_t f;
	if (!fopen(filename, FILE_MODE_READ | FILE_MODE_DELETE_ON_CLOSE, &f))
		return -1;
	
	// Close (and therefore delete) it
	return (fclose(&f) ? 0 : -1);
}

// Memory operations

// Set the program break to a specific address
uint32_t brk(uint32_t addr) {
	if (addr < USER_ADDRESS + FOUR_MB_SIZE)
		return -1;
	
	addr -= USER_ADDRESS;
	pcb_t* pcb = get_current_pcb();
	uint32_t needed_pages = addr / FOUR_MB_SIZE + 1;
	
	// Get number of current pages
	uint32_t num_pages = 0;
	memory_list_t* t = pcb->memory_map;
	memory_list_t* prev = NULL;
	while (t) {
		num_pages++;
		prev = t;
		t = t->next;
	}
	
	if (needed_pages > num_pages) {
		bool flags = set_multitasking_enabled(false);
		// Allocate new pages
		t = prev;
		while (needed_pages > num_pages) {
			t->next = kmalloc(sizeof(memory_list_t));
			if (!t->next) {
				brk(pcb->brk);
				return -1;
			}
			memset(t->next, 0, sizeof(memory_list_t));
			t->next->prev = t;
			
			void* page = page_get_four_mb(1, VIRTUAL_MEMORY_USER);
			if (!page) {
				kfree(t->next);
				t->next = NULL;
				brk(pcb->brk);
				return -1;
			}
			
			t->paddr = (uint32_t)vm_virtual_to_physical((uint32_t)page);
			vm_unmap_page((uint32_t)page);
			
			t = t->next;
			num_pages++;
		}
		set_multitasking_enabled(flags);
	} else if (needed_pages < num_pages) {
		bool flags = set_multitasking_enabled(false);
		// Dealloc uneccessary pages
		t = prev;
		while (needed_pages < num_pages) {
			prev = t->prev;
			prev->next = NULL;
			page_free((void*)(USER_ADDRESS + FOUR_MB_SIZE * (num_pages - 1)));
			kfree(t);
			
			t = prev;
			num_pages--;
		}
		
		set_multitasking_enabled(flags);
	}
	
	// Return the new break
	pcb->brk = addr + USER_ADDRESS;
	return pcb->brk;
}

// Offset the current program break by a specific ammount
uint32_t sbrk(int32_t offset) {
	return brk(get_current_pcb()->brk + offset);
}

// File descriptors

// Duplicate a file descriptor
uint32_t dup(uint32_t fd) {
	// Find and open descriptor
	uint32_t current_fd = -1;
	int z;								// Looping over descriptors
	for (z = 0; z < NUMBER_OF_DESCRIPTORS; z++) {
		if (!descriptors[z]) {
			current_fd = z;
			break;
		}
	}
	
	// We have no more descriptors available so we can't open anything
	if (current_fd == -1)
		return -1;
	
	return dup2(fd, current_fd);
}

// Duplicate a file descriptor into the specific file descriptor
uint32_t dup2(uint32_t fd, uint32_t new_fd) {
	// Check if this descriptor is within the bounds
	if (fd >= NUMBER_OF_DESCRIPTORS || new_fd >= NUMBER_OF_DESCRIPTORS)
		return -1;
	
	if (!descriptors[fd])
		return -1;
	
	bool flags = set_multitasking_enabled(false);
	pcb_t* pcb = get_current_pcb();
	// Close the file handle if opened
	if (pcb->descriptors[new_fd]) {
		pcb->descriptors[new_fd]->close(pcb->descriptors[new_fd]);
		if ((--pcb->descriptors[new_fd]->ref_count) == 0)
			kfree(pcb->descriptors[new_fd]);
	}
	// Point the new file handle to the old one
	pcb->descriptors[new_fd] = pcb->descriptors[fd];
	pcb->descriptors[new_fd]->ref_count++;
	
	descriptors = pcb->descriptors;
	
	set_multitasking_enabled(flags);
	
	return new_fd;
}

// Time

// Get the timing information for a process
uint32_t times(sys_time_type* data) {
	if (!data)
		return -1;
	
	// TODO: fill these out
	data->child_user_time.val = 0;
	data->child_process_time.val = 0;
	data->system_time.val = 0;
	data->user_time.val = 0;
	
	return 0;
}

// Signals

// Send a signal to a process
uint32_t kill(uint32_t pid, uint32_t signum) {
	if (signum > NUMBER_OF_SIGNALS || signum == 0)
		return -1;
	
	pcb_t* pcb = pcb_from_pid(pid);
	if (!pcb)
		return -1;
	
	signal_send(pcb, signum);
	return 0;
}

// Set the signal handler for a certain signal
uint32_t signal(uint32_t signum, sighandler_t handler) {
	if (signum > NUMBER_OF_SIGNALS || signum == 0)
		return -1;
	
	signal_set_handler(get_current_pcb(), signum, handler);
	return 0;
}

// Mask signals
uint32_t sigsetmask(uint32_t signum, bool masked) {
	signal_set_masked(get_current_pcb(), signum, masked);
	return 0;
}

// Unmask signals
uint32_t siggetmask(uint32_t signum) {
	return signal_is_masked(get_current_pcb(), signum);
}

// Pipes

// Sockets

// Misc

// Initialize the descriptors for a pcb
void init_descriptors(pcb_t* pcb) {
	// Open default descriptors
	descriptors = pcb->descriptors;
	pcb->descriptors[STDIN] = terminal_open("stdin", FILE_MODE_READ);
	pcb->descriptors[STDIN]->ref_count = 1;
	pcb->descriptors[STDOUT] = terminal_open("stdout", FILE_MODE_WRITE);
	pcb->descriptors[STDOUT]->ref_count = 1;
	
	descriptors = pcb->descriptors;
}

// Queue a task in the currently loaded task's spot
int32_t queue_task(const char* cmd, const char** argv, const char** envp, pcb_t** ret) {
	pcb_t* pcb = load_task_replace((char*)cmd, argv, envp);
	
	// If we couldn't load the task, let the caller know
	if (!pcb)
		return -1;
	
	init_descriptors(pcb);
	
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
	
	set_current_task(pcb);

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
