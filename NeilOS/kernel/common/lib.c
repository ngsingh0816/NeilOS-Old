/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab
 */

#include "lib.h"
#include <memory/memory.h>
#include <drivers/video/video.h>

#define VIDEO 0xB8000
#define NUM_COLS 80//(RESOLUTION_X / 8)
#define NUM_ROWS 25//(RESOLUTION_Y / 16)
#define ATTRIB 0x7
#define BLUE_BACKGROUND_WHITE_TEXT_ATTRIB		0x1F

#define VGA_COMMAND_PORT		0x3D4
#define VGA_DATA_PORT			0x3D5
#define VGA_INDEX_REGISTER		0x0E
#define VGA_CRT_CONTROLLER_REGISTER	0x0A
#define VGA_LINE_REGISTER		0x0B

int32_t screen_x = 0, screen_y = 0;

static int current_attrib = ATTRIB;
static char* video_mem = (char *)VIDEO;

int32_t cursor_position_get_x() {
	return screen_x;
}

int32_t cursor_position_get_y() {
	return screen_y;
}

void cursor_position_set_x(int32_t x) {
	screen_x = x;
	if (screen_x < 0)
		screen_x = 0;
	else if (screen_x >= NUM_COLS)
		screen_x = NUM_COLS-1;
}

void cursor_position_set_y(int32_t y) {
	screen_y = y;
	if (screen_y < 0)
		screen_y = 0;
	else if (screen_y >= NUM_ROWS)
		screen_y = NUM_ROWS-1;
}

// Move the cursor pointer back one (change buffer in function which calls this)
//inputs: specific terminal screen
//outputs: none
//side effects: prints a backspace to the screen
void backspace(uint8_t key) {
	int32_t next_x = screen_x - 1;
	int32_t next_y = screen_y;
	if (next_x < 0) {
		next_x += NUM_COLS;
		next_y--;
	}
	
	// Delete both the ^ and the X (e.g.) when backspacing
	int loops = 1;
	if (key + 0x40 == *(uint8_t *)(video_mem + ((NUM_COLS*next_y + next_x) << 1)))
		loops = 2;
	for (int z = 0; z < loops; z++) {
		// Move back one space
		screen_x--;
		if (screen_x < 0) {
			screen_x += NUM_COLS;
			screen_y--;
		}
		
		// Place an empty space
		*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1)) = ' ';
		*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = current_attrib;
		
		set_rectangle(screen_x * 8, screen_y * 16, 8, 16, 0, 0, 0);
	}
	
	// Update the cursor
	cursor_position_set(screen_x, screen_y);
}

void reverse_line_feed() {
	screen_x = 0;
	screen_y--;
	
	int x_pos, y_pos;		// For iterating over the screen
	// We've gone past the screen, shift everything up 1 row
	if (screen_y < 0) {
		for (y_pos = 1; y_pos < NUM_ROWS; y_pos++) {
			for (x_pos = 0; x_pos < NUM_COLS; x_pos++) {
				uint8_t last_char = *(uint8_t *)(video_mem +
												 ((NUM_COLS*(y_pos-1) + x_pos) << 1));
				uint8_t last_attrib = *(uint8_t *)(video_mem +
												   ((NUM_COLS*(y_pos-1) + x_pos) << 1) + 1);
				
				*(uint8_t *)(video_mem + ((NUM_COLS*y_pos + x_pos) << 1)) = last_char;
				*(uint8_t *)(video_mem + ((NUM_COLS*y_pos + x_pos) << 1) + 1) = last_attrib;
			}
		}
		
		// Clear the first row
		for (x_pos = 0; x_pos < NUM_COLS; x_pos++) {
			*(uint8_t *)(video_mem + (x_pos << 1)) = ' ';
			*(uint8_t *)(video_mem + (x_pos << 1) + 1) = current_attrib;
		}
		
		// Update the new screen position
		screen_y = 0;
	}
}

