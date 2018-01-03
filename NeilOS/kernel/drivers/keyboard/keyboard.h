#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <common/types.h>

// Keycodes for this OS
#define ESCAPE_KEY		0x1B
#define ENTER_KEY		'\n'
#define TAB_KEY			0x09
#define BACKSPACE_KEY	0x08
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
#define UP_ARROW_KEY	0x9C
#define DOWN_ARROW_KEY	0x9D
#define LEFT_ARROW_KEY	0x9E
#define RIGHT_ARROW_KEY	0x9F
#define RIGHT_CONTROL_KEY 0xA0
#define NUM_LOCK_KEY	0xA1
#define SCROLL_LOCK_KEY	0xA2

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

#endif
