# boot.S - start point for the kernel after GRUB gives us control
# vim:ts=4 noexpandtab

#define ASM     1

#include "multiboot.h"
#include "x86_desc.h"

.text

#ifdef __APPLE__
.globl entry
.globl _entry
entry:
.globl setup_pages, _setup_pages
setup_pages:
_kernel_end:
#endif

#define VM_KERNEL_ADDRESS			0xC0000000
#define INITIAL_KERNEL_STACK_SIZE   (1024 * 128)

# Multiboot header (required for GRUB to boot us)
.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long -(MULTIBOOT_HEADER_MAGIC+MULTIBOOT_HEADER_FLAGS)


# Entrypoint to the kernel
.globl  start, _start
.globl setup_pages, _setup_pages
.globl _kernel_end

.align 4
start:
_start:
# Make sure interrupts are off
cli
jmp     continue

continue:
# Load the GDT
lgdt (gdt_desc - VM_KERNEL_ADDRESS)

# Give 64KB for the initial kernel stack
movl    $_kernel_end, %esp
addl    $INITIAL_KERNEL_STACK_SIZE, %esp
subl    $VM_KERNEL_ADDRESS, %esp

# Setup paging
call setup_pages

# Load CS with the new descriptor value and jump to higher half
ljmp    $KERNEL_CS, $keep_going

keep_going:
# Set up the rest of the segment selector registers
movw    $KERNEL_DS, %cx
movw    %cx, %ss
movw    %cx, %ds
movw    %cx, %es
movw    %cx, %fs
movw    %cx, %gs
# Push the parameters that entry() expects (see kernel.c):
# eax = multiboot magic
# ebx = address of multiboot info struct
pushl   %ebx
pushl   %eax

# Jump to the C entrypoint to the kernel.
call    entry

# We'll never get back here, but we put in a hlt anyway.
halt:
hlt
jmp     halt
