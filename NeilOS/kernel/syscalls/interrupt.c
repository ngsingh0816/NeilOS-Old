#include "interrupt.h"
#include <boot/x86_desc.h>
#include <common/lib.h>
#include <drivers/pic/i8259.h>

// External defintions for the assembly interrupt functions
extern int* intx80;
extern int* idt_vectors[NUMBER_OF_INTERRUPTS];
extern int* idt_vectors_user[NUMBER_OF_USER_INTERRUPTS];
extern void load_lidt();

// Array of functions for specific pic IRQ's
void (*pic_table[NUMBER_OF_USER_INTERRUPTS])() = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

/* More system calls
 * For vi:
	sigjump_buf??
 * For coreutils:
	 du - "cannot read directory ., not a directory"
	 timeout - hangs
 * For awk:
 	* usleep (implemented using sleep)
 	* popen / pclose
 * More
	sched_yield
 	mmap, munmap
	abort
	pread, pwrite
	syslog
	clock_gettime, clock_nanosleep, clock_settime
	nanosleep
	timer_create, timer_delete, timer_settime
 	ioctl
 	select (stubbed - test "tail -f"), pselect,
 	sockets stuff
 */

void* syscalls[] = { fork, execve, getpid, getppid, waitpid, exit,
	open, read, write, llseek, truncate, stat, close, isatty, pipe, fcntl,
	mkdir, link, unlink, readdir, utime,
	brk, sbrk,
	dup, dup2,
	times, gettimeofday,
	kill, sigaction, sigsetmask, siggetmask, sigprocmask, sigsuspend, alarm,
	sleep,
	sysconf, fpathconf,
	getwd, chdir,
	ioctl,
	sys_thread_create, gettid, thread_wait, thread_exit,
};


//Various interrupts
//Some are currently met with a blue screen exception
//If so, pass their error as a string for easier debugging.

// Interrupt 0
void divide_error(uint32_t code, uint32_t eip) {
	signal_send(current_pcb, SIGFPE);
	
#if DEBUG
	blue_screen("Divide by 0 error (eip - 0x%x)", eip);
#else
	printf("Divide by 0.\n");
	schedule();
#endif
}

// Interrupt 2
void NMI_interrupt(uint32_t code, uint32_t eip) {
}

// Interrupt 3
void breakpoint(uint32_t code, uint32_t eip) {
	signal_send(current_pcb, SIGTRAP);
	schedule();
}

// Interrupt 4
void overflow(uint32_t code, uint32_t eip) {
}

// Interrupt 5
void bound_range_exception(uint32_t code, uint32_t eip) {
#if !DEBUG
	// Kill the current task
	terminate_task(256);
#endif
	
	blue_screen("Bound range exception");
}

// Interrupt 6
void invalid_opcode(uint32_t code, uint32_t eip) {
	signal_send(current_pcb, SIGILL);
	
#if DEBUG
	blue_screen("Invalid opcode");
#else
	printf("Invalid opcode error.\n");
	schedule();
#endif
}

extern void enable_sse();

// Interrupt 7
// Most likely a SSE instruction error or FPU (they use the same registers)
void device_not_available(uint32_t code, uint32_t eip) {
	// Restore sse registers and enable sse
	down(&current_thread->lock);
	enable_sse();
	if (current_thread->sse_init)
		asm volatile(" fxrstor (%0); "::"r"(current_thread->sse_registers));
	current_thread->sse_used = true;
	current_thread->sse_init = true;
	up(&current_thread->lock);
}

// Interrupt 8
void double_fault(uint32_t code, uint32_t eip) {
#if !DEBUG
	// Kill the current task
	terminate_task(256);
#endif
	
	blue_screen("Double fault");
}

// Interrupt 9
void coprocessor_segment_overrun(uint32_t code, uint32_t eip) {
#if !DEBUG
	// Kill the current task
	terminate_task(256);
#endif
	
	blue_screen("Coprocessor segment overrun");
}

// Interrupt 10
void invalid_tss(uint32_t code, uint32_t eip) {
#if !DEBUG
	// Kill the current task
	terminate_task(256);
#endif
	
	blue_screen("Invalid TSS");
}

// Interrupt 11
void segment_not_present(uint32_t code, uint32_t eip) {
#if !DEBUG
	// Kill the current task
	terminate_task(256);
#endif
	
	blue_screen("Segment not present");
}

// Interrupt 12
void stack_segment_fault(uint32_t code, uint32_t eip) {
	
#if DEBUG
	blue_screen("Stack segment fault");
#else
	printf("Stack segfault.\n");
	schedule();
#endif
}

// Interrupt 13
void general_protection(uint32_t code, uint32_t eip) {
#if !DEBUG
	// Kill the current task
	terminate_task(256);
#endif
	
	blue_screen("General protection fault - 0x%x (eip: 0x%x)", code, eip);
}

#define PAGE_PRESENT		0x1
#define WRITE_VIOLATON		0x2

