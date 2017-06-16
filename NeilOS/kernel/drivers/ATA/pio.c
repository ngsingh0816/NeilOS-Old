#include "pio.h"
#include "ata.h"
#include <syscalls/interrupt.h>
#include <drivers/i8259.h>
#include <common/lib.h>
#include <common/concurrency/semaphore.h>
#include <drivers/filesystem/filesystem.h>
#include <drivers/pci.h>

#define PIO_IRQ					0x0E

#define ATA_READ_SECTORS			0x20
#define ATA_WRITE_SECTORS			0x30
#define ATA_READ_SECTORS_EXT		0x24
#define ATA_WRITE_SECTORS_EXT		0x34
#define ATA_CACHE_FLUSH				0xE7
#define ATA_CACHE_FLUSH_EXT			0xEA

#define MAX_BLOCK_READ_COUNT		(1024 * 8)

bool pio_has_error = false;

void ata_pio_handler() {
	uint32_t val = inb(ATA_STATUS_PORT(ata_previous_drive->bus));
	if (val & ATA_STATUS_ERROR) {
		uint8_t error = inb(ATA_ERROR_PORT(ata_previous_drive->bus));
		printf("ATA error - 0x%x\n", error);
		pio_has_error = true;
	}
	
	send_eoi(PIO_IRQ);
}

// Initialize PIO on the ATA
bool ata_pio_init() {
	request_irq(PIO_IRQ, ata_pio_handler);
	enable_irq(PIO_IRQ);
	return true;
}

// Poll for the drive to be ready
uint8_t ata_pio_poll(uint8_t bus) {
	pio_has_error = false;
	
	// Wait 400ns
	int i;
	for (i = 0; i < 4; i++)
		inb(ATA_STATUS_PORT(bus));
	
	// Wait for the drive to be ready
	uint8_t val = inb(ATA_STATUS_PORT(bus));
	while (val & ATA_STATUS_BUSY) {
		val = inb(ATA_STATUS_PORT(bus));
		if ((val & ATA_STATUS_ERROR) || pio_has_error) {
			uint8_t error = inb(ATA_ERROR_PORT(ata_previous_drive->bus));
			printf("ATA error - 0x%x\n", error);
			return val | ATA_STATUS_ERROR;
		}
	}
	
	return val;
}

// Read sectors from the specified sector address into the buffer (returns number of bytes read)
uint32_t ata_pio_read_blocks(uint8_t bus, uint8_t drive, uint64_t address, void* buffer, uint32_t blocks) {
	if (blocks == 0 || blocks > MAX_BLOCK_READ_COUNT)
		return 0;
	
	uint16_t sectors = blocks << 3;
	
	request_irq(PIO_IRQ, ata_pio_handler);
	
	// Wait for drive to be ready
	ata_pio_poll(bus);
	
	// Check if we need to rechoose the correct bus
	if (ata_previous_drive->bus != bus || ata_previous_drive->drive != drive) {
		ata_previous_drive = &ata_drives[bus * 2 + drive];
		outb(ATA_DRIVE_SELECT(drive), ATA_DRIVE_PORT(bus));
		
		// Wait for drive to be ready
		ata_pio_poll(bus);
	}
	
	uint32_t max_sectors = ata_previous_drive->ext ? 0x10000 : 0x100;
	if (ata_previous_drive->ext) {
		// Send the high bytes first
		outb((sectors >> 8) & 0xFF, ATA_SECTOR_COUNT_PORT(bus));
		outb((address.low >> 24) & 0xFF, ATA_LBA_LOW_PORT(bus));
		outb((address.high >> 0) & 0xFF, ATA_LBA_MID_PORT(bus));
		outb((address.high >> 8) & 0xFF, ATA_LBA_HIGH_PORT(bus));

		// Send low bytes next
		outb(sectors & 0xFF, ATA_SECTOR_COUNT_PORT(bus));
		outb((address.low >> 0) & 0xFF, ATA_LBA_LOW_PORT(bus));
		outb((address.low >> 8) & 0xFF, ATA_LBA_MID_PORT(bus));
		outb((address.low >> 16) & 0xFF, ATA_LBA_HIGH_PORT(bus));
		
		// Send the command
		outb(ATA_READ_SECTORS_EXT, ATA_COMMAND_PORT(bus));
	} else {
		// Send the data
		outb(ATA_DRIVE_SELECT28(drive) | ((address.low >> 24) & 0xF), ATA_DRIVE_PORT(bus));
		outb(sectors & 0xFF, ATA_SECTOR_COUNT_PORT(bus));
		outb((address.low >> 0) & 0xFF, ATA_LBA_LOW_PORT(bus));
		outb((address.low >> 8) & 0xFF, ATA_LBA_MID_PORT(bus));
		outb((address.low >> 16) & 0xFF, ATA_LBA_HIGH_PORT(bus));
		
		// Send the command
		outb(ATA_READ_SECTORS, ATA_COMMAND_PORT(bus));
	}
	
	// Read all the sectors
	int z;
	uint32_t real_sectors = (sectors == 0) ? (max_sectors) : sectors;
	for (z = 0; z < real_sectors; z++) {
		// Wait for the data to be ready
		if (ata_pio_poll(bus) & ATA_STATUS_ERROR)
			return z * ATA_SECTOR_SIZE;
				
		// Copy over the data
		int i;
		for (i = 0; i < ATA_SECTOR_SIZE / 2; i++)
			((uint16_t*)buffer)[(z * ATA_SECTOR_SIZE + i * 2) / 2] = inw(ATA_DATA_PORT(bus));
	}
	
	return sectors * ATA_SECTOR_SIZE;
}

