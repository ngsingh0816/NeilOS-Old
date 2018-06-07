//
//  mouse.c
//  NeilOS
//
//  Created by Neil Singh on 1/2/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "mouse.h"
#include <syscalls/interrupt.h>
#include <drivers/pic/i8259.h>
#include <drivers/keyboard/keyboard.h>
#include <drivers/graphics/graphics.h>

#define COMMAND_PORT			0x64
#define DATA_PORT				0x60
#define COMMAND_GET_STATUS		0x20
#define COMMAND_SET_STATUS		0x60

#define DATA_BIT				0x1
#define MOUSE_BIT				(1 << 5)

#define COMMAND_WRITE			0xD4
#define MOUSE_DEFAULT_SETTINGS	0xF6
#define MOUSE_ENABLE			0xF4
#define MOUSE_GET_ID			0xF2
#define MOUSE_SET_SAMPLE_RATE	0xF3

#define ENABLE_IRQ12			0x2
#define ENABLE_MOUSE_AUX		0xA8

#define DATA_TYPE				0
#define SIGNAL_TYPE				1

#define MOUSE_IRQ				12

#define MOUSE_PACKET_SIZE		4

#define OVERFLOW_Y(x)				((x >> 7) & 0x1)
#define OVERFLOW_X(x)				((x >> 6) & 0x1)
#define Y_SIGN_BIT(x)				((x >> 5) & 0x1)
#define X_SIGN_BIT(x)				((x >> 4) & 0x1)
#define MIDDLE_BUTTON_BIT(x)		((x >> 2) & 0x1)
#define RIGHT_BUTTON_BIT(x)			((x >> 1) & 0x1)
#define LEFT_BUTTON_BIT(x)			((x >> 0) & 0x1)

#define SCROLL_NONE					0x0
#define SCROLL_UP					0x1
#define SCROLL_DOWN					0xF
#define SCROLL_LEFT					0xE
#define SCROLL_RIGHT				0x2

#define MOUSE_BYTE_STATUS			0x0
#define MOUSE_BYTE_X_MOVEMENT		0x1
#define MOUSE_BYTE_Y_MOVEMENT		0x2
#define MOUSE_BYTE_SCROLL			0x3

#define NEG_VALUE					0xFFFFFF00

unsigned char mouse_bytes[MOUSE_PACKET_SIZE];
int mouse_index = 0;

int mouse_x = 0, mouse_y = 0;
int scroll_dx = 0, scroll_dy = 0;
int left_down = 0, right_down = 0, middle_down = 0;
mutex_t mouse_lock = MUTEX_UNLOCKED;
volatile unsigned int mouse_packet_id = 0;

typedef struct {
	uint32_t id;
	bool is_waiting;
	bool pass;
} mouse_info_t;

void mouse_handle() {
	mouse_bytes[mouse_index++] = inb(DATA_PORT);
	if (mouse_index == MOUSE_PACKET_SIZE) {
		mouse_index = 0;
		if (!(OVERFLOW_Y(mouse_bytes[MOUSE_BYTE_STATUS]) || OVERFLOW_X(mouse_bytes[MOUSE_BYTE_STATUS]))) {
			int prev_x = mouse_x, prev_y = mouse_y;
			int prev_left = left_down, prev_right = right_down, prev_middle = middle_down;
			
			if (X_SIGN_BIT(mouse_bytes[MOUSE_BYTE_STATUS]))
				mouse_x += mouse_bytes[MOUSE_BYTE_X_MOVEMENT] | NEG_VALUE;
			else
				mouse_x += mouse_bytes[MOUSE_BYTE_X_MOVEMENT];
			if (Y_SIGN_BIT(mouse_bytes[MOUSE_BYTE_STATUS]))
				mouse_y -= mouse_bytes[MOUSE_BYTE_Y_MOVEMENT] | NEG_VALUE;
			else
				mouse_y -= mouse_bytes[MOUSE_BYTE_Y_MOVEMENT];
			middle_down = MIDDLE_BUTTON_BIT(mouse_bytes[MOUSE_BYTE_STATUS]);
			left_down = LEFT_BUTTON_BIT(mouse_bytes[MOUSE_BYTE_STATUS]);
			right_down = RIGHT_BUTTON_BIT(mouse_bytes[MOUSE_BYTE_STATUS]);
			scroll_dx = 0;
			scroll_dy = 0;
			switch ((mouse_bytes[MOUSE_BYTE_SCROLL] & 0xF)) {
				case SCROLL_UP:
					scroll_dy++;
					break;
				case SCROLL_DOWN:
					scroll_dy--;
					break;
				case SCROLL_LEFT:
					scroll_dx--;
					break;
				case SCROLL_RIGHT:
					scroll_dx++;
					break;
				default:
					break;
			}
			
			// Don't let the mouse leave the screen
			graphics_info_t info;
			graphics_info_get(&info);
			if (mouse_x < 0)
				mouse_x = 0;
			if (mouse_x > info.resolution_x)
				mouse_x = info.resolution_x;
			if (mouse_y < 0)
				mouse_y = 0;
			if (mouse_y > info.resolution_y)
				mouse_y = info.resolution_y;
			
			if (prev_x != mouse_x || prev_y != mouse_y ||
				prev_left != left_down || prev_right != right_down || prev_middle != middle_down ||
				scroll_dx != 0 || scroll_dy != 0) {
				// Update
				mouse_packet_id++;
				
				// Tell the graphics to update the cursor position
				if (graphics_enabled())
					graphics_cursor_position_set(mouse_x, mouse_y, true);
			}
		}
	}
}

