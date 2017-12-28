.text
.globl load_cr3, load_cr0_and_cr4, invalidate_page_address

#ifdef __APPLE__
.globl _load_cr3
.globl _load_cr0_and_cr4
.globl _invalidate_page_address

_load_cr3:
_load_cr0_and_cr4:
_invalidate_page_address:
#endif


// Function: puts the pointer to the page directory into the upper 20 bits of control register 3.
// Inputs: Takes in a 32 bit pointer to the start of the page directory
// Outputs: None

load_cr3:
	pushl %ebp
	movl %esp, %ebp

	# Load the stack argument into CR3
	movl 8(%esp), %eax
	movl %eax, %cr3

	leave
	ret

// Function: enables multiple sized paging by setting the PSE (bit 4) in CR4 to 1.
// 		 : sets the bit 31 of cr0 to enable paging.
// Inputs: None.
// Outputs: None.
load_cr0_and_cr4:
	pushl %ebp
	movl %esp, %ebp

	// Set 4th bit of CR4
	movl %cr4, %eax
	orl $0x10, %eax
	movl %eax, %cr4

	// Clear 5th bit of CR4
	movl %cr4, %eax
	andl $(0x20^0xFFFFFFFF), %eax
	movl %eax, %cr4

	// Set 31st bit of CR0
	movl %cr0, %eax
	orl $0x80000000, %eax
	movl %eax, %cr0

	// Set 16th bit of CR0 (prevents supervisor from writing to read only pages)
	movl %cr0, %eax
	orl $(1 << 16), %eax
	movl %eax, %cr0

	leave
	ret

// Invalidate a certain page
invalidate_page_address:
	movl 4(%esp), %eax
	invlpg (%eax)
	ret
