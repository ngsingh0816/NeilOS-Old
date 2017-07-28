# interrupt.S - Set up interrupt function pointers
# vim:ts=4 noexpandtab

#define ASM     1

#define NUM_SYSCALLS			36

#include <boot/x86_desc.h>

.text

.globl intx80
.globl interrupt_table
.globl idt_vectors
.globl idt_vectors_user
.globl syscall
.globl PIC_int

.globl return_to_user
.globl return_from_user
.globl context_switch_asm
.globl get_context

.globl fork_return

.globl signal_handler
.globl sigreturn
.globl sigreturn_impl
.globl sigjmp

.globl syscalls

#ifdef __APPLE__

.globl _sigreturn
.globl _sigjmp
.globl _context_switch_asm
.globl _return_from_user
.globl _idt_vectors
.globl _idt_vectors_user
.globl _intx80
.globl _get_context
.globl _return_to_user
.globl _fork_return

sigreturn:
_sigreturn:
sigjmp:
_sigjmp:
context_switch_asm:
_context_switch_asm:
return_from_user:
_return_from_user:
idt_vectors:
_idt_vectors:
idt_vectors_user:
_idt_vectors_user:
intx80:
_intx80:
get_context:
_get_context:
return_to_user:
_return_to_user:
fork_return:
_fork_return:

#else

.align 4

// Jump table for interrupts
idt_vectors:
.long int0, 0, int2, int3, int4, int5, int7, int7, int8, int9, int10
.long int11, int12, int13, int14, 0, int16, int17, int18, int19

idt_vectors_user:
.long intx20, intx21, intx22, intx23, intx24, intx25, intx26, intx27, intx28, intx29, intx2A, intx2B, intx2C, intx2D, intx2E, intx2F

// get_context(context_state_t* context)
get_context:
	// Save eax and load its value with the argument
	pushl %eax
	movl 8(%esp), %eax

	// Save the original esp (ebp + 4 points to the return address)
	addl $4, %ebp
	movl %ebp, 12(%eax)
	addl $-4, %ebp

	// Save everything else
	movl %edi, 0(%eax)
	movl %esi, 4(%eax)

	// Save old ebp (located at ebp)
	pushl %ebx
	movl (%ebp), %ebx
	movl %ebx, 8(%eax)
	popl %ebx

	// Continue saving things
	movl %ebx, 16(%eax)
	movl %edx, 20(%eax)
	movl %ecx, 24(%eax)

	// Save eax
	popl %eax
	pushl %ebx
	movl 8(%esp), %ebx
	movl %eax, 28(%ebx)
	popl %ebx

	ret

// ContextSwitch(pcb_t* from, pcb_t* to)
// Switches contexes from the first argument to the second
context_switch_asm:
	cli

	// eax = from, ebx = to
	movl 4(%esp), %eax
	movl 8(%esp), %ebx

	cmp $0, %eax
	je done_parent_cs

	// Prepare the first argument for a context switch by saving the stack
	pusha
	movl %esp, (%eax)

done_parent_cs:

	// Allow for signal handling
	pushl %ebx
	call signal_handle
	addl $4, %esp

	// Assume that we must have come from context switch or return_to_user
	// so just restore the saved esp and return
	movl (%ebx), %esp

	popa
	sti

	ret

// Return into a signal handler (exec, ret, signum)
sigjmp:
	movl %esp, %eax

	pushl 12(%eax)
	pushl 8(%eax)
	pushl 4(%eax)

	ret

// Restore the context
sigreturn:
	call sigreturn_impl
	addl $4, %esp

	ret

// Passed an address to return to
// Jumps to the specified address in user mode using the iret instruction
return_to_user:
	// Order goes SS, ESP, EFLAGS, CS, EIP
	movl $USER_DS, %ecx
	movw    %cx, %ds
	movw    %cx, %es
	movw    %cx, %fs
	movw    %cx, %gs

	// Save our stack and get the entry point
	// set parent->saved_esp = esp
	movl 8(%esp), %edx
	movl 4(%esp), %eax
	cmp $0, %edx
	je done_parent
	pusha
	movl %esp, (%edx)

	done_parent:
	// set ebx = pcb->entry
	movl 4(%eax), %ebx

	pushl %ecx
	// - 12 because the argc, argv, and evp have been pushed to the stack
	// set ecx = pcb->stack_address
	movl 8(%eax), %ecx
	addl $-12, %ecx
	pushl %ecx

	// Enable interrupts upon returning
	pushfl
	popl %ecx
	orl $(1 << 9), %ecx
	pushl %ecx

	movl $USER_CS, %eax
	pushl %eax
	pushl %ebx
	iret

