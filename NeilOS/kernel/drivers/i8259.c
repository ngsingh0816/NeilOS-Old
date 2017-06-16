/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include <common/lib.h>

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

/* Function Initialize the 8259 PIC, sets up the master and slave and writes to ports needed*/
//inputs: none
//outputs: none
void
i8259_init(void)
{
	// Initialize the two PICs
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW1, SLAVE_8259_PORT);
	
	// Set where the interrupts are (master - 0x20 - 0x27, slave - 0x28-0x30)
	outb(ICW2_MASTER, MASTER_8259_DATA_PORT);
	outb(ICW2_SLAVE, SLAVE_8259_DATA_PORT);
	
	// Tell the master that there is a slave at IRQ2
	// and the slave that it is cascaded at 2
	outb(ICW3_MASTER, MASTER_8259_DATA_PORT);
	outb(ICW3_SLAVE, SLAVE_8259_DATA_PORT);
	
	// Finally, tell the PICs we are using x8086 protocol
	outb(ICW4, MASTER_8259_DATA_PORT);
	outb(ICW4, SLAVE_8259_DATA_PORT);
	
	// Disable all irq's
	master_mask = 0xFB;
	slave_mask = 0xFF;
	outb(master_mask, MASTER_8259_DATA_PORT);
	outb(slave_mask, SLAVE_8259_DATA_PORT);
}

/* Enable (unmask) the specified IRQ */
//inputs: the irq number
//outputs: none
void
enable_irq(uint32_t irq_num)
{
	//if the irq number is 8 or more, it is on the slave
	if (irq_num >= 8) {
		slave_mask &= ~(1 << (irq_num - 8));
		outb(slave_mask, SLAVE_8259_DATA_PORT);
	}
	//otherwise it is on the master 
	else {
		master_mask &= ~(1 << irq_num);
		outb(master_mask, MASTER_8259_DATA_PORT);
	}
}

/* Disable (mask) the specified IRQ */
//inputs: the irq number
//outputs: none
void
disable_irq(uint32_t irq_num)
{
	//if the irq number is 8 or more, it is on the slave
	if (irq_num >= 8) {
		slave_mask |= (1 << (irq_num - 8));
		outb(slave_mask, SLAVE_8259_DATA_PORT);
	} 
	//otherwise it is on the master
	else {
		master_mask |= (1 << irq_num);
		outb(master_mask, MASTER_8259_DATA_PORT);
	}
}

// Returns whether an irq is enabled
bool irq_enabled(uint32_t irq_num) {
	//if the irq number is 8 or more, it is on the slave
	if (irq_num >= 8)
		return !((slave_mask >> (irq_num - 8)) & 0x1);
	else
		return !((master_mask >> irq_num) & 0x1);
}

/* Send end-of-interrupt signal for the specified IRQ */
//inputs: the irq number
//outputs: none
void
send_eoi(uint32_t irq_num)
{
	//if the irq number is 8 or more, it is on the slave
	if (irq_num > 7) {
		outb(EOI | (irq_num - 8), SLAVE_8259_PORT);
		outb(EOI | 0x2, MASTER_8259_PORT);
	}
	//otherwise it is on the master 
	else
		outb(EOI | irq_num, MASTER_8259_PORT);
}

