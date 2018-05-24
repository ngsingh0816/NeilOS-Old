
#include "keyboard.h"
#include <drivers/pic/i8259.h>
#include <syscalls/interrupt.h>
#include <common/lib.h>
#include <drivers/mouse/mouse.h>

#define KEYBOARD_IRQ		0x1

#define KEYBOARD_DATA_PORT			0x60
#define KEYBOARD_COMMAND_PORT		0x64

#define DATA_BIT					0x1
#define MOUSE_BIT					(1 << 5)

#define KEYBOARD_DISABLE1			0xAD
#define KEYBOARD_DISABLE2			0xA7

#define KEYBOARD_WRITE_CONFIG		0x60
#define KEYBOARD_CONFIG				0x05

#define KEYBOARD_ENABLE				0xAE
#define KEYBOARD_RESET				0xFF

//initializes array of all keyboard characters
uint8_t basic_keycodes_press[0x84] = {
	0, F9_KEY, 0, F5_KEY,
	F3_KEY, F1_KEY, F2_KEY, F12_KEY,
	0, F10_KEY, F8_KEY, F6_KEY,
	F4_KEY, TAB_KEY, '`', 0,
	0, LEFT_ALT_KEY, LEFT_SHIFT_KEY, 0,
	LEFT_CONTROL_KEY, 'q', '1', 0,
	0, 0, 'z', 's',
	'a', 'w', '2', 0,
	0, 'c', 'x', 'd',
	'e', '4', '3', 0,
	0, ' ', 'v', 'f',
	't', 'r', '5', 0,
	0, 'n', 'b', 'h',
	'g', 'y', '6', 0,
	0, 0, 'm', 'j',
	'u', '7', '8', 0,
	0, ',', 'k', 'i',
	'o', '0', '9', 0,
	0, '.', '/', 'l',
	';', 'p', '-', 0,
	0, 0, '\'', 0,
	'[', '=', 0, 0,
	CAPS_LOCK_KEY, RIGHT_SHIFT_KEY, ENTER_KEY, ']',
	0, '\\', 0, 0,
	0, 0, 0, 0,
	0, 0, BACKSPACE_KEY, 0,
	0, '1', 0, LEFT_ARROW_KEY,
	'7', 0, 0, 0,
	'0', '.', DOWN_ARROW_KEY, '5',
	RIGHT_ARROW_KEY, UP_ARROW_KEY, ESCAPE_KEY, NUM_LOCK_KEY,
	F11_KEY, '+', '3', '-',
	'*', '9', SCROLL_LOCK_KEY, 0,
	0, 0, 0, F7_KEY
};

//set up truth variables 
bool lastWasF0 = false, lastWasE0 = false;

// Allow other things to handle interrupts from the keyboard
void (*user_handler)(uint8_t key, modifier_keys_t mkeys, bool pressed);
modifier_keys_t modifier_keys;

volatile unsigned int keyboard_packet_id = 0;
unsigned char keyboard_code = 0;
bool keyboard_pressed = false;

/* special_scancode
	DESCRIPTION: This checks for special characters
	INPUTS: the pointer to a bool that tells if a special character is pressed
	OUTPUTS: Returns character that was pressed, otherwise returns 0 if none of the special characters are pressed

*/
unsigned char special_scancode(bool* pressed) {
	unsigned char input = inb(KEYBOARD_DATA_PORT);
	*pressed = !lastWasF0;
	
	lastWasF0 = false;
	lastWasE0 = false;
	
	switch (input) {
		case 0x11:
			return RIGHT_ALT_KEY;
		case 0x14:
			return RIGHT_CONTROL_KEY;
		case 0x1F:
			return LEFT_COMMAND_KEY;
		case 0x27:
			return RIGHT_COMMAND_KEY;
		case 0x4A:
			return '/';
		case 0x5A:
			return ENTER_KEY;
		case 0x69:
			return END_KEY;
		case 0x6B:
			return LEFT_ARROW_KEY;
		case 0x6C:
			return HOME_KEY;
		case 0x70:
			return INSERT_KEY;
		case 0x71:
			return DELETE_KEY;
		case 0x72:
			return DOWN_ARROW_KEY;
		case 0x74:
			return RIGHT_ARROW_KEY;
		case 0x75:
			return UP_ARROW_KEY;
		case 0x7A:
			return PAGE_DOWN_KEY;
		case 0x7D:
			return PAGE_UP_KEY;
		case 0xF0:
			lastWasE0 = true;
			lastWasF0 = true;
			return 0;
	}
	
	return 0;
}