void clear_display(int type) {
	if (type == 2 || type == 3) {
		clear();
		return;
	}
	
	int32_t start_x = screen_x, start_y = screen_y;
	int32_t end_x = NUM_COLS-1, end_y = NUM_ROWS-1;
	if (type == 1) {
		start_x = 0;
		start_y = 0;
		end_x = screen_x;
		end_y = screen_y;
	}
	
	for (int y = start_y; y <= end_y; y++) {
		for (int x = start_x; x < end_x; x++) {
			*(uint8_t *)(video_mem + ((NUM_COLS*y + x) << 1)) = ' ';
			*(uint8_t *)(video_mem + ((NUM_COLS*y + x) << 1) + 1) = current_attrib;
		}
	}
}

void clear_line(int type) {
	int32_t start_x = screen_x;
	int32_t end_x = NUM_COLS-1;
	if (type == 1) {
		start_x = 0;
		end_x = screen_x;
	} else if (type == 2) {
		start_x = 0;
		end_x = screen_x;
	}
	
	for (int x = start_x; x < end_x; x++) {
		*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + x) << 1)) = ' ';
		*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + x) << 1) + 1) = current_attrib;
	}
}

/*
* void clear(void);
*   Inputs: void
*   Return Value: none
*	Function: Clears video memory
*/

void
clear(void)
{
    int32_t i;
    for(i=0; i<NUM_ROWS*NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
    }
	
	set_rectangle(0, 0, RESOLUTION_X, RESOLUTION_Y, 0, 0, 0);
	
	// Reset the cursor position
	cursor_position_set(0, 0);
	current_attrib = ATTRIB;
}

/*
 * cursor_position_set()
 *	Inputs: x: the new x cursor position (0, 79)
			y: the new y cursor position (0, 24)
 *	Ouptuts: none
 *  Return Value: none
 *	Function: Sets the cursor to a new position
 */

void cursor_position_set(int32_t x, int32_t y) {
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x >= NUM_COLS)
		x = NUM_COLS - 1;
	if (y >= NUM_ROWS)
		y = NUM_ROWS - 1;
	unsigned int position = y * NUM_COLS + x;
	
	// Set the low byte to the VGA Index Register
	outw(VGA_INDEX_REGISTER+1, VGA_COMMAND_PORT);
	outb(position & 0xFF, VGA_DATA_PORT);
	
	// Set the high byte to the VGA Index Register
	outw(VGA_INDEX_REGISTER, VGA_COMMAND_PORT);
	outb((position >> 8) & 0xFF, VGA_DATA_PORT);
	
	screen_x = x;
	screen_y = y;
}

void cursor_position_refresh() {
	cursor_position_set(screen_x, screen_y);
}

/*
 * show_cursor()
 * Inputs: show - shows the cursor if true, otherwise hides it
 * Function - ^
 */
void show_cursor(bool show) {
	// Set or clear bit 5 of the CRT Controller Register
	outb(VGA_CRT_CONTROLLER_REGISTER, VGA_COMMAND_PORT);
	outb(show ? 0x00 : 0x20, VGA_DATA_PORT);
	
	// Clear the line register
	outb(VGA_LINE_REGISTER, VGA_COMMAND_PORT);
	outb(0x0, VGA_DATA_PORT);
}

/*
 * void blue_screen(message);
 *	Inputs: message - the message to be displayed during the blue screen
 *	Return Value: none
 *	Function: shows that an exception has occurred in a fun way!
 */
