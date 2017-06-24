#include "interrupt.h"
#include <boot/x86_desc.h>
#include <common/lib.h>
#include <drivers/i8259.h>
#include "syscall.h"

// External defintions for the assembly interrupt functions
extern int* intx80;
extern int* idt_vectors[NUMBER_OF_INTERRUPTS];
extern int* idt_vectors_user[NUMBER_OF_USER_INTERRUPTS];

// Array of functions for specific pic IRQ's
void (*pic_table[NUMBER_OF_USER_INTERRUPTS])() = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};



//Various interrupts
//Some are currently met with a blue screen exception
//If so, pass their error as a string for easier debugging.

// Interrupt 0
void divide_error(uint32_t code, uint32_t eip) {
	signal_send(get_current_pcb(), SIGFPE);
	
#if DEBUG
	blue_screen("Divide by 0 error");
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
	signal_send(get_current_pcb(), SIGTRAP);
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
	signal_send(get_current_pcb(), SIGILL);
	
#if DEBUG
	blue_screen("Invalid opcode");
#else
	printf("Invalid opcode error.\n");
	schedule();
#endif
}

// Interrupt 7
void device_not_available(uint32_t code, uint32_t eip) {
#if !DEBUG
	// Kill the current task
	terminate_task(256);
#endif
	
	blue_screen("Device not available");
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
	signal_send(get_current_pcb(), SIGSTKFLT);
	
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

// Interrupt 14
void page_fault(uint32_t code, uint32_t eip) {
	signal_send(get_current_pcb(), SIGSEGV);
	
#if DEBUG
	uint32_t address = 0;
	asm volatile ("movl %%cr2, %0"
				  : "=a"(address));
	blue_screen("Page fault - 0x%x at address 0x%x (eip: 0x%x)", code, address, eip);
#else
	printf("Segfault.\n");
	schedule();
#endif
}

// Interrupt 16
void floating_point_error(uint32_t code, uint32_t eip) {
	signal_send(get_current_pcb(), SIGFPE);
	
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
	signal_send(get_current_pcb(), SIGFPE);
	
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
	lidt((int)&idt_desc_ptr);
#endif
}
