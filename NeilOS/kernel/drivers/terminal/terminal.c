#include "terminal.h"
#include <drivers/keyboard/keyboard.h>
#include <common/lib.h>

#include <drivers/filesystem/filesystem.h>
#include <program/task.h>
#include <memory/memory.h>
#include <common/log.h>
#include <syscalls/interrupt.h>

#define SCREEN_WIDTH				80
#define SCREEN_HEIGHT				25
#define FOUR_KB						(1024 * 4)

//keeps track of when enter is pressed so read can finish
volatile bool enter_pressed;
//keeps track of up to 128 characters that have been pressed currently
uint8_t buffer[MAX_TERMINAL_BUFFER_LENGTH];
//keeps track of furthest character in buffer
uint32_t pos;

//array of shifted values
uint8_t shift_map[] = {
	')', '!', '@', '#', '$', '%', '^', '&', '*', '('
};

//takes in a key and turns it into its shifted value
//inputs: keycode
//outputs: keycode's shifted value
uint8_t handle_shift(uint8_t key) {
	if (key >= '0' && key <= '9')
		return shift_map[key - '0'];
	if (key >= 'a' && key <= 'z')
		return key - 'a' + 'A';
	if (key >= 'A' && key <= 'Z')
		return key - 'A' + 'a';
	switch (key) {
		case '`':
			return '~';
		case '-':
			return '_';
		case '=':
			return '+';
		case '[':
			return '{';
		case ']':
			return '}';
		case '\\':
			return '|';
		case '\'':
			return '"';
		case ';':
			return ':';
		case ',':
			return '<';
		case '.':
			return '>';
		case '/':
			return '?';
	}
	
	return key;
}

//takes in a key and turns it into its uppercase value
//inputs: keycode
//outputs: keycode's uppercase value
uint8_t to_upper(uint8_t key) {
	if (key >= 'a' && key <= 'z')
		return key - 'a' + 'A';
	return key;
}

//takes in a key and if  ctrl- l, clears the screen.
//inputs: keycode
//outputs: none
void handle_control(uint8_t keycode) {
	// Control + L clears the screen
	if (keycode == 'l') {
		clear();
		
		// Copy over the user input so that its always last on the screen
		int i;
		for (i = 0; i < pos; i++)
			printf("%c", buffer[i]);
	} else if (keycode == 'c') {
		// Kill the current running program
		task_list_t* t = tasks;
		task_list_t* prev = tasks;
		while (t) {
			prev = t;
			t = t->next;
		}
		if (prev && prev->pcb)
			signal_send(prev->pcb, SIGINT);
	}
}

//takes in a key and converts it to its correct modified value, prints it to the screen and stores it in a buffer
//inputs: the keycode, the modified keys structure, and whether the current key is pressed or not
//outputs: none
void handle_keys(uint8_t keycode, modifier_keys_t modifier_keys, bool pressed) {
	// We only care about key pressed
	if (!pressed)
		return;
	
	// Critical section
	uint32_t flags = 0;
	cli_and_save(flags);
	
	bool has_shift = (modifier_keys & RIGHT_SHIFT) || (modifier_keys & LEFT_SHIFT);
	bool has_control = (modifier_keys & RIGHT_CONTROL) || (modifier_keys & LEFT_CONTROL);
	//bool has_alt = (modifier_keys & LEFT_ALT) || (modifier_keys & RIGHT_ALT);
	// If it's printable, print it
	if (((keycode >= ' ' && keycode <= '~') || keycode == '\n') && pos <
		MAX_TERMINAL_BUFFER_LENGTH) {
		// Check caps lock (ignored when control is down)
		if ((modifier_keys & CAPS_LOCK) && !has_control)
			keycode = to_upper(keycode);
		// Check shift
		if (has_shift)
			keycode = handle_shift(keycode);
		
		if (has_control)
			handle_control(keycode);
		else {
			// Otherwise print it to the screen
			printf("%c", keycode);
			
			buffer[pos++] = keycode;

			// Handle enter specially
			if (keycode == ENTER_KEY)
				enter_pressed = true;
		}
	} else {
		// Handle the special keys, "backspace", "arrow keys"
		if (keycode == BACKSPACE_KEY){
			if (pos > 0) {
				pos--;
				
				// Delete the character on the screen
				backspace();
			}
		}
	}
	
	restore_flags(flags);
}

