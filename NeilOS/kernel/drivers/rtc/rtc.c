#include "rtc.h"
#include <common/lib.h>
#include <syscalls/interrupt.h>
#include <drivers/pic/i8259.h>
#include <drivers/filesystem/filesystem.h>
#include <memory/memory.h>
#include <common/types.h>
#include <program/task.h>

// TODO: make this not perform a task every 1 ms...

#define RTC_IRQ		0x8

#define RTC_COMMAND_PORT	0x70
#define RTC_DATA_PORT		0x71

#define REGISTER_A_CMD		0x8A
#define REGISTER_B_CMD		0x8B
#define REGISTER_C_CMD		0x0C

#define MAX_FREQUENCY	1024
#define MIN_FREQUENCY	2

uint32_t num_rtc_files_open = 0;

typedef struct {
	bool waiting;
	
	uint16_t target_freq;
	uint16_t counter;
} rtc_info_t;

//Function takes in the interrupt vector on the IDT
//It disables other interrupts while it is carrying out current interrupt.
// inputs: interrupt vector
// outputs: none
void rtc_handler(int irq) {
	// Block interrupts
	disable_irq(RTC_IRQ);
	
	// Say we handled the interrupt
	send_eoi(irq);
	
	// Read reigster C to unblock subsequent interrupts
	outb(REGISTER_C_CMD, RTC_COMMAND_PORT);
	inb(RTC_DATA_PORT);
	
	// Critical section
	cli();
	
	enable_irq(RTC_IRQ);
	
	// TODO: this doesn't even really do much and is slow
	
	// Update the descriptors for all tasks
	task_list_t* t = NULL;
	for (t = tasks; t != NULL; t = t->next) {
		if (!t->pcb)
			continue;
		pcb_t* pcb = t->pcb;
		// Only update running tasks
		if (pcb->state != RUNNING)
			continue;
		
		// Update all rtc descrptors
		int z;
		for (z = 0; z < NUMBER_OF_DESCRIPTORS; z++) {
			if (!pcb->descriptors[z])
				continue;
			if (pcb->descriptors[z]->type != RTC_FILE_TYPE)
				continue;
			if (!pcb->descriptors[z]->info)
				continue;
			
			// Update its counter by 1
			rtc_info_t* info = pcb->descriptors[z]->info;
			info->counter++;
			
			// If something has just finished "read" switch to it
			if (info->waiting && (info->counter >= (MAX_FREQUENCY / info->target_freq))) {
				// Auto enables interrupts
				// context_switch(current_pcb, pcb);
				return;
			}
		}
	}
	
	// End critical section
	sti();
}

// Enable or disable interrupts
void rtc_set_interrupts(bool enabled) {
	// Modify bit 6 of Register B to turn on interrupts
	outb(REGISTER_B_CMD, RTC_COMMAND_PORT);
	unsigned char reg_b = inb(RTC_DATA_PORT);
	outb(REGISTER_B_CMD, RTC_COMMAND_PORT);
	if (enabled)
		outb(reg_b | 0x40, RTC_DATA_PORT);
	else
		outb(reg_b & ~0x40, RTC_DATA_PORT);
}

// Initialize the rtc
void rtc_init() {
	// Set the handler
	request_irq(RTC_IRQ, rtc_handler);
	
	// Set the rate
	rtc_set_freq(MAX_FREQUENCY);
	
	// Enable the interrupt
	enable_irq(RTC_IRQ);

	// Disable interrupts if we don't need them
	rtc_set_interrupts(false);
}

//Function initializes the rtc and sets ports
//input: none
//output none
//return: returns new file handle if successfully opened, otherwise -1
file_descriptor_t* rtc_open(const char* filename, uint32_t mode) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	// Assign this file descriptor
	memset(d, 0, sizeof(file_descriptor_t));
	d->lock = MUTEX_UNLOCKED;
	d->filename = "rtc";
	d->info = kmalloc(sizeof(rtc_info_t));
	if (!d->info)
		return d;
	
	*(rtc_info_t*)d->info =
		(rtc_info_t){ .waiting = false, .target_freq = MIN_FREQUENCY, .counter = 0 };
	d->type = RTC_FILE_TYPE;
	d->mode = mode | FILE_TYPE_CHARACTER;
	
	// Set the function calls
	d->read = rtc_read;
	d->write = rtc_write;
	d->stat = rtc_stat;
	d->duplicate = rtc_duplicate;
	d->close = rtc_close;
	
	// Turn on interrupts
	rtc_set_interrupts(true);
	num_rtc_files_open++;
	
	return d;
}

