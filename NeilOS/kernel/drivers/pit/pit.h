#ifndef PIT_H
#define PIT_H

#include <common/types.h>

// Initialize and setup the Programmable Interval Timer
bool pit_init();

// Sets the interval that the interrupts execute
void pit_set_interval(uint16_t ms);

// Sets the function that gets called when an interrupt executes
void pit_register_handler(void (*func)());

#endif
