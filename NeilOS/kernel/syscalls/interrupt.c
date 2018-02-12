#include "interrupt.h"
#include <boot/x86_desc.h>
#include <common/lib.h>
#include <drivers/pic/i8259.h>
#include <memory/mmap_list.h>
#include <drivers/graphics/graphics.h>
#include <drivers/graphics/svga/svga_3d.h>

// External defintions for the assembly interrupt functions
extern int* intx80;\
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
 * More 
	timer_create, timer_delete, timer_settime
	select (implemented for terminal, need others - files), poll
 	sockets stuff
 */

void* syscalls[] = { fork, execve, getpid, getppid, waitpid, exit,
	open, read, write, llseek, truncate, stat, close, isatty, pipe, fcntl, select,
	mkdir, link, unlink, readdir, utime,
	brk, sbrk, mmap, munmap, msync,
	dup, dup2,
	times, gettimeofday,
	kill, sigaction, sigsetmask, siggetmask, sigprocmask, sigsuspend, alarm,
	nanosleep, sched_yield,
	sysconf, fpathconf,
	getwd, chdir,
	ioctl,
	sys_thread_create, gettid, thread_wait, thread_exit,
	// 2D Graphics
	graphics_fb_map, graphics_fb_unmap, graphics_info_get, graphics_info_set, graphics_update_rect,
		graphics_cursor_image_set, graphics_fence_create, graphics_fence_sync, graphics_fence_passed,
	// 3D Graphics
	svga3d_present, svga3d_present_readback, svga3d_surface_create, graphics3d_surface_dma, svga3d_surface_copy,
		svga3d_surface_stretch_copy, svga3d_surface_destroy, svga3d_context_create, svga3d_context_destroy,
		svga3d_clear, svga3d_draw, svga3d_state_render_target, svga3d_state_z_range, svga3d_state_viewport,
		svga3d_state_scissor_rect, svga3d_state_clip_plane, svga3d_state_texture_state, svga3d_state_render_state,
		svga3d_transform_set, svga3d_light_material, svga3d_light_data, svga3d_light_enabled,
		svga3d_shader_create, svga3d_shader_const, svga3d_shader_set_active, svga3d_shader_destroy,
	
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
	// probably need to enable AVX??
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
	if ((code & PAGE_FAULT_PAGE_PRESENT) && (code & PAGE_FAULT_WRITE_VIOLATON)) {
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
	
	// Check if this is from a mmap region
	if (current_pcb && mmap_list_process(current_pcb->user_mappings, address, code, current_pcb))
		return;
	
	signal_send(current_pcb, SIGSEGV);
	
#if DEBUG
	blue_screen("Page fault - 0x%x at address 0x%x (eip: 0x%x)", code, address, eip);
#else
	schedule();
#endif
}

// Interrupt 16
void floating_point_error(uint32_t code, uint32_t eip) {
	signal_send(current_pcb, SIGFPE);
	
#if DEBUG
	blue_screen("Floating point error");
#else
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