// Write blocks to the specified sector address from a buffer (returns bytes written)
// Blocks (size = 4KB) can be from 0 to 8192
uint32_t ata_pio_write_blocks(uint8_t bus, uint8_t drive, uint64_t address, const void* buffer, uint32_t blocks) {
	if (blocks == 0 || blocks > MAX_BLOCK_READ_COUNT)
		return 0;
	
	uint16_t sectors = blocks * (BLOCK_SIZE / ATA_SECTOR_SIZE);
	
	request_irq(PIO_IRQ, ata_pio_handler);
	
	// Wait for drive to be ready
	ata_pio_poll(bus);
	
	// Check if we need to rechoose the correct bus
	if (ata_previous_drive->bus != bus || ata_previous_drive->drive != drive) {
		ata_previous_drive = &ata_drives[bus * 2 + drive];
		if (ata_previous_drive->ext)
			outb(ATA_DRIVE_SELECT(drive), ATA_DRIVE_PORT(bus));
		
		// Wait for drive to be ready
		ata_pio_poll(bus);
	}
	
	uint32_t max_sectors = ata_previous_drive->ext ? 0x10000 : 0x100;
	if (ata_previous_drive->ext) {
		// Send the high bytes first
		outb((sectors >> 8) & 0xFF, ATA_SECTOR_COUNT_PORT(bus));
		outb((address.low >> 24) & 0xFF, ATA_LBA_LOW_PORT(bus));
		outb((address.high >> 0) & 0xFF, ATA_LBA_MID_PORT(bus));
		outb((address.high >> 8) & 0xFF, ATA_LBA_HIGH_PORT(bus));
		
		// Send low bytes next
		outb(sectors & 0xFF, ATA_SECTOR_COUNT_PORT(bus));
		outb((address.low >> 0) & 0xFF, ATA_LBA_LOW_PORT(bus));
		outb((address.low >> 8) & 0xFF, ATA_LBA_MID_PORT(bus));
		outb((address.low >> 16) & 0xFF, ATA_LBA_HIGH_PORT(bus));
		
		// Send the command
		outb(ATA_WRITE_SECTORS_EXT, ATA_COMMAND_PORT(bus));
	} else {
		// Send the data
		outb(ATA_DRIVE_SELECT28(drive) | ((address.low >> 24) & 0xF), ATA_DRIVE_PORT(bus));
		outb(sectors & 0xFF, ATA_SECTOR_COUNT_PORT(bus));
		outb((address.low >> 0) & 0xFF, ATA_LBA_LOW_PORT(bus));
		outb((address.low >> 8) & 0xFF, ATA_LBA_MID_PORT(bus));
		outb((address.low >> 16) & 0xFF, ATA_LBA_HIGH_PORT(bus));
		
		// Send the command
		outb(ATA_WRITE_SECTORS, ATA_COMMAND_PORT(bus));
	}
	
	// Write all the sectors
	pio_has_error = false;
	int z;
	uint32_t real_sectors = (sectors == 0) ? (max_sectors) : sectors;
	for (z = 0; z < real_sectors; z++) {
		// Wait for the data to be ready
		if (ata_pio_poll(bus) & ATA_STATUS_ERROR)
			return z * ATA_SECTOR_SIZE;
		
		// Copy over the data
		int i;
		for (i = 0; i < ATA_SECTOR_SIZE / 2; i++)
			outw(((uint16_t*)buffer)[(z * ATA_SECTOR_SIZE + i * 2) / 2], ATA_DATA_PORT(bus));
	}
	
	// Flush the cache
	if (ata_previous_drive->ext)
		outb(ATA_CACHE_FLUSH_EXT, ATA_COMMAND_PORT(bus));
	else
		outb(ATA_CACHE_FLUSH, ATA_COMMAND_PORT(bus));
	
	// Wait for drive to be ready
	ata_pio_poll(bus);
		
	return sectors * ATA_SECTOR_SIZE;
}
