//
//  dma.c
//  NeilOS
//
//  Created by Neil Singh on 12/24/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#include "dma.h"
#include "ata.h"
#include <syscalls/interrupt.h>
#include <drivers/pic/i8259.h>
#include <common/concurrency/semaphore.h>
#include <drivers/pci/pci.h>
#include <drivers/filesystem/filesystem.h>
#include "pio.h"

// Relative ports for PCI DMA
#define IDE_COMMAND(x)				(0x00 + (0x8 * x))
#define IDE_STATUS(x)				(0x02 + (0x8 * x))
#define	IDE_PRD_TABLE(x)			(0x04 + (0x8 * x))

// IDE Status bitmask
#define IDE_STATUS_IN_USE			0x1
#define IDE_STATUS_ERROR			0x02
#define IDE_STATUS_INTERRUPT		0x04

// Commands for PCI DMA
#define IDE_COMMAND_START_WRITE		0x05
#define IDE_COMMAND_START_READ		0x01
#define IDE_COMMAND_STOP_WRITE		0x04
#define IDE_COMMAND_STOP_READ		0x00
#define IDE_COMMAND_STOP			0x00

// Commands for ATA
#define ATA_COMMAND_READ_DMA_28				0xC8
#define ATA_COMMAND_READ_DMA_48				0x25
#define ATA_COMMAND_WRITE_DMA_28			0xCA
#define ATA_COMMAND_WRITE_DMA_48			0x35

#define PCI_ENABLE_BUSMASTER		0x07

#define DMA_IRQ						0x0E

// Base BAR4 port
uint32_t base_port = 0;

// Buffer for data
uint8_t sector[BLOCK_SIZE] __attribute__((aligned(BLOCK_SIZE)));

// PRDT table
uint32_t prdt[2] __attribute__((aligned(sizeof(uint32_t) * 2)));

// Let's us know whether the data transfer has completed
volatile bool dma_data_ready = false;

// Interrupt handler for DMA's
void ata_dma_handler() {
	// Disable this interrupt
	disable_irq(DMA_IRQ);
	
	// Read the BusMaster status byte
	inb(base_port + IDE_STATUS(ata_previous_drive->bus));
	
	// Clear the interrupt flag in the disk controller and the DMA command
	inb(ATA_STATUS_PORT(ata_previous_drive->bus));
	
	outb(IDE_COMMAND_STOP, base_port + IDE_COMMAND(ata_previous_drive->bus));
	
	// Reenable this interrupt
	send_eoi(DMA_IRQ);
	enable_irq(DMA_IRQ);
	
	// Let the driver know the data transfer is complete
	dma_data_ready = true;
}

pci_device_t ide_device;

// Initialize the ATA for DMA
bool ata_dma_init() {
	ide_device = pci_device_info(IDE_CLASS, IDE_SUBCLASS);
	base_port = pci_interpret_bar(ide_device.bar[4]);
	pci_set_command(&ide_device, PCI_ENABLE_BUSMASTER);
	
	if (ide_device.command != PCI_ENABLE_BUSMASTER)
		return false;
	
	// Save our IRQ
	request_irq(DMA_IRQ, ata_dma_handler);
	enable_irq(DMA_IRQ);
	
	return true;
}

// Read blocks from the specified sector address into a buffer (returns bytes read)
uint32_t ata_dma_read_blocks(uint8_t bus, uint8_t drive, uint64_t address, void* buffer, uint32_t blocks) {
	// Loop through all the sectors
	int z;
	for (z = 0; z < blocks; z++) {
		request_irq(DMA_IRQ, ata_dma_handler);
		
		// Check if we need to rechoose the correct bus
		if (ata_previous_drive->bus != bus || ata_previous_drive->drive != drive) {
			ata_previous_drive = &ata_drives[bus * 2 + drive];
			outb(ATA_DRIVE_SELECT(drive), ATA_DRIVE_PORT(bus));
			
			// Wait 400ns
			int i;
			for (i = 0; i < 4; i++)
				inb(ATA_ALT_STATUS_PORT(bus));
		}

		// Tell it to use one block
		prdt[0] = (uint32_t)sector;
		prdt[1] = (1 << 31) | BLOCK_SIZE;
		
		// Load the info
		outb(IDE_COMMAND_STOP_READ, base_port + IDE_COMMAND(bus));
		outb(IDE_STATUS_ERROR | IDE_STATUS_INTERRUPT, base_port + IDE_STATUS(bus));
		outl(prdt, base_port + IDE_PRD_TABLE(bus));

		// Load the address
		if (ata_previous_drive->ext) {
			// Send high bytes first
			outb(0, ATA_SECTOR_COUNT_PORT(bus));
			outb((address.low >> 24) & 0xFF, ATA_LBA_LOW_PORT(bus));
			outb((address.high >> 0) & 0xFF, ATA_LBA_MID_PORT(bus));
			outb((address.high >> 8) & 0xFF, ATA_LBA_HIGH_PORT(bus));
			// Send low bytes next
			outb(BLOCK_SIZE / ATA_SECTOR_SIZE, ATA_SECTOR_COUNT_PORT(bus));
			outb(address.low & 0xFF, ATA_LBA_LOW_PORT(bus));
			outb((address.low >> 8) & 0xFF, ATA_LBA_MID_PORT(bus));
			outb((address.low >> 16) & 0xFF, ATA_LBA_HIGH_PORT(bus));
			
			// DMA transfer command
			outb(ATA_COMMAND_READ_DMA_48, ATA_COMMAND_PORT(bus));
		} else {
			// Info
			outb(ATA_DRIVE_SELECT28(drive) | ((address.low >> 24) & 0xF), ATA_DRIVE_PORT(bus));
			outb((BLOCK_SIZE / ATA_SECTOR_SIZE) & 0xFF, ATA_SECTOR_COUNT_PORT(bus));
			outb((address.low >> 0) & 0xFF, ATA_LBA_LOW_PORT(bus));
			outb((address.low >> 8) & 0xFF, ATA_LBA_MID_PORT(bus));
			outb((address.low >> 16) & 0xFF, ATA_LBA_HIGH_PORT(bus));
			
			// DMA transfer command
			outb(ATA_COMMAND_READ_DMA_28, ATA_COMMAND_PORT(bus));
		}
		
		// Reset our done flag
		dma_data_ready = false;
	
		// Start Bus Master command
		outb(IDE_COMMAND_START_READ, base_port + IDE_COMMAND(bus));
		
		inb(base_port + IDE_STATUS(bus));
		
		// TODO: figure out why DMA doesn't work on VMWare (need to change BUSMASTER_ENABLE to 0x5)
		// For some reason results in a IDE_STATUS bus error (bit 2)
	
		// Wait for it to be ready
		while (!dma_data_ready)  {}
		
		// Copy over the data
		memcpy(buffer + (z * BLOCK_SIZE), sector, BLOCK_SIZE);
				
		// Get ready for the next address
		address = uint64_inc(address);
	}
	
	return BLOCK_SIZE * blocks;
}