void blue_screen(char* message, ...) {
	// Go back to vga
	video_deinit();

	// Diable any more interrupts
	cli();
		
	// Clear the screen and set the background color to blue
	// with white text
	int32_t i;
	for(i=0; i<NUM_ROWS*NUM_COLS; i++) {
		*(uint8_t *)(video_mem + (i << 1)) = ' ';
		*(uint8_t *)(video_mem + (i << 1) + 1) = BLUE_BACKGROUND_WHITE_TEXT_ATTRIB;
	}
	screen_x = 0;
	screen_y = 0;
	
	// Hide the cursor
	show_cursor(false);
	
	current_attrib = BLUE_BACKGROUND_WHITE_TEXT_ATTRIB;
	printf("A problem has been detected and NeilOS has been shut"
		   "down to prevent damage\nto your computer.\n\n");
	printf("The problem seems to be caused by:\n\n");
	
	// Print message
	int32_t* esp = (void *)&message;
	esp++;
	char* buf = message;
	while(*buf != '\0') {
		switch(*buf) {
			case '%':
			{
				int32_t alternate = 0;
				buf++;
				
			format_char_switch:
				/* Conversion specifiers */
				switch(*buf) {
						/* Print a literal '%' character */
					case '%':
						putc('%');
						break;
						
						/* Use alternate formatting */
					case '#':
						alternate = 1;
						buf++;
						/* Yes, I know gotos are bad.  This is the
						 * most elegant and general way to do this,
						 * IMHO. */
						goto format_char_switch;
						
						/* Print a number in hexadecimal form */
					case 'x':
					{
						int8_t conv_buf[64];
						if(alternate == 0) {
							itoa(*((uint32_t *)esp), conv_buf, 16);
							puts(conv_buf);
						} else {
							int32_t starting_index;
							int32_t i;
							itoa(*((uint32_t *)esp), &conv_buf[8], 16);
							i = starting_index = strlen(&conv_buf[8]);
							while(i < 8) {
								conv_buf[i] = '0';
								i++;
							}
							puts(&conv_buf[starting_index]);
						}
						esp++;
					}
						break;
						
						/* Print a number in unsigned int form */
					case 'u':
					{
						int8_t conv_buf[36];
						itoa(*((uint32_t *)esp), conv_buf, 10);
						puts(conv_buf);
						esp++;
					}
						break;
						
						/* Print a number in signed int form */
					case 'd':
					{
						int8_t conv_buf[36];
						int32_t value = *((int32_t *)esp);
						if(value < 0) {
							conv_buf[0] = '-';
							itoa(-value, &conv_buf[1], 10);
						} else {
							itoa(value, conv_buf, 10);
						}
						puts(conv_buf);
						esp++;
					}
						break;
						
						/* Print a single character */
					case 'c':
						putc( (uint8_t) *((int32_t *)esp) );
						esp++;
						break;
						
						/* Print a NULL-terminated string */
					case 's':
						puts( *((int8_t **)esp) );
						esp++;
						break;
						
					default:
						break;
				}
				
			}
				break;
				
			default:
				putc(*buf);
				break;
		}
		buf++;
	}
	
	
	printf("\n\n");
	printf("If this is the first time you've seen this Stop error screen,\n"
		   "restart your computer. If this screen appears again, follow\nthese steps:\n\n");
	printf("Check to make sure any new hardware or software is properly installed.\n"
		   "If this is a new installation, ask your hardware or software manufacturer\n"
		   "for any windows updates you might need.\n\n");
	printf("If problems continue, disable or remove any newly installed hardware\n"
		   "or software. Disable BIOS memory options such as caching or shadowing.\n"
		   "There is no safe mode on this computer, so don't try to use it.\n\n");
	printf("Thank you for using NeilOS! :)\n");
	current_attrib = ATTRIB;
	
	show_cursor(true);
	
	/* Spin (nicely, so we don't chew up cycles) */
	asm volatile("halt: hlt; jmp halt;");
}

