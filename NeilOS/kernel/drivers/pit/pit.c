#include "pit.h"
#include <drivers/pic/i8259.h>
#include <syscalls/interrupt.h>
#include <common/lib.h>

#define PIT_IRQ						0
#define PIT_DEFAULT_INTERVAL		20
#define PIT_CHANNEL_0_PORT			0x40
#define PIT_COMMAND_PORT			0x43
#define PIT_CHANNEL_0_RATE			0x34
#define PIT_RATE_FREQ				1193182

void (*pit_handler)() = NULL;

// The interrupt handler for a PIT irq
void pit_irq(int irq) {
	send_eoi(irq);
	
	// Call the registered handler if we have one
	if (pit_handler)
		pit_handler();	// In our case, this could perform a context switch
}

// Initialize and setup the Programmable Interval Timer
bool pit_init() {
	// Critical section with respect to this IRQ
	disable_irq(PIT_IRQ);
	
	// Turn on a rate generator for IRQ 0
	outb(PIT_CHANNEL_0_RATE, PIT_COMMAND_PORT);
	
	// Set our default interval of 20ms
	pit_set_interval(PIT_DEFAULT_INTERVAL);
	
	// Register our interrupt handler for PIC interrupt 0 (PIT)
	request_irq(PIT_IRQ, pit_irq);
	
	enable_irq(PIT_IRQ);
	
	return true;
}

// Sets the interval that the interrupts execute
void pit_set_interval(uint16_t ms) {
	// Get the frequency in Hz
	double freq = 1000.0 / ms;
	// Round of a reload rate
	uint16_t reload_value = (PIT_RATE_FREQ / freq) + 0.5;
	
	// Set it
	outb(reload_value & 0xFF, PIT_CHANNEL_0_PORT);
	outb((reload_value >> 8) & 0xFF, PIT_CHANNEL_0_PORT);
}

// Sets the function that gets called when an interrupt executes
void pit_register_handler(void (*func)()) {
	pit_handler = func;
}