save_return: .long 0
// Will return here after the process ends, so continue the kernel
return_from_user:
	// Critical section
	cli

	popl %eax
	movl %eax, save_return
	popl %esp
	popa
	movl save_return, %eax
	sti

	ret

// Helper to return a child from fork (immediately exits)
// fork_return()
fork_return:
	iret

// The assembly linkage function for calling an interrupt
interrupt:
	// Check if its a syscall
	cmp $0x80, %eax
	je is_syscall

	// Save all the registers
	pusha

	// Save segment registers
	push %ds
	push %es
	push %fs
	push %gs

	// Set kernel data segment
	mov $KERNEL_DS, %cx
	mov %cx, %ds
	mov %cx, %es
	mov %cx, %fs
	mov %cx, %gs

	// Check if it's a PIC interrupt
	cmp $0x20, %eax
	jge is_pic

	// Call correct entry in the the jump table
	movl $interrupt_table, %ecx
	// Push error code onto the stack
	pushl %edx
	pushl %ebx
	call *(%ecx, %eax, 4)
	popl %ebx
	popl %edx
	jmp done_interrupt

	// Call the pic interrupt
	is_pic:
		pushl %eax
		call PIC_int
		popl %eax
		jmp done_interrupt

	// Call the syscall
	is_syscall:
		// Get the value from eax back
		movl 4(%esp), %eax

		// Check its a valid syscall
		cmp $0, %eax
		jl invalid_syscall
		cmp $NUM_SYSCALLS, %eax
		jge invalid_syscall

		pushl %ebp
		movl %esp, %ebp
		addl $-4, %esp

		// Save the registers
		pusha

		// Save segment registers
		push %ds
		push %es
		push %fs
		push %gs

		pushl %ecx
		// Set kernel data segment
		mov $KERNEL_DS, %cx
		mov %cx, %ds
		mov %cx, %es
		mov %cx, %fs
		mov %cx, %gs
		popl %ecx

		// Say that the current task is in a syscall
		pushl %eax
		movl current_pcb, %eax
		movb $1, 12(%eax)
		popl %eax

		// Push the arguments
		pushl %esi
		pushl %edx
		pushl %ecx
		pushl %ebx

		// Call the syscall
		movl $syscalls, %ebx
		call *(%ebx, %eax, 4)

		// Save the return value
		movl %eax, -4(%ebp)
		// Pop the arguments
		popl %ebx
		popl %ecx
		popl %edx
		popl %esi

		// Say that the current task is out of the syscall
		movl current_pcb, %eax
		movb $0, 12(%eax)

		// If we are supposed to terminate the task, do it
		cmpb $1, 13(%eax)
		je terminate_syscall_task

		// Segment registers
		pop %gs
		pop %fs
		pop %es
		pop %ds

		// Restore the registers
		popa

		// Get the return value
		movl -4(%ebp), %eax

		// Restore frame
		addl $4, %esp
		popl %ebp

	done_syscall:
		ret
	invalid_syscall:
		movl $-1, %eax
		ret
	terminate_syscall_task:
		movl $-1, %eax
		pushl %eax
		call terminate_task
		popl %eax
		// If we get here the OS is probably broken, so just blue screen
		pushl broken_string
		call blue_screen
		hlt
	finally_done: jmp finally_done


broken_string: .string "OS Integrity Error"

done_interrupt:
	// Segment registers
	pop %gs
	pop %fs
	pop %es
	pop %ds

	// Restore registers
	popa

	ret

// Interrupt handlers (call through to a common function)
int0:
	pushl %edx

	// Get eip
	movl 4(%esp), %edx

	pushl %eax
	movl $0, %eax
	call interrupt

	popl %eax
	popl %edx
	iret
int2:
	pushl %eax
	movl $2, %eax
	call interrupt
	popl %eax
	iret
int3:
	pushl %eax
	movl $3, %eax
	call interrupt
	popl %eax
	iret
int4:
	pushl %eax
	movl $4, %eax
	call interrupt
	popl %eax
	iret