int32_t sprintf(char* str, const int8_t* format, ...) {
	/* Pointer to the format string */
	const int8_t* buf = format;
	
	/* Stack pointer for the other parameters */
	int32_t* esp = (void *)&format;
	esp++;
	
	int32_t ptr = 0;
	
	while(*buf != '\0') {
		switch(*buf) {
			case '%':
			{
				int32_t alternate = 0;
				buf++;
				
			format_char_switch:
				/* Conversion specifiers */
				switch(*buf) {
						/* Print a literal '%' character */
					case '%':
						str[ptr++] = '%';
						break;
						
						/* Use alternate formatting */
					case '#':
						alternate = 1;
						buf++;
						/* Yes, I know gotos are bad.  This is the
						 * most elegant and general way to do this,
						 * IMHO. */
						goto format_char_switch;
						
						/* Print a number in hexadecimal form */
					case 'x':
					{
						int8_t conv_buf[64];
						if(alternate == 0) {
							itoa(*((uint32_t *)esp), conv_buf, 16);
							int32_t len = strlen(conv_buf);
							memcpy(&str[ptr], conv_buf, len);
							ptr += len;
						} else {
							int32_t starting_index;
							int32_t i;
							itoa(*((uint32_t *)esp), &conv_buf[8], 16);
							i = starting_index = strlen(&conv_buf[8]);
							while(i < 8) {
								conv_buf[i] = '0';
								i++;
							}
							int32_t len = strlen(&conv_buf[starting_index]);
							memcpy(&str[ptr], &conv_buf[starting_index], len);
							ptr += len;
						}
						esp++;
					}
						break;
						
						/* Print a number in unsigned int form */
					case 'u':
					{
						int8_t conv_buf[36];
						itoa(*((uint32_t *)esp), conv_buf, 10);
						int32_t len = strlen(conv_buf);
						memcpy(&str[ptr], conv_buf, len);
						ptr += len;
						esp++;
					}
						break;
						
						/* Print a number in signed int form */
					case 'i':
					case 'd':
					{
						int8_t conv_buf[36];
						int32_t value = *((int32_t *)esp);
						if(value < 0) {
							conv_buf[0] = '-';
							itoa(-value, &conv_buf[1], 10);
						} else {
							itoa(value, conv_buf, 10);
						}
						int32_t len = strlen(conv_buf);
						memcpy(&str[ptr], conv_buf, len);
						ptr += len;
						esp++;
					}
						break;
						
						/* Print a single character */
					case 'c':
						str[ptr++] = (uint8_t)*((int32_t*)esp);
						esp++;
						break;
						
						/* Print a NULL-terminated string */
					case 's': {
						char* string = *((int8_t **)esp);
						int32_t len = strlen(string);
						memcpy(&str[ptr], string, len);
						ptr += len;
						esp++;
						break;
					}
					
					default:
						break;
				}
				
			}
				break;
				
			default:
				str[ptr++] = *buf;
				break;
		}
		buf++;
	}
	
	str[ptr] = 0;
	return ptr;
}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output.
 * */
int32_t
printf(int8_t *format, ...)
{
	// Critical section
	// Needed to prevent freezes while executing a new probgram while writing (odn't know why)
	uint32_t flags;
	cli_and_save(flags);
	
	/* Pointer to the format string */
	int8_t* buf = format;

	/* Stack pointer for the other parameters */
	int32_t* esp = (void *)&format;
	esp++;

	while(*buf != '\0') {
		switch(*buf) {
			case '%':
				{
					int32_t alternate = 0;
					buf++;

format_char_switch:
					/* Conversion specifiers */
					switch(*buf) {
						/* Print a literal '%' character */
						case '%':
							putc('%');
							break;

						/* Use alternate formatting */
						case '#':
							alternate = 1;
							buf++;
							/* Yes, I know gotos are bad.  This is the
							 * most elegant and general way to do this,
							 * IMHO. */
							goto format_char_switch;

						/* Print a number in hexadecimal form */
						case 'x':
							{
								int8_t conv_buf[64];
								if(alternate == 0) {
									itoa(*((uint32_t *)esp), conv_buf, 16);
									puts(conv_buf);
								} else {
									int32_t starting_index;
									int32_t i;
									itoa(*((uint32_t *)esp), &conv_buf[8], 16);
									i = starting_index = strlen(&conv_buf[8]);
									while(i < 8) {
										conv_buf[i] = '0';
										i++;
									}
									puts(&conv_buf[starting_index]);
								}
								esp++;
							}
							break;

						/* Print a number in unsigned int form */
						case 'u':
							{
								int8_t conv_buf[36];
								itoa(*((uint32_t *)esp), conv_buf, 10);
								puts(conv_buf);
								esp++;
							}
							break;

						/* Print a number in signed int form */
						case 'i':
						case 'd':
							{
								int8_t conv_buf[36];
								int32_t value = *((int32_t *)esp);
								if(value < 0) {
									conv_buf[0] = '-';
									itoa(-value, &conv_buf[1], 10);
								} else {
									itoa(value, conv_buf, 10);
								}
								puts(conv_buf);
								esp++;
							}
							break;

						/* Print a single character */
						case 'c':
							putc( (uint8_t) *((int32_t *)esp) );
							esp++;
							break;

						/* Print a NULL-terminated string */
						case 's':
							puts( *((int8_t **)esp) );
							esp++;
							break;

						default:
							break;
					}

				}
				break;

			default:
				putc(*buf);
				break;
		}
		buf++;
	}
	
	// Move the cursor
	cursor_position_set(screen_x, screen_y);
	
	// End critical section
	restore_flags(flags);

	return (buf - format);
}