/*	interpret_scancode 
	DESCRIPTION: checks the current keyboard data port and outputs a character accordingly
	INPUTS: pointer to bool with key pressed
	OUTPUTS: unsigned char of which character was pressed
*/
unsigned char interpret_scancode(bool* pressed) {
	if (lastWasE0) {
		return special_scancode(pressed);
	}
	
	unsigned char input = inb(KEYBOARD_DATA_PORT);
	
	if (input < 0x84) {
		*pressed = !lastWasF0;
		lastWasF0 = false;
		lastWasE0 = false;
		return basic_keycodes_press[input];
	} else if (input == 0xE0) {
		lastWasE0 = true;
		return 0;
	} else if (input == 0xF0) {
		lastWasF0 = true;
		return 0;
	}
	
	return 0;
}

void update_modifier_keys(unsigned char keycode, bool pressed) {
	// Update the modifier keys
	if (keycode == LEFT_SHIFT_KEY) {
		if (pressed)
			modifier_keys |= LEFT_SHIFT;
		else
			modifier_keys &= ~LEFT_SHIFT;
	} else if (keycode == RIGHT_SHIFT_KEY) {
		if (pressed)
			modifier_keys |= RIGHT_SHIFT;
		else
			modifier_keys &= ~RIGHT_SHIFT;
	} else if (keycode == CAPS_LOCK_KEY && pressed) {
		if (modifier_keys & CAPS_LOCK)
			modifier_keys &= ~CAPS_LOCK;
		else
			modifier_keys |= CAPS_LOCK;
	} else if (keycode == LEFT_CONTROL_KEY) {
		if (pressed)
			modifier_keys |= LEFT_CONTROL;
		else
			modifier_keys &= ~LEFT_CONTROL;
	} else if (keycode == RIGHT_CONTROL_KEY) {
		if (pressed)
			modifier_keys |= RIGHT_CONTROL;
		else
			modifier_keys &= ~RIGHT_CONTROL;
	} else if (keycode == LEFT_ALT_KEY) {
		if (pressed)
			modifier_keys |= LEFT_ALT;
		else
			modifier_keys &= ~LEFT_ALT;
	} else if (keycode == RIGHT_ALT_KEY) {
		if (pressed)
			modifier_keys |= RIGHT_ALT;
		else
			modifier_keys &= ~RIGHT_ALT;
	}
}

void keyboard_handle() {
	//set-up a boolean and then pass its memory address to interpret scancode.
	//scancode will change pressed accordingly.
	bool pressed = false;
	unsigned char keycode = interpret_scancode(&pressed);
	
	update_modifier_keys(keycode, pressed);
	
	// Let the devices know
	if (keycode != 0) {
		keyboard_code = keycode;
		keyboard_pressed = pressed;
		keyboard_packet_id++;
	}
	
	// Call the user handler if it exists
	if (user_handler && keycode != 0)
		user_handler(keycode, modifier_keys, pressed);
}

/*	keyboard_handler 
	DESCRIPTION: handles a keyboard interrupt
	INPUTS: irq number
	OUTPUTS: none
*/
void keyboard_handler(int irq) {
	// Disable this interrupt
	disable_irq(KEYBOARD_IRQ);
	
	// Say the we've handled this interrupt
	send_eoi(irq);
	
	int status = inb(KEYBOARD_COMMAND_PORT);
	while (status & DATA_BIT) {
		if (status & MOUSE_BIT)
			mouse_handle();
		else
			keyboard_handle();
		status = inb(KEYBOARD_COMMAND_PORT);
	}
	
	enable_irq(KEYBOARD_IRQ);
}