// Write blocks to the specified sector address from a buffer (returns bytes written)
uint32_t ata_dma_write_blocks(uint8_t bus, uint8_t drive, uint64_t address, const void* buffer, uint32_t blocks) {
	// Loop through all the sectors
	int z;
	for (z = 0; z < blocks; z++) {
		request_irq(DMA_IRQ, ata_dma_handler);
		
		// Check if we need to rechoose the correct bus
		if (ata_previous_drive->bus != bus || ata_previous_drive->drive != drive) {
			ata_previous_drive = &ata_drives[bus * 2 + drive];
			outb(ATA_DRIVE_SELECT(drive), ATA_DRIVE_PORT(bus));
			
			// Wait 400ns
			int i;
			for (i = 0; i < 4; i++)
				inb(ATA_STATUS_PORT(bus));
		}
		
		// Tell it to use one block
		prdt[0] = (uint32_t)sector;
		prdt[1] = (1 << 31) | BLOCK_SIZE;
		
		// Load the info
		outl(prdt, base_port + IDE_PRD_TABLE(bus));
		outb(IDE_COMMAND_STOP_WRITE, IDE_COMMAND(bus));
		outb(IDE_STATUS_ERROR | IDE_STATUS_INTERRUPT, base_port + IDE_STATUS(bus));
		
		// Reset our done flag
		dma_data_ready = false;
		
		// Copy over the data
		memcpy(sector, buffer + (z * BLOCK_SIZE), BLOCK_SIZE);
		
		// Load the address
		if (ata_previous_drive->ext) {
			// Send high bytes first
			outb(0, ATA_SECTOR_COUNT_PORT(bus));
			outb((address.low >> 24) & 0xFF, ATA_LBA_LOW_PORT(bus));
			outb((address.high >> 0) & 0xFF, ATA_LBA_MID_PORT(bus));
			outb((address.high >> 8) & 0xFF, ATA_LBA_HIGH_PORT(bus));
			// Send low bytes next
			outb(BLOCK_SIZE / ATA_SECTOR_SIZE, ATA_SECTOR_COUNT_PORT(bus));
			outb(address.low & 0xFF, ATA_LBA_LOW_PORT(bus));
			outb((address.low >> 8) & 0xFF, ATA_LBA_MID_PORT(bus));
			outb((address.low >> 16) & 0xFF, ATA_LBA_HIGH_PORT(bus));
			
			// DMA transfer command
			outb(ATA_COMMAND_WRITE_DMA_48, ATA_COMMAND_PORT(bus));
		} else {
			// Info
			outb(ATA_DRIVE_SELECT28(drive) | ((address.low >> 24) & 0xF), ATA_DRIVE_PORT(bus));
			outb((BLOCK_SIZE / ATA_SECTOR_SIZE) & 0xFF, ATA_SECTOR_COUNT_PORT(bus));
			outb((address.low >> 0) & 0xFF, ATA_LBA_LOW_PORT(bus));
			outb((address.low >> 8) & 0xFF, ATA_LBA_MID_PORT(bus));
			outb((address.low >> 16) & 0xFF, ATA_LBA_HIGH_PORT(bus));
			
			// DMA transfer command
			outb(ATA_COMMAND_WRITE_DMA_28, ATA_COMMAND_PORT(bus));
		}
		
		// Start Bus Master command
		outb(IDE_COMMAND_START_WRITE, base_port + IDE_COMMAND(bus));
		
		// Wait for it to be ready
		while (!dma_data_ready)  {
			// Poll for errors
			uint8_t status = inb(ATA_ALT_STATUS_PORT(bus));
			if ((status & ATA_STATUS_ERROR))
				return BLOCK_SIZE * z;
		}
		
		// Get ready for the next address
		address = uint64_inc(address);
	}
	
	return BLOCK_SIZE * blocks;
}