int5:
	pushl %eax
	movl $5, %eax
	call interrupt
	popl %eax
	iret
int6:
	pushl %eax
	movl $6, %eax
	call interrupt
	popl %eax
	iret
int7:
	pushl %eax
	movl $7, %eax
	call interrupt
	popl %eax
	iret
int8:
	// Double fault pushes a 0 onto the stack
	addl $4, %esp
	pushl %eax
	movl $8, %eax
	call interrupt
	popl %eax
	iret
int9:
	pushl %eax
	movl $9, %eax
	call interrupt
	popl %eax
	iret
int10:
	// invalid tss
	pushl %ebx
	movl 4(%esp), %ebx

	pushl %eax
	movl $10, %eax
	call interrupt

	popl %eax
	popl %ebx
	addl $4, %esp
	iret
int11:
	// segment not present
	pushl %ebx
	movl 4(%esp), %ebx

	pushl %eax
	movl $11, %eax
	call interrupt

	popl %eax
	popl %ebx
	addl $4, %esp
	iret
int12:
	// stack segment fault
	pushl %ebx
	movl 4(%esp), %ebx

	pushl %eax
	movl $12, %eax
	call interrupt

	popl %eax
	popl %ebx
	addl $4, %esp
	iret
int13: // gpf
	pushl %ebx
	pushl %edx

	movl 12(%esp), %edx
	movl 8(%esp), %ebx

	pushl %eax
	movl $13, %eax
	call interrupt

	popl %eax
	popl %edx
	popl %ebx
	addl $4, %esp
	iret
int14:	// page fault
	pushl %ebx
	pushl %edx

	// Get eip
	movl 12(%esp), %edx
	// Get code
	movl 8(%esp), %ebx

	pushl %eax
	movl $14, %eax
	call interrupt

	popl %eax
	popl %edx
	popl %ebx
	addl $4, %esp
	iret
int16:
	pushl %eax
	movl $16, %eax
	call interrupt
	popl %eax
	iret
int17: // alignment check
	pushl %ebx
	movl 4(%esp), %ebx

	pushl %eax
	movl $17, %eax
	call interrupt

	popl %eax
	popl %ebx
	addl $4, %esp
	iret
int18:
	pushl %eax
	movl $18, %eax
	call interrupt
	popl %eax
	iret
int19:
	pushl %eax
	movl $19, %eax
	call interrupt
	popl %eax
	iret
intx20:
	pushl %eax
	movl $0x20, %eax
	call interrupt
	popl %eax
	iret
intx21:
	pushl %eax
	movl $0x21, %eax
	call interrupt
	popl %eax
	iret
intx22:
	pushl %eax
	movl $0x22, %eax
	call interrupt
	popl %eax
	iret
intx23:
	pushl %eax
	movl $0x23, %eax
	call interrupt
	popl %eax
	iret
intx24:
	pushl %eax
	movl $0x24, %eax
	call interrupt
	popl %eax
	iret
intx25:
	pushl %eax
	movl $0x25, %eax
	call interrupt
	popl %eax
	iret
intx26:
	pushl %eax
	movl $0x26, %eax
	call interrupt
	popl %eax
	iret
intx27:
	pushl %eax
	movl $0x27, %eax
	call interrupt
	popl %eax
	iret
intx28:
	pushl %eax
	movl $0x28, %eax
	call interrupt
	popl %eax
	iret
intx29:
	pushl %eax
	movl $0x29, %eax
	call interrupt
	popl %eax
	iret
intx2A:
	pushl %eax
	movl $0x2A, %eax
	call interrupt
	popl %eax
	iret
intx2B:
	pushl %eax
	movl $0x2B, %eax
	call interrupt
	popl %eax
	iret
intx2C:
	pushl %eax
	movl $0x2C, %eax
	call interrupt
	popl %eax
	iret
intx2D:
	pushl %eax
	movl $0x2D, %eax
	call interrupt
	popl %eax
	iret
intx2E:
	pushl %eax
	movl $0x2E, %eax
	call interrupt
	popl %eax
	iret
intx2F:
	pushl %eax
	movl $0x2F, %eax
	call interrupt
	popl %eax
	iret
intx80:
	pusha
	call pcb_set_context
	popa

	pushl %eax
	movl $0x80, %eax
	call interrupt
	addl $4, %esp
	iret

#endif