// Initialize the keyboard, write to ports, returns false on failure
// inputs: none
// outputs: boolean for succes/failure
bool keyboard_init() {
	// Disable the keyboard
	outb(KEYBOARD_DISABLE1, KEYBOARD_COMMAND_PORT);
	outb(KEYBOARD_DISABLE2, KEYBOARD_COMMAND_PORT);
	
	// Flush the output buffer
	inb(KEYBOARD_DATA_PORT);
	
	// Set the configuration
	outb(KEYBOARD_WRITE_CONFIG, KEYBOARD_COMMAND_PORT);
	outb(KEYBOARD_CONFIG, KEYBOARD_DATA_PORT);
	
	// Register the keyboard handler
	request_irq(KEYBOARD_IRQ, keyboard_handler);
	
	// Enable and reset the keyboard
	outb(KEYBOARD_ENABLE, KEYBOARD_COMMAND_PORT);
	outb(KEYBOARD_RESET, KEYBOARD_COMMAND_PORT);
	
	// Enable interrupts for the keyboard
	enable_irq(KEYBOARD_IRQ);
	
	// Clear register
	modifier_keys = NONE;
	lastWasE0 = false;
	lastWasF0 = false;
	
	return true;
}

void register_keychange(void (*handler)(uint8_t key, modifier_keys_t modifier_keys,
									   bool pressed)) {
	user_handler = handler;
}

// Initialize the keyboard
file_descriptor_t* keyboard_open(const char* filename, uint32_t mode) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memset(d, 0, sizeof(file_descriptor_t));
	// Mark in use
	d->lock = MUTEX_UNLOCKED;
	d->type = KEYBOARD_FILE_TYPE;
	d->mode = FILE_TYPE_CHARACTER | mode;
	int namelen = strlen(filename);
	d->filename = kmalloc(namelen + 1);
	if (!d->filename) {
		kfree(d);
		return NULL;
	}
	memcpy(d->filename, filename, namelen + 1);
	
	// Assign the functions
	d->read = keyboard_read;
	d->write = keyboard_write;
	d->stat = keyboard_stat;
	d->llseek = keyboard_llseek;
	d->duplicate = keyboard_duplicate;
	d->close = keyboard_close;
	
	return d;
}

/* Keyboard Packet is of the format (2 bytes)
 {
 	unsigned char pressed;
 	unsigned char keycode;
 }
 */

#define KEYBOARD_STRUCT_SIZE		2

// Returns the last keyboard command if in nonblocking mode, otherwise blocks until key is pressed / released
uint32_t keyboard_read(file_descriptor_t* f, void* buf, uint32_t bytes) {
	if (bytes != KEYBOARD_STRUCT_SIZE)
		return -EINVAL;
	
	if (!(f->mode & FILE_MODE_NONBLOCKING)) {
		unsigned int id = keyboard_packet_id;
		pcb_t* pcb = current_pcb;
		while (id == keyboard_packet_id && !(pcb && pcb->should_terminate) && !f->closed) {
			if (signal_occurring(pcb))
				return -EINTR;
			up(&f->lock);
			schedule();
			down(&f->lock);
		}
	}
	
	((uint8_t*)buf)[0] = keyboard_pressed;
	((uint8_t*)buf)[1] = keyboard_code;
	
	return KEYBOARD_STRUCT_SIZE;
}

// Nop
uint32_t keyboard_write(file_descriptor_t* f, const void* buf, uint32_t nbytes) {
	return 0;
}

// Get info
uint32_t keyboard_stat(file_descriptor_t* f, sys_stat_type* data) {
	data->dev_id = f->type;
	data->size = 0;
	data->mode = f->mode;
	return 0;
}

// Seek a keyboard (returns error)
uint64_t keyboard_llseek(file_descriptor_t* f, uint64_t offset, int whence) {
	return uint64_make(-1, -ESPIPE);
}

// Duplicate the file handle
file_descriptor_t* keyboard_duplicate(file_descriptor_t* f) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memcpy(d, f, sizeof(file_descriptor_t));
	int namelen = strlen(f->filename);
	d->filename = kmalloc(namelen + 1);
	if (!d->filename) {
		kfree(d);
		return NULL;
	}
	memcpy(d->filename, f->filename, namelen + 1);
	d->lock = MUTEX_UNLOCKED;
	
	return d;
}

// Close the keyboard
uint32_t keyboard_close(file_descriptor_t* fd) {
	kfree(fd->filename);
	return 0;
}