// Set the rate of the RTC
//inputs: pointer to buffer, number of bytes
//output: 4 bytes or 0 if failed
bool rtc_set_freq(uint32_t hz) {
	int z;										// Looping over the 32 bits of the rate
	
	// Don't allow rates that aren't a power of two or that are bigger than 1024
	if (hz > MAX_FREQUENCY || hz < MIN_FREQUENCY)
		return false;
	// We can check it is a power of two if there is only one 1 set in binary
	bool seen1 = false;
	for (z = 0; z < 32; z++) {
		if ((hz >> z) & 0x1) {
			if (seen1)
				return false;
			seen1 = true;
		}
	}
	
	// Find out what the power of two is
	int rate = 1;
	while ((hz = hz / 2) != 1)
		rate++;
	// The rate to the RTC is 32768 >> (rate - 1), so get a hz of 2, we need rate=15
	// and to get a rate of 8192, rate=3
	rate = 16 - rate;
	
	// Disable interrupts
	uint32_t flags = 0;
	cli_and_save(flags);
	
	// Set the rate in Register A
	outb(REGISTER_A_CMD, RTC_COMMAND_PORT);
	unsigned char reg_a = inb(RTC_DATA_PORT);
	outb(REGISTER_A_CMD, RTC_COMMAND_PORT);
	outb((reg_a & 0xF0) | rate, RTC_DATA_PORT);
	
	// Reenable interrupts
	restore_flags(flags);
	
	return true;
}

uint32_t rtc_write(file_descriptor_t* f, const void* buf, uint32_t nbytes) {
	int z;										// Looping over the 32 bits of the rate
	
	// Only accept 4 byte integers
	if (nbytes != sizeof(int32_t))
		return 0;
	
	// Get the user data
	uint32_t hz = 0;
	memcpy(&hz, buf, nbytes);
	
	// Don't allow rates that aren't a power of two or that are bigger than 1024
	if (hz > MAX_FREQUENCY || hz < MIN_FREQUENCY)
		return 0;
	// We can check it is a power of two if there is only one 1 set in binary
	bool seen1 = false;
	for (z = 0; z < 32; z++) {
		if ((hz >> z) & 0x1) {
			if (seen1)
				return 0;
			seen1 = true;
		}
	}
	
	rtc_info_t* info = f->info;
	info->target_freq = hz;
	
	return sizeof(int32_t);
}

//inputs: file descriptor, pointer to buffer, number of bytes
//output: 0 when interrupted
uint32_t rtc_read(file_descriptor_t* f, void* buf, uint32_t bytes) {
	rtc_info_t* info = f->info;
	// Wait for an interrupt to occur
	info->waiting = true;
	info->counter = 0;
	if (!(f->mode & FILE_MODE_NONBLOCKING)) {
		while (info->counter < (MAX_FREQUENCY / info->target_freq)) {
			if (signal_occurring(current_pcb))
				return -EINTR;
			// TODO: maybe not busy wait here?
		}
	}
	info->waiting = false;
	
	return 0;
}

// Get info
uint32_t rtc_stat(file_descriptor_t* f, sys_stat_type* data) {
	data->dev_id = 1;
	data->size = 0;
	data->mode = f->mode;
	return 0;
}

// Duplicate the file handle
file_descriptor_t* rtc_duplicate(file_descriptor_t* f) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memcpy(d, f, sizeof(file_descriptor_t));
	d->lock = MUTEX_UNLOCKED;
	
	d->info = kmalloc(sizeof(rtc_info_t));
	if (!d->info) {
		kfree(d);
		return NULL;
	}
	memcpy(d->info, f->info, sizeof(rtc_info_t));
	
	num_rtc_files_open++;
	
	return d;
}

// Close the rtc
//input: file descriptor
//output: 0 if success
uint32_t rtc_close(file_descriptor_t* fd) {
	// Make this descriptor avaialable
	kfree(fd->info);
	
	// Turn off interrupts if not in use
	if ((--num_rtc_files_open) == 0)
		rtc_set_interrupts(false);
	
	// Success
	return 0;
}
