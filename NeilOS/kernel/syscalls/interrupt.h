#ifndef INTERRUPT_H
#define INTERRUPT_H

#define NUMBER_OF_INTERRUPTS				20
#define NUMBER_OF_USER_INTERRUPTS			0x10

//loads the interrupt descriptor table
void load_idt();

//handles a requested irq by calling a specific function for the interrupt
void request_irq(unsigned int irq, void (*handler)(int));

#endif	/* INTERRUPT_H */
