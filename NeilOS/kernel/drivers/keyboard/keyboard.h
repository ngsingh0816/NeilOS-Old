#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <common/types.h>
#include <syscalls/descriptor.h>

// Keycodes for this OS
#define NUL_KEY			0x00
#define EOF_KEY			0x04
#define BEL_KEY			0x07
#define BACKSPACE_KEY	0x08
#define TAB_KEY			0x09
#define ENTER_KEY		0xA
#define VERTICAL_TAB_KEY 0xB
#define NEW_PAGE_KEY	0xC
#define CARRIAGE_RETURN_KEY	0xD
#define SHIFT_OUT_KEY	0xE
#define SHIFT_IN_KEY	0xF
#define CANCEL_KEY		0x18
#define SUBSTITUTE_KEY	0x1A
#define ESCAPE_KEY		0x1B
#define DELETE_KEY		0x7F
#define F1_KEY			0x80
#define F2_KEY			0x81
#define F3_KEY			0x82
#define F4_KEY			0x83
#define F5_KEY			0x84
#define F6_KEY			0x85
#define F7_KEY			0x86
#define F8_KEY			0x87
#define F9_KEY			0x88
#define F10_KEY			0x89
#define F11_KEY			0x8A
#define F12_KEY			0x8B
#define LEFT_CONTROL_KEY 0x90
#define LEFT_SHIFT_KEY	0x91
#define RIGHT_SHIFT_KEY	0x92
#define CAPS_LOCK_KEY	0x93
#define LEFT_ALT_KEY	0x94
#define RIGHT_ALT_KEY	0x95
#define END_KEY			0x96
#define HOME_KEY		0x97
#define INSERT_KEY		0x98
#define PAGE_UP_KEY		0x99
#define PAGE_DOWN_KEY	0x9A
#define PAUSE_KEY		0x9B
#define CSI_KEY			0x9B
#define UP_ARROW_KEY	0x9C
#define DOWN_ARROW_KEY	0x9D
#define RIGHT_ARROW_KEY	0x9E
#define LEFT_ARROW_KEY	0x9F
#define RIGHT_CONTROL_KEY 0xA0
#define NUM_LOCK_KEY	0xA1
#define SCROLL_LOCK_KEY	0xA2
#define LEFT_COMMAND_KEY 0xA3
#define RIGHT_COMMAND_KEY 0xA4

// Tells what modifier keys are currently down
typedef enum {
	NONE = 0,
	LEFT_SHIFT = 1 << 0,
	RIGHT_SHIFT = 1 << 1,
	CAPS_LOCK = 1 << 2,
	LEFT_CONTROL = 1 << 3,
	RIGHT_CONTROL = 1 << 4,
	LEFT_ALT = 1 << 5,
	RIGHT_ALT = 1 << 6
} modifier_keys_t;

// Initialize the keyboard, write to ports, returns false on failure
bool keyboard_init();
// Have an external source respond to key changes
void register_keychange(void (*handler)(uint8_t key, modifier_keys_t modifier_keys,
										bool pressed));

void keyboard_handle();

// Initialize the keyboard
file_descriptor_t* keyboard_open(const char* filename, uint32_t mode);

// Returns the last keyboard command if in nonblocking mode, otherwise blocks until key is pressed / released
uint32_t keyboard_read(file_descriptor_t* f, void* buf, uint32_t bytes);

// Nop
uint32_t keyboard_write(file_descriptor_t* f, const void* buf, uint32_t nbytes);

// Get info
uint32_t keyboard_stat(file_descriptor_t* f, sys_stat_type* data);

// Seek a keyboard (returns error)
uint64_t keyboard_llseek(file_descriptor_t* f, uint64_t offset, int whence);

// Duplicate the file handle
file_descriptor_t* keyboard_duplicate(file_descriptor_t* f);

// Close the keyboard
uint32_t keyboard_close(file_descriptor_t* fd);

#endif
