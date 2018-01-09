#ifndef TERMINAL_H
#define TERMINAL_H

#include "common/types.h"
#include "syscalls/descriptor.h"

// File descriptors
#define STDIN		0
#define STDOUT		1
#define STDERR		2

// Maximum read size
#define MAX_TERMINAL_BUFFER_LENGTH	256

// Various IOCTL
#define TCFLUSH			0
#define TCFLOW			1
#define TCGETATTR		2
#define TCSETATTR		3
#define TTYNAME			4

// Termios defines
#define BRKINT	0x00000100
#define ICRNL	0x00000200
#define IGNBRK	0x00000400
#define IGNCR	0x00000800
#define IGNPAR	0x00001000
#define INLCR	0x00002000
#define INPCK	0x00004000
#define ISTRIP	0x00008000
#define IXOFF	0x00010000
#define IXON	0x00020000
#define PARMRK	0x00040000
#define	IXANY		0x00080000	/* any char will restart after stop */
#define IMAXBEL		0x00100000	/* ring bell on input queue full */

#define OPOST	0x00000100

#define CLOCAL	0x00000100
#define CREAD	0x00000200
#define CS5	0x00000000
#define CS6	0x00000400
#define CS7	0x00000800
#define CS8	0x00000c00
#define CSIZE	0x00000c00
#define CSTOPB	0x00001000
#define HUPCL	0x00002000
#define PARENB	0x00004000
#define PARODD	0x00008000

#define ECHO	0x00000100
#define ECHOE	0x00000200
#define ECHOK	0x00000400
#define ECHONL	0x00000800
#define ICANON	0x00001000
#define IEXTEN	0x00002000
#define ISIG	0x00004000
#define NOFLSH	0x00008000
#define TOSTOP	0x00010000

#define TCIFLUSH	1
#define TCOFLUSH	2
#define TCIOFLUSH	3
#define TCOOFF		1
#define TCOON		2
#define TCIOFF		3
#define TCION		4

#define TCSADRAIN	1
#define TCSAFLUSH	2
#define TCSANOW		3

#define VEOF	1
#define VEOL	2
#define VERASE	3
#define VINTR	4
#define VKILL	5
#define VMIN	6
#define VQUIT	7
#define VSTART	8
#define VSTOP	9
#define VSUSP	10
#define VTIME	11
#define NCCS	12

#define TERMINAL_NAME 	"dumb"

typedef struct {
	uint32_t cc[NCCS];
	uint32_t cflag;
	uint32_t iflag;
	uint32_t lflag;
	uint32_t oflag;
	uint32_t ispeed;
	uint32_t ospeed;
} termios_t;

// Initialize terminal
void terminal_init();

// Initialize the terminal
file_descriptor_t* terminal_open(const char* filename, uint32_t mode);

// Returns a buffered string of input
uint32_t terminal_read(file_descriptor_t* f, void* buf, uint32_t bytes);

// Writes a string to the terminal
uint32_t terminal_write(file_descriptor_t* f, const void* buf, uint32_t nbytes);

// Get info
uint32_t terminal_stat(file_descriptor_t* f, sys_stat_type* data);

// Seek a terminal (returns error)
uint64_t terminal_llseek(file_descriptor_t* f, uint64_t offset, int whence);

// ioctl
uint32_t terminal_ioctl(file_descriptor_t* f, int request, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);

// Used for select
bool terminal_can_read(file_descriptor_t* f);
bool terminal_can_write(file_descriptor_t* f);

// Duplicate the file handle
file_descriptor_t* terminal_duplicate(file_descriptor_t* f);

// Close the terminal (the user shouldn't even call this)
uint32_t terminal_close(file_descriptor_t* fd);

#endif