void mouse_handler(int irq) {
	// Disable this interrupt
	disable_irq(MOUSE_IRQ);
	
	int status = inb(COMMAND_PORT);
	while (status & DATA_BIT) {
		if (status & MOUSE_BIT)
			mouse_handle();
		else
			keyboard_handle();
		status = inb(COMMAND_PORT);
	}
	
	// Say the we've handled this interrupt
	send_eoi(irq);
	enable_irq(MOUSE_IRQ);
}

void mouse_wait(int type) {
	if (type == DATA_TYPE) {
		while ((inb(COMMAND_PORT) & 0x1) != 1) {}
	} else {
		while ((inb(COMMAND_PORT) & 0x2) != 0) {}
	}
}

void mouse_writeb(uint8_t data) {
	mouse_wait(SIGNAL_TYPE);
	outb(COMMAND_WRITE, COMMAND_PORT);
	mouse_wait(SIGNAL_TYPE);
	outb(data, DATA_PORT);
}

uint8_t mouse_readb() {
	mouse_wait(DATA_TYPE);
	return inb(DATA_PORT);
}

void mouse_set_sample_rate(uint8_t rate) {
	mouse_writeb(MOUSE_SET_SAMPLE_RATE);
	mouse_readb();
	mouse_writeb(rate);
	mouse_readb();
}

void mouse_init() {
	// Enable Auxiliary Mouse Device
	mouse_wait(SIGNAL_TYPE);
	outb(ENABLE_MOUSE_AUX, COMMAND_PORT);
	
	// Enable interrupts
	mouse_wait(SIGNAL_TYPE);
	outb(COMMAND_GET_STATUS, COMMAND_PORT);
	mouse_wait(DATA_TYPE);
	uint8_t status = inb(DATA_PORT) | ENABLE_IRQ12;
	mouse_wait(SIGNAL_TYPE);
	outb(COMMAND_SET_STATUS, COMMAND_PORT);
	mouse_wait(SIGNAL_TYPE);
	outb(status, DATA_PORT);
	
	// Use default settings
	mouse_writeb(MOUSE_DEFAULT_SETTINGS);
	mouse_readb();
	
	// Enable scroll wheels
	mouse_set_sample_rate(200);
	mouse_set_sample_rate(100);
	mouse_set_sample_rate(80);
	mouse_writeb(MOUSE_GET_ID);
	mouse_readb();
	mouse_readb();
	
	// Enable mouse
	mouse_writeb(MOUSE_ENABLE);
	mouse_readb();
	
	request_irq(MOUSE_IRQ, mouse_handler);
	enable_irq(MOUSE_IRQ);
}