/*
* int32_t puts(int8_t* s);
*   Inputs: int_8* s = pointer to a string of characters
*   Return Value: Number of bytes written
*	Function: Output a string to the console 
*/

int32_t
puts(int8_t* s)
{
	register int32_t index = 0;
	while(s[index] != '\0') {
		putc(s[index]);
		index++;
	}

	return index;
}

/*
* void putc(uint8_t c);
*   Inputs: uint_8* c = character to print
*   Return Value: void
*	Function: Output a character to the console 
*/

void
putc(uint8_t c)
{
    if(c == '\n' || c == '\r') {
        screen_y++;
        screen_x=0;
    } else {
        *(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = current_attrib;
		
		set_rectangle(screen_x * 8, screen_y * 16, 8, 16, 255, 255, 255);
		
        screen_x++;
        screen_y = (screen_y + (screen_x / NUM_COLS));
		screen_x %= NUM_COLS;
    }
	
	int x_pos, y_pos;		// For iterating over the screen
	// We've gone past the screen, shift everything up 1 row
	while (screen_y >= NUM_ROWS) {
		for (y_pos = 0; y_pos < NUM_ROWS - 1; y_pos++) {
			for (x_pos = 0; x_pos < NUM_COLS; x_pos++) {
				uint8_t last_char = *(uint8_t *)(video_mem +
								((NUM_COLS*(y_pos+1) + x_pos) << 1));
				uint8_t last_attrib = *(uint8_t *)(video_mem +
								((NUM_COLS*(y_pos+1) + x_pos) << 1) + 1);
				
				*(uint8_t *)(video_mem + ((NUM_COLS*y_pos + x_pos) << 1)) = last_char;
				*(uint8_t *)(video_mem + ((NUM_COLS*y_pos + x_pos) << 1) + 1) =
				last_attrib;
				
				set_rectangle(x_pos * 8, y_pos * 16, 8, 16, 255, 255, 255);
			}
		}
		
		// Clear the last row
		for (x_pos = 0; x_pos < NUM_COLS; x_pos++) {
			*(uint8_t *)(video_mem + ((NUM_COLS*(NUM_ROWS-1) + x_pos) << 1)) = ' ';
			*(uint8_t *)(video_mem + ((NUM_COLS*(NUM_ROWS-1) + x_pos) << 1) + 1) =
				current_attrib;
			
			set_rectangle(x_pos * 8, y_pos * 16, 8, 16, 0, 0, 0);
		}
		
		// Update the new screen position
		screen_y--;
	}
}

/*
* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
*   Inputs: uint32_t value = number to convert
*			int8_t* buf = allocated buffer to place string in
*			int32_t radix = base system. hex, oct, dec, etc.
*   Return Value: number of bytes written
*	Function: Convert a number to its ASCII representation, with base "radix"
*/

int8_t*
itoa(uint32_t value, int8_t* buf, int32_t radix)
{
	static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	int8_t *newbuf = buf;
	int32_t i;
	uint32_t newval = value;

	/* Special case for zero */
	if(value == 0) {
		buf[0]='0';
		buf[1]='\0';
		return buf;
	}

	/* Go through the number one place value at a time, and add the
	 * correct digit to "newbuf".  We actually add characters to the
	 * ASCII string from lowest place value to highest, which is the
	 * opposite of how the number should be printed.  We'll reverse the
	 * characters later. */
	while(newval > 0) {
		i = newval % radix;
		*newbuf = lookup[i];
		newbuf++;
		newval /= radix;
	}

	/* Add a terminating NULL */
	*newbuf = '\0';

	/* Reverse the string and return */
	return strrev(buf);
}

/*
* int8_t* strrev(int8_t* s);
*   Inputs: int8_t* s = string to reverse
*   Return Value: reversed string
*	Function: reverses a string s
*/

int8_t*
strrev(int8_t* s)
{
	register int8_t tmp;
	register int32_t beg=0;
	register int32_t end=strlen(s) - 1;

	while(beg < end) {
		tmp = s[end];
		s[end] = s[beg];
		s[beg] = tmp;
		beg++;
		end--;
	}

	return s;
}

/*
* uint32_t strlen(const int8_t* s);
*   Inputs: const int8_t* s = string to take length of
*   Return Value: length of string s
*	Function: return length of string s
*/

uint32_t
strlen(const int8_t* s)
{
	register uint32_t len = 0;
	while(s[len] != '\0')
		len++;

	return len;
}

/*
* void* memset(void* s, int32_t c, uint32_t n);
*   Inputs: void* s = pointer to memory
*			int32_t c = value to set memory to
*			uint32_t n = number of bytes to set
*   Return Value: new string
*	Function: set n consecutive bytes of pointer s to value c
*/

void*
memset(void* s, int32_t c, uint32_t n)
{
	c &= 0xFF;
	asm volatile("                  \n\
			.memset_top:            \n\
			testl   %%ecx, %%ecx    \n\
			jz      .memset_done    \n\
			testl   $0x3, %%edi     \n\
			jz      .memset_aligned \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			subl    $1, %%ecx       \n\
			jmp     .memset_top     \n\
			.memset_aligned:        \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			movl    %%ecx, %%edx    \n\
			shrl    $2, %%ecx       \n\
			andl    $0x3, %%edx     \n\
			cld                     \n\
			rep     stosl           \n\
			.memset_bottom:         \n\
			testl   %%edx, %%edx    \n\
			jz      .memset_done    \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			subl    $1, %%edx       \n\
			jmp     .memset_bottom  \n\
			.memset_done:           \n\
			"
			:
			: "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/*
* void* memset_word(void* s, int32_t c, uint32_t n);
*   Inputs: void* s = pointer to memory
*			int32_t c = value to set memory to
*			uint32_t n = number of bytes to set
*   Return Value: new string
*	Function: set lower 16 bits of n consecutive memory locations of pointer s to value c
*/

/* Optimized memset_word */
void*
memset_word(void* s, int32_t c, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			rep     stosw           \n\
			"
			:
			: "a"(c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/*
* void* memset_dword(void* s, int32_t c, uint32_t n);
*   Inputs: void* s = pointer to memory
*			int32_t c = value to set memory to
*			uint32_t n = number of bytes to set
*   Return Value: new string
*	Function: set n consecutive memory locations of pointer s to value c
*/

void*
memset_dword(void* s, int32_t c, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			rep     stosl           \n\
			"
			:
			: "a"(c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/*
* void* memcpy(void* dest, const void* src, uint32_t n);
*   Inputs: void* dest = destination of copy
*			const void* src = source of copy
*			uint32_t n = number of byets to copy
*   Return Value: pointer to dest
*	Function: copy n bytes of src to dest
*/

__attribute__((noinline)) void*
memcpy(void* dest, const void* src, uint32_t n)
{
	asm volatile("                  \n\
			._memcpy_top:            \n\
			testl   %%ecx, %%ecx    \n\
			jz      ._memcpy_done    \n\
			testl   $0x3, %%edi     \n\
			jz      ._memcpy_aligned \n\
			movb    (%%esi), %%al   \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			addl    $1, %%esi       \n\
			subl    $1, %%ecx       \n\
			jmp     ._memcpy_top     \n\
			._memcpy_aligned:        \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			movl    %%ecx, %%edx    \n\
			shrl    $2, %%ecx       \n\
			andl    $0x3, %%edx     \n\
			cld                     \n\
			rep     movsl           \n\
			._memcpy_bottom:         \n\
			testl   %%edx, %%edx    \n\
			jz      ._memcpy_done    \n\
			movb    (%%esi), %%al   \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			addl    $1, %%esi       \n\
			subl    $1, %%edx       \n\
			jmp     ._memcpy_bottom  \n\
			._memcpy_done:           \n\
			"
			:
			: "S"(src), "D"(dest), "c"(n)
			: "eax", "edx", "memory", "cc"
			);
	
	return dest;
}

/*
* void* memmove(void* dest, const void* src, uint32_t n);
*   Inputs: void* dest = destination of move
*			const void* src = source of move
*			uint32_t n = number of byets to move
*   Return Value: pointer to dest
*	Function: move n bytes of src to dest
*/

/* Optimized memmove (used for overlapping memory areas) */
void*
memmove(void* dest, const void* src, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			cmp     %%edi, %%esi    \n\
			jae     .memmove_go     \n\
			leal    -1(%%esi, %%ecx), %%esi    \n\
			leal    -1(%%edi, %%ecx), %%edi    \n\
			std                     \n\
			.memmove_go:            \n\
			rep     movsb           \n\
			"
			:
			: "D"(dest), "S"(src), "c"(n)
			: "edx", "memory", "cc"
			);

	return dest;
}

/*
* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
*   Inputs: const int8_t* s1 = first string to compare
*			const int8_t* s2 = second string to compare
*			uint32_t n = number of bytes to compare
*	Return Value: A zero value indicates that the characters compared 
*					in both strings form the same string.
*				A value greater than zero indicates that the first 
*					character that does not match has a greater value 
*					in str1 than in str2; And a value less than zero 
*					indicates the opposite.
*	Function: compares string 1 and string 2 for equality
*/

int32_t
strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
{
	int32_t i;
	for(i=0; i<n; i++) {
		if( (s1[i] != s2[i]) ||
				(s1[i] == '\0') /* || s2[i] == '\0' */ ) {

			/* The s2[i] == '\0' is unnecessary because of the short-circuit
			 * semantics of 'if' expressions in C.  If the first expression
			 * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
			 * s2[i], then we only need to test either s1[i] or s2[i] for
			 * '\0', since we know they are equal. */

			return s1[i] - s2[i];
		}
	}
	return 0;
}

/*
* int8_t* strcpy(int8_t* dest, const int8_t* src)
*   Inputs: int8_t* dest = destination string of copy
*			const int8_t* src = source string of copy
*   Return Value: pointer to dest
*	Function: copy the source string into the destination string
*/

int8_t*
strcpy(int8_t* dest, const int8_t* src)
{
	int32_t i=0;
	while(src[i] != '\0') {
		dest[i] = src[i];
		i++;
	}

	dest[i] = '\0';
	return dest;
}

/*
* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
*   Inputs: int8_t* dest = destination string of copy
*			const int8_t* src = source string of copy
*			uint32_t n = number of bytes to copy
*   Return Value: pointer to dest
*	Function: copy n bytes of the source string into the destination string
*/

int8_t*
strncpy(int8_t* dest, const int8_t* src, uint32_t n)
{
	int32_t i=0;
	while(src[i] != '\0' && i < n) {
		dest[i] = src[i];
		i++;
	}

	while(i < n) {
		dest[i] = '\0';
		i++;
	}

	return dest;
}