#ifdef DEBUG
context_state_t	page_fault_context;
void debug_save_context(context_state_t	state) {
	page_fault_context = state;
}
#endif

// Interrupt 14
void page_fault(uint32_t code, uint32_t eip) {
	// Get the address it happened at
	uint32_t address = 0;
#ifndef __APPLE__
	asm volatile ("movl %%cr2, %0"
				  : "=a"(address));
#endif
		
	// Check if we were copying on write
	if ((code & PAGE_PRESENT) && (code & WRITE_VIOLATON)) {
		pcb_t* pcb = current_pcb;
		uint32_t addr_aligned = address & ~(FOUR_MB_SIZE - 1);
		// Find the page that deals with this
		page_list_t* t = pcb->page_list;
		if (t)
			down(&t->lock);
		while (t) {
			if (t->vaddr == addr_aligned && t->copy_on_write) {
				up(&t->lock);
				if (!page_list_copy_on_write(t, address))
					break;
				return;
			}
			
			page_list_t* prev = t;
			t = t->next;
			if (t)
				down(&t->lock);
			up(&prev->lock);
		}
	}
	
	signal_send(current_pcb, SIGSEGV);
	
#if DEBUG
	blue_screen("Page fault - 0x%x at address 0x%x (eip: 0x%x)", code, address, eip);
#else
	printf("Segfault.\n");
	schedule();
#endif
}

// Interrupt 16
void floating_point_error(uint32_t code, uint32_t eip) {
	signal_send(current_pcb, SIGFPE);
	
#if DEBUG
	blue_screen("Floating point error");
#else
	printf("Floating point error.\n");
	schedule();
#endif
}

// Interrupt 17
void alignment_check(uint32_t code, uint32_t eip) {
#if !DEBUG
	// Kill the current task
	terminate_task(256);
#endif
	
	blue_screen("Alignment check");
}

// Interrupt 18
void machine_check(uint32_t code, uint32_t eip) {
#if !DEBUG
	// Kill the current task
	terminate_task(256);
#endif
	
	blue_screen("Machine check");
}

// Interrupt 19
void smid_floating_point_exception(uint32_t code, uint32_t eip) {
	signal_send(current_pcb, SIGFPE);
	
#if DEBUG
	blue_screen("SMID floating point exception");
#else
	printf("Floating point error.\n");
	schedule();
#endif
}

// PIC interrupt (0x20-0x2F)
// Inputs: irq number
void PIC_int(int irq) {
	// Call through to the assigned handler if it exists
	if (pic_table[irq - 0x20])
		pic_table[irq - 0x20](irq - 0x20);
}

/*
 * request_irq()
 * Inputs: irq - the specific PIC IRQ you want to handle (0 - 0xF)
 *			handler - the function pointer of the handler
 * Function: sets a custom interrupt handler for a PIC interrupt
 */
void request_irq(unsigned int irq, void (*handler)(int)) {
	pic_table[irq] = handler;
}

// Table of reserved interrupts, sets up interrupts to use functions we declared above
void (*interrupt_table[NUMBER_OF_INTERRUPTS])(uint32_t code, uint32_t eip) = {
	divide_error, NULL, NMI_interrupt, breakpoint, overflow, bound_range_exception,
	invalid_opcode, device_not_available, double_fault, coprocessor_segment_overrun,
	invalid_tss, segment_not_present, stack_segment_fault, general_protection, page_fault,
	NULL, floating_point_error, alignment_check, machine_check, smid_floating_point_exception,
};

/*
 * set_idt_entry()// Sets an entry in the idt correctly
 * Inputs: desc_t struct, function pointer, priviledge (for user/ kernel)
 */
void set_idt_entry(idt_desc_t* entry, int* function, uint8_t privilege) {
	entry->val[1] = (((uint32_t)function) & 0xFFFF0000) |
		(1 << 15) | ((privilege & 0x3) << 13) | (0xF << 8);
	entry->val[0] = (KERNEL_CS << 16) | (((uint32_t)function) & 0xFFFF);
}

/* void load_idt(void)
 * Inputs: none
 * Return Value: none
 * Function: loads the IDT table with the correct pointers and then sets the
 *	processor to use that IDT.
 */
void load_idt() {
	int z;					// Loop over interrupt table
	
	// Fill out the interrupts
	for (z = 0; z < NUMBER_OF_INTERRUPTS; z++)
		set_idt_entry(&idt[z], (int*)idt_vectors[z], KERNEL_PRIVILEGE);
	
	// Devices
	for (z = 0; z < NUMBER_OF_USER_INTERRUPTS; z++) {
		set_idt_entry(&idt[PIC_USER_INTERRUPT_VECTOR + z],
					  (int*)idt_vectors_user[z], KERNEL_PRIVILEGE);
	}
	
	// Don't forget about the syscall
	set_idt_entry(&idt[0x80], (int*)&intx80, USER_PRIVILEGE);
	
	// Avoids error in xcode
#ifndef __APPLE__
	// Load the IDT
	load_lidt();
	//lidt((int)&idt_desc_ptr);
#endif
}