// Initialize the mouse
file_descriptor_t* mouse_open(const char* filename, uint32_t mode) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memset(d, 0, sizeof(file_descriptor_t));
	// Mark in use
	d->lock = MUTEX_UNLOCKED;
	d->type = MOUSE_FILE_TYPE;
	d->mode = FILE_TYPE_CHARACTER | mode;
	int namelen = strlen(filename);
	d->filename = kmalloc(namelen + 1);
	if (!d->filename) {
		kfree(d);
		return NULL;
	}
	memcpy(d->filename, filename, namelen + 1);
	d->info = kmalloc(sizeof(mouse_info_t));
	if (!d->info) {
		kfree(d->filename);
		kfree(d);
		return NULL;
	}
	memset(d->info, 0, sizeof(mouse_info_t));
	
	// Assign the functions
	d->read = mouse_read;
	d->write = mouse_write;
	d->stat = mouse_stat;
	d->can_read = mouse_can_read;
	d->can_write = mouse_can_write;
	d->llseek = mouse_llseek;
	d->duplicate = mouse_duplicate;
	d->close = mouse_close;
	
	return d;
}

/* Mouse Packet is of the format (9 bytes)
{
 	int mouse_x,
 	int mouse_y,
 	0|scroll_dy(2)|scroll_dx(2)|middle|right|left
}
*/

#define MOUSE_STRUCT_SIZE		9

// Returns the last mouse position if in nonblocking mode, otherwise blocks until mouse is moved
uint32_t mouse_read(file_descriptor_t* f, void* buf, uint32_t bytes) {
	if (bytes != MOUSE_STRUCT_SIZE)
		return -EINVAL;
	
	mouse_info_t* info = (mouse_info_t*)f->info;
	if (!(f->mode & FILE_MODE_NONBLOCKING) && !info->pass) {
		unsigned int id = mouse_packet_id;
		pcb_t* pcb = current_pcb;
		while (id == mouse_packet_id && !(pcb && pcb->should_terminate) && !f->closed) {
			if (signal_occurring(pcb))
				return -EINTR;
			up(&f->lock);
			schedule();
			down(&f->lock);
		}
	}
	info->pass = false;
	
	((int*)buf)[0] = mouse_x;
	((int*)buf)[1] = mouse_y;
	((char*)buf)[8] = left_down | (right_down << 1) | (middle_down << 2) | ((scroll_dx & 0x3) << 3) | ((scroll_dy & 0x3) << 5);
	
	return MOUSE_STRUCT_SIZE;
}

// Sets the cursor position
uint32_t mouse_write(file_descriptor_t* f, const void* buf, uint32_t nbytes) {
	if (nbytes != MOUSE_STRUCT_SIZE)
		return -EINVAL;
	
	down(&mouse_lock);
	mouse_x = ((int*)buf)[0];
	mouse_y = ((int*)buf)[1];
	char val = ((char*)buf)[8];
	left_down = val & 0x1;
	right_down = (val >> 1) & 0x1;
	middle_down = (val >> 2) & 0x1;
	scroll_dx = (val >> 3) & 0x3;
	scroll_dy = (val >> 5) & 0x3;
	up(&mouse_lock);
	
	// Tell the graphics to update the cursor position
	if (graphics_enabled())
		graphics_cursor_position_set(mouse_x, mouse_y, true);
	
	return MOUSE_STRUCT_SIZE;
}

// Can read
bool mouse_can_read(file_descriptor_t* f) {
	mouse_info_t* info = (mouse_info_t*)f->info;
	if (info->pass)
		return true;
	if (info->is_waiting) {
		bool ret = (info->id != mouse_packet_id);
		if (ret) {
			info->is_waiting = false;
			info->pass = true;
		}
		return ret;
	}
	info->is_waiting = true;
	info->id = mouse_packet_id;
	return false;
}

// Can write
bool mouse_can_write(file_descriptor_t* f) {
	return true;
}

// Get info
uint32_t mouse_stat(file_descriptor_t* f, sys_stat_type* data) {
	data->dev_id = f->type;
	data->size = MOUSE_PACKET_SIZE;
	data->mode = f->mode;
	return 0;
}

// Seek a mouse (returns error)
uint64_t mouse_llseek(file_descriptor_t* f, uint64_t offset, int whence) {
	return uint64_make(-1, -ESPIPE);
}

// Duplicate the file handle
file_descriptor_t* mouse_duplicate(file_descriptor_t* f) {
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
	d->info = kmalloc(sizeof(mouse_info_t));
	if (!d->info) {
		kfree(d->filename);
		kfree(d);
		return NULL;
	}
	memcpy(d->info, f->info, sizeof(mouse_info_t));
	d->lock = MUTEX_UNLOCKED;
	
	return d;
}

// Close the mouse
uint32_t mouse_close(file_descriptor_t* fd) {
	kfree(fd->filename);
	kfree(fd->info);
	return 0;
}