void terminal_init() {
	// Clear the buffer
	memset(buffer, 0, MAX_TERMINAL_BUFFER_LENGTH);
	pos = 0;
	
	// Clear the screen
	clear();
	
	// Reset other things
	enter_pressed = false;
	
	// Register keyboard events
	register_keychange(handle_keys);
}

// Initialize the terminal
//inputs: the filename
//outputs: -1 if failed or stdout if success
file_descriptor_t* terminal_open(const char* filename, uint32_t mode) {
	int type = 0;
	if (strncmp(filename, "stdin", strlen(filename)) == 0)
		type = STDIN;
	else if (strncmp(filename, "stdout", strlen(filename)) == 0)
		type = STDOUT;
	else if (strncmp(filename, "stderr", strlen(filename)) == 0)
		type = STDERR;
	// Don't allow this to be opened more than once
	if (type != 0 && descriptors && descriptors[type]) {
		descriptors[type]->ref_count++;
		return descriptors[type];
	}
	
	// Actually perform initialization if this is the first time we are initializing
	if (type != 0)
		pos = 0;
	
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memset(d, 0, sizeof(file_descriptor_t));
	// Mark in use
	d->lock = MUTEX_UNLOCKED;
	d->type = STANDARD_INOUT_TYPE;
	d->mode = FILE_MODE_READ | FILE_MODE_WRITE | FILE_TYPE_CHARACTER;
	int namelen = strlen(filename);
	d->filename = kmalloc(namelen + 1);
	if (!d->filename) {
		kfree(d);
		return NULL;
	}
	memcpy(d->filename, filename, namelen + 1);
	
	// Assign the functions
	d->read = terminal_read;
	d->write = terminal_write;
	d->stat = terminal_stat;
	d->llseek = terminal_llseek;
	d->duplicate = terminal_duplicate;
	d->close = terminal_close;
	
	return d;
}

// Stores a buffered string of input (blocks until enter is pressed).
// The string returned must support editing of the input string
// using backspace and the arrow keys.
// inputs: file descriptor, buffer pointer and the bytes
// ouutputs: returns how many bytes were read
uint32_t terminal_read(int32_t fd, void* buf, uint32_t bytes) {
	/*static int asdf = 0;
	if (asdf == 0) {
		memcpy(buf, "bin/dash\n", 9);
		asdf = 1;
		return 9;
	} else {
		//memcpy(buf, "ls|grep cat\n", 12);
		//return 12;
		memcpy(buf, "ls\n", 3);
		return 3;
	}*/
	
	int i;

	// Reset variables
	enter_pressed = false;
	
	// Print the buffer that you have right now so that its
	// shown at the end of the screen
	for (i = 0; i < pos; i++)
		putc(buffer[i]);
	refresh_cursor_position();

	pcb_t* pcb = current_pcb;
	while (!enter_pressed && !(descriptors[fd]->mode & FILE_MODE_NONBLOCKING) &&
		   !(pcb && pcb->should_terminate)) {
		if (signal_occurring(pcb)) {
			pos = 0;
			return -EINTR;
		}
		schedule();
	}
	
	// Copy buffer over now that enter is pressed
	uint32_t min = (bytes < pos) ? bytes : pos;
	memcpy(buf, buffer, min);
	
	// Reset pos so that the buffer is thought to be empty
	// so that we can continue to print keys if there was a full
	// buffer before.
	pos = 0;
	
	// Return bytes read
	return min;
}

// Writes a string to the terminal (returns how many bytes were written)
//inputs: file descriptor, buffer pointer and the bytes
//outputs: -1 for failure and the bytes for success
uint32_t terminal_write(int32_t fd, const void* buf, uint32_t nbytes) {
	int i;
	
	// Loop over and print all the characters
	for(i = 0; i < nbytes; i++)
		putc(((uint8_t*)buf)[i]);
	refresh_cursor_position();
	
	// Return the number of bytes written
	return nbytes;
}

// Get info
uint32_t terminal_stat(int32_t fd, sys_stat_type* data) {
	data->dev_id = 1;
	data->size = 0;
	data->mode = descriptors[fd]->mode;
	return 0;
}

// Seek a terminal (not supported).
uint64_t terminal_llseek(int32_t fd, uint64_t offset, int whence) {
	return uint64_make(-1, -ESPIPE);
}

// Duplicate the file handle
file_descriptor_t* terminal_duplicate(file_descriptor_t* f) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memcpy(d, f, sizeof(file_descriptor_t));
	d->lock = MUTEX_UNLOCKED;
	
	return d;
}

// Close the terminal
uint32_t terminal_close(file_descriptor_t* fd) {
	return 0;
}
