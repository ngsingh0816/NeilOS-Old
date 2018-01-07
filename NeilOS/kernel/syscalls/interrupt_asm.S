# interrupt.S - Set up interrupt function pointers
# vim:ts=4 noexpandtab

#define ASM     1

#define NUM_SYSCALLS			49
#define THREAD_EXIT_SYSCALL		48

#include <boot/x86_desc.h>

.text

.globl intx80
.globl interrupt_table
.globl idt_vectors
.globl idt_vectors_user
.globl syscall
.globl PIC_int

.globl return_to_user
.globl context_switch_asm
.globl get_context

.globl fork_return

.globl signal_handler
.globl sigreturn
.globl sigreturn_impl
.globl sigjmp

.globl enable_sse
.globl disable_sse

.globl syscalls

.globl schedule

.globl implicit_thread_exit_start
.globl implicit_thread_exit_end

#if DEBUG
.globl debug_save_context
#endif

#ifdef __APPLE__

.globl _sigreturn
.globl _sigjmp
.globl _context_switch_asm
.globl _idt_vectors
.globl _idt_vectors_user
.globl _intx80
.globl _get_context
.globl _return_to_user
.globl _fork_return
.globl _enable_sse
.globl _disable_sse
.globl _implicit_thread_exit_start
.globl _implicit_thread_exit_end

sigreturn:
_sigreturn:
sigjmp:
_sigjmp:
context_switch_asm:
_context_switch_asm:
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
enable_sse:
_enable_sse:
disable_sse:
_disable_sse:
implicit_thread_exit_start:
_implicit_thread_exit_start:
implicit_thread_exit_end:
_implicit_thread_exit_end:

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

// ContextSwitch(thread_t* from, thread_t* to)
// Switches contexes from the first argument to the second
context_switch_asm:
	cli

	// 4(%esp) = from
	// 8(%esp) = to

	movl 8(%esp), %eax
	cmp $0, 4(%esp)
	je done_parent_cs

	// Prepare the "from" thread for a context switch by saving the stack
	movl 4(%esp), %eax
	pusha
	movl %esp, (%eax)
	movl 40(%esp), %eax

done_parent_cs:

	// Assume that we must have come from context switch or return_to_user
	// so just restore the saved esp and return
	movl (%eax), %esp

	popa
	sti

	ret

// Return into a signal handler (exec, ret, signum)
sigjmp:
	movl %esp, %eax

	pushfl
	pushl 12(%eax)
	pushl 8(%eax)
	pushl 4(%eax)

	sti
	ret

// Restore the context
sigreturn:
	call sigreturn_impl
	addl $4, %esp
	popfl

	ret

// Passed an address to return to
// Jumps to the specified address in user mode using the iret instruction
return_to_user:
	// 12(%esp) = parent thread
	// 8(%esp) = pcb
	// 4(%esp) = thread
	pushl %edx
	pushl %ebx
	// 20(%esp) = parent thread
	// 16(%esp) = pcb
	// 12(%esp) = thread

	movl 20(%esp), %edx		// parent thread
	movl 16(%esp), %eax		// pcb
	movl 12(%esp), %ebx		// thread

	cmp $0, %edx
	je done_parent
	popl %ebx
	popl %edx
	pusha
	movl 44(%esp), %edx		// parent thread
	movl 36(%esp), %ebx		// thread
	// set parent_thread->saved_esp = esp
	movl %esp, (%edx)
	addl $-8, %esp

	done_parent:
	addl $8, %esp
	// Order goes SS, ESP, EFLAGS, CS, EIP
	movl $USER_DS, %ecx
	movw    %cx, %ds
	movw    %cx, %es
	movw    %cx, %fs
	movw    %cx, %gs

	pushl %ecx
	// set ecx = thread->stack_address - 12
	// - 12 because the argc, argv, and evp have been pushed to the stack
	movl 4(%ebx), %ecx	// thread->stack_address
	addl $-12, %ecx
	pushl %ecx

	// Enable interrupts upon returning
	pushfl
	popl %ecx
	orl $(1 << 9), %ecx
	pushl %ecx

	movl $USER_CS, %ecx
	pushl %ecx
	pushl (%eax)		// pcb->entry
	iret

// Helper to return a child from fork (immediately exits)
// fork_return()
fork_return:
	pushl %ecx
	movl $USER_DS, %ecx
	movw    %cx, %ds
	movw    %cx, %es
	movw    %cx, %fs
	movw    %cx, %gs
	popl %ecx

	iret

// Helper code to perform an implicit call to thread_exit()
implicit_thread_exit_start:
	movl $THREAD_EXIT_SYSCALL, %eax
	int $0x80
	// Shouldn't return
implicit_thread_exit_end:

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
		addl $-8, %esp
		// Save the ebp
		pushl %eax
		movl 12(%esp), %eax
		movl %eax, -8(%ebp)
		popl %eax

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

		// If we are supposed to terminate the task, do it
		pushl %eax
		movl current_pcb, %eax
		cmpb $1, 4(%eax)
		je terminate_syscall_task
		popl %eax

		// Check for signals
		pusha
		movl current_pcb, %eax
		pushl %eax
		call signal_handle
		addl $4, %esp
		popa

		// Say that the current thread is in a syscall and clear the signal occurred flag for the task
		pushl %eax
		movl current_thread, %eax
		movb $1, 8(%eax)
		movl current_pcb, %eax
		movb $0, 5(%eax)
		popl %eax

		// Push the arguments
		pushl -8(%ebp)			// ebp
		pushl %edi
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
		addl $4, %esp			// ebp
		popl %ebx
		popl %ecx
		popl %edx
		popl %esi
		popl %edi

		// Say that the current thread is out of the syscall
		movl current_thread, %eax
		movb $0, 8(%eax)

		// If we are supposed to terminate the task, do it
		movl current_pcb, %eax
		cmpb $1, 4(%eax)
		je terminate_syscall_task2

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
		addl $8, %esp
		popl %ebp

	done_syscall:
		ret
	invalid_syscall:
		movl $-1, %eax
		ret
	terminate_syscall_task:
		popl %eax
	terminate_syscall_task2:
		movl $0, %eax
		pushl %eax
		call terminate_task
		popl %eax
		// If we get here, there are still some other threads running, so just schedule then loop around
		call schedule
		jmp terminate_syscall_task2


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

// Enable / Disable SSE
enable_sse:
	mov %cr0, %eax
	and $0xFFFFFFFB, %eax	// clear coprocessor emulation CR0.EM
	or $0x2, %eax			// set coprocessor monitoring  CR0.MP
	mov %eax, %cr0
	mov %cr4, %eax
	or $(3 << 9), %eax		// set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
	mov %eax, %cr4
	ret

disable_sse:
	mov %cr0, %eax
	or $0x4, %eax		    // set coprocessor emulation CR0.EM
	and  $0xFFFFFFFD, %eax	// clear coprocessor monitoring  CR0.MP
	mov %eax, %cr0
	mov %cr4, %eax
	and $(~(3 << 9)), %eax	// clear CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
	mov %eax, %cr4
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
	pushl %ebx
	pushl %edx

	// Get eip
	movl 8(%esp), %edx
	// Get code
	movl $0, %ebx

	pushl %eax
	movl $7, %eax
	call interrupt
	popl %eax
	popl %edx
	popl %ebx
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
#if DEBUG
	pusha
	call debug_save_context
	popa
#endif

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
	call thread_set_context
	popa

	pushl %eax
	movl $0x80, %eax
	call interrupt
	addl $4, %esp
	iret

#endif
