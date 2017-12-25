#include "ata.h"
#include "pio.h"
#include "dma.h"
#include <common/lib.h>
#include <memory/allocation/heap.h>
#include <drivers/filesystem/filesystem.h>
#include <program/task.h>
#include <syscalls/interrupt.h>

#define ATA_COMMAND_SET_EXT_SUPPORTED		(1 << 26)

#define MBR_PARTITION_TABLE_START			0x01BE
#define PARTITION_INVALID_SYSTEM_ID			0x00

#define ATA_NUM_UDMA_MODES					0x6
#define ATA_NUM_DMA_MODES					0x2

// The four ata drives
ata_drive_t ata_drives[4];
ata_drive_t* ata_previous_drive = NULL;

typedef struct {
	union {
		uint32_t val[4];
		struct {
			uint8_t boot;
			uint8_t starting_head;
			uint32_t starting_sector : 6;
			uint32_t starting_cylinder : 10;
			uint8_t system_id;
			uint8_t ending_head;
			uint32_t ending_sector : 6;
			uint32_t ending_cylinder : 10;
			uint32_t relative_sector;
			uint32_t num_sectors;
		};
	};
} disk_partition_t;

// Initialize the disk
bool ata_init() {
	// Disable interrupts
	outb(ATA_CONTROL_INTERRUPT_DISABLE, ATA_CONTROL_PORT(0));
	outb(ATA_CONTROL_INTERRUPT_DISABLE, ATA_CONTROL_PORT(1));
	
	// Loop through the 4 possible drives
	int i, j;
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			// Select drive
			outb(ATA_DRIVE_SELECT_IDENTIFY(j), ATA_DRIVE_PORT(i));
			
			// Send identify command
			outb(ATA_COMMAND_IDENTIFY, ATA_COMMAND_PORT(i));
			
			// Assume there is no drive
			uint32_t drive_num = i * 2 + j;
			memset(&ata_drives[drive_num], 0, sizeof(ata_drive_t));
			ata_drives[drive_num].bus = i;
			ata_drives[drive_num].drive = j;
			ata_drives[drive_num].present = false;
			
			// There is no drive
			if (inb(ATA_STATUS_PORT(i)) == 0)
				continue;
			
			// Verify this is an ATA device
			bool is_ata = true;
			for (;;) {
				uint8_t status = inb(ATA_STATUS_PORT(i));
				if (status & ATA_STATUS_ERROR) {
					is_ata = false;
					break;
				} else if (!(status & ATA_STATUS_BUSY) && (status & ATA_STATUS_DATA_REQUEST_READY))
					break;
			}
			
			if (!is_ata)
				continue;
			
			// Read the idenitfy data
			uint8_t buffer[128 * 4];
			int z;
			for (z = 0; z < 128; z++) {
				uint32_t val = inl(ATA_DATA_PORT(i));
				memcpy(&buffer[z * 4], &val, sizeof(uint32_t));
			}
						
			// Check for 48-bit LBA
			uint32_t command_sets = 0;
			memcpy(&command_sets, &buffer[ATA_IDENTIFY_COMMAND_SETS], sizeof(uint32_t));
			if (command_sets & ATA_COMMAND_SET_EXT_SUPPORTED) {
				memcpy(&ata_drives[drive_num].num_sectors, &buffer[ATA_IDENTIFY_MAX_LBA_EXT], sizeof(uint32_t));
				ata_drives[drive_num].ext = true;
			}
			else {
				memcpy(&ata_drives[drive_num].num_sectors, &buffer[ATA_IDENTIFY_MAX_LBA], sizeof(uint32_t));
				ata_drives[drive_num].ext = false;
			}
			
			// Convert from big endian to little endian
			ata_drives[drive_num].model[ATA_MODEL_LENGTH] = 0;
			for (z = 0; z < ATA_MODEL_LENGTH; z += 2) {
				ata_drives[drive_num].model[z] = buffer[ATA_IDENTIFY_MODEL + z + 1];
				ata_drives[drive_num].model[z + 1] = buffer[ATA_IDENTIFY_MODEL + z];
			}
			ata_drives[drive_num].present = true;
			
			printf("Detected drive (%d, %d) %d sectors - %s\n", i, j, ata_drives[drive_num].num_sectors
				   , ata_drives[drive_num].model);
		}
	}
		
	// Enable interrupts
	outb(ATA_CONTROL_NONE, ATA_CONTROL_PORT(0));
	outb(ATA_CONTROL_NONE, ATA_CONTROL_PORT(1));
	
	// Select the first drive we detected
	for (i = 0; i < 4; i++) {
		if (ata_drives[i].present) {
			outb(ATA_DRIVE_SELECT(ata_drives[i].drive), ATA_DRIVE_PORT(ata_drives[i].bus));
			ata_previous_drive = &ata_drives[i];
			break;
		}
	}
	
	// If no drives are present, return failure
	if (!ata_previous_drive)
		return false;
	
	// Initialize our drivers
	if (!ata_pio_init())
		return false;
	
	// Check if we can enable DMA
	if (ata_dma_init()) {
		for (i = 0; i < 4; i++)
			ata_drives[i].dma = true;
	}
	
	return true;
}

// Open a handle to the disk
// Filename is of the form disk0 or disk0s1
file_descriptor_t* ata_open(const char* filename, uint32_t mode) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	// Assign this file descriptor
	memset(d, 0, sizeof(file_descriptor_t));
	d->lock = MUTEX_UNLOCKED;
	d->filename = kmalloc(strlen(filename) + 1);
	if (!d->filename)
		return NULL;
	strcpy(d->filename, filename);
	
	d->info = kmalloc(sizeof(disk_info_t));
	if (!d->info) {
		kfree(d->filename);
		return NULL;
	}
	
	// Get the disk and the desired partition
	uint8_t disk = filename[4] - '0', partition = 0;
	if (strlen(filename) == 7)
		partition = filename[6] - '0';
	
	// Get the disk / partition info
	*(disk_info_t*)d->info = ata_open_partition(disk, partition);
	// Check if we had a valid result (the disk and partition exist)
	if (((disk_info_t*)d->info)->bus == (uint8_t)-1) {
		kfree(d->filename);
		kfree(d->info);
		return NULL;
	}
	d->mode = mode | FILE_TYPE_BLOCK;
	
	// Set the function calls
	d->read = ata_read;
	d->write = ata_write;
	d->llseek = ata_llseek;
	d->stat = ata_stat;
	d->duplicate = ata_duplicate;
	d->close = ata_close;
	
	return d;
}

// Read data from the disk
uint32_t ata_read(int32_t fd, void* buf, uint32_t bytes) {
	if (!(descriptors[fd]->mode & FILE_MODE_READ))
		return -EACCES;
	
	return ata_partition_read(descriptors[fd]->info, buf, bytes);
}

// Write data to the disk
uint32_t ata_write(int32_t fd, const void* buf, uint32_t nbytes) {
	if (!(descriptors[fd]->mode & FILE_MODE_WRITE))
		return -EACCES;
	
	if (descriptors[fd]->mode & FILE_MODE_APPEND)
		ata_llseek(fd, uint64_make(0, 0), SEEK_END);
	return ata_partition_write(descriptors[fd]->info, buf, nbytes);
}

// Seek to a disk's position
uint64_t ata_llseek(int32_t fd, uint64_t offset, int whence) {
	return ata_partition_llseek(descriptors[fd]->info, offset, whence);
}

// Get info
uint32_t ata_stat(int32_t fd, sys_stat_type* data) {
	// TODO: fill this in maybe
	return 0;
}

// Duplicate the file handle
file_descriptor_t* ata_duplicate(file_descriptor_t* f) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memcpy(d, f, sizeof(file_descriptor_t));
	
	uint32_t len = strlen(f->filename);
	d->lock = MUTEX_UNLOCKED;
	d->filename = kmalloc(len + 1);
	if (!d->filename) {
		kfree(d);
		return NULL;
	}
	memcpy(d->filename, f->filename, len + 1);
	
	d->info = kmalloc(sizeof(disk_info_t));
	if (!d->info) {
		kfree(d->filename);
		kfree(d);
		return NULL;
	}
	memcpy(d->info, f->info, sizeof(disk_info_t));
	
	return d;
}

// Close a handle to the disk
uint32_t ata_close(file_descriptor_t* fd) {
	// Make this descriptor avaialable
	kfree(fd->filename);
	kfree(fd->info);
	
	// Success
	return 0;
}

// Open a particular partition (partion 0 is the raw disk)
disk_info_t ata_open_partition(uint8_t disk, uint8_t partition) {
	disk_info_t d;
	memset(&d, 0, sizeof(disk_info_t));
	
	// Check the disk exists
	if (!ata_drives[disk].present || partition > 4) {
		d.bus = -1;
		return d;
	}
	
	// Get the info
	d.bus = disk / 2;
	d.drive = disk % 2;
	d.partition = partition;
	d.lock = SPIN_LOCK_UNLOCKED;
	
	// Assume we are using partition 0 (because we need to read from the whole disk)
	d.partition_offset = uint64_make(0, 0);
	d.partition_size = uint64_shl(uint64_make(0, ata_drives[0].num_sectors), NUMBER_OF_SHIFT_BITS_IN_SECTOR);
	
	// Otherwise search for the partition in the MBR partition table
	if (partition != 0) {
		disk_partition_t part;
		ata_partition_llseek(&d, uint64_make(0, MBR_PARTITION_TABLE_START +
											 (partition - 1) * sizeof(disk_partition_t)), SEEK_SET);
		ata_partition_read(&d, &part, sizeof(disk_partition_t));
				
		// Don't open an invalid partition
		if (part.system_id == PARTITION_INVALID_SYSTEM_ID) {
			d.bus = -1;
			return d;
		}
		
		// Set the correct information
		d.seek_offset = uint64_make(0, 0);
		d.partition_offset = uint64_shl(uint64_make(0, part.relative_sector), NUMBER_OF_SHIFT_BITS_IN_SECTOR);
		d.partition_size = uint64_shl(uint64_make(0, part.num_sectors), NUMBER_OF_SHIFT_BITS_IN_SECTOR);
	}

	return d;
}

// Read data from a partition
uint32_t ata_partition_read(disk_info_t* d, void* buf, uint32_t bytes) {
	if (!d)
		return -EFAULT;
	
	// Do nothing if we don't need to
	if (bytes == 0)
		return 0;
			
	// Calculate the correct address
	uint64_t addr = uint64_add(d->partition_offset, d->seek_offset);
	uint64_t sector_addr = uint64_shr(addr, NUMBER_OF_SHIFT_BITS_IN_SECTOR);
	
	// If we will read past the end of the partition, adjust bytes accordingly
	if (uint64_greater(uint64_add(d->seek_offset, uint64_make(0, bytes)), d->partition_size))
		bytes = uint64_sub(d->partition_size, d->seek_offset).low;
	
	// Get the best read function we can
	uint32_t (*read)(uint8_t, uint8_t, uint64_t, void*, uint32_t, uint32_t, uint32_t) = ata_pio_read_blocks;
	if (ata_drives[d->bus * 2 + d->drive].dma)
		read = ata_dma_read_blocks;
	
	uint64_t end_addr = uint64_add(addr, uint64_make(0, bytes));
	uint32_t num_sectors = uint64_sub(uint64_shr(uint64_sub(end_addr, uint64_make(0, 1)),
				NUMBER_OF_SHIFT_BITS_IN_SECTOR), uint64_shr(addr, NUMBER_OF_SHIFT_BITS_IN_SECTOR)).low + 1;
	uint32_t num_blocks = (num_sectors - 1) / (BLOCK_SIZE / ATA_SECTOR_SIZE) + 1;
	int z;							// For looping over the blocks
	uint32_t copy_pos = 0;			// For keeping track where in buf we are writing
	for (z = 0; z < num_blocks; z++) {
		uint32_t size = BLOCK_SIZE;			// Amount of data to be copied this round
		uint32_t b_offset = 0;				// Where to start in this block
		
		// Account for the offset into the first sector
		if (z == 0) {
			b_offset = addr.low % ATA_SECTOR_SIZE;
			size -= b_offset;
		}
		// If we are on the last block, we only need to copy the remaining amount of data
		if (z == num_blocks - 1) {
			size = bytes - copy_pos;
			if (size > BLOCK_SIZE) {
				// Something went wrong
				return -ENOMEM;
			}
		}
		
		// Don't do anything if we don't need to
		if (size == 0)
			break;
		
		// Copy this data over (try to avoid copying twice if we don't have to)
		if (read(d->bus, d->drive, sector_addr, buf + copy_pos, 1, b_offset, size) != BLOCK_SIZE)
			break;
		
		// Increment our positions
		sector_addr = uint64_add(sector_addr, uint64_make(0, BLOCK_SIZE / ATA_SECTOR_SIZE));
		copy_pos += size;
	}
	
	// Update our new seek position
	d->seek_offset = uint64_add(d->seek_offset, uint64_make(0, copy_pos));
	
	// Return the number of bytes copied
	return copy_pos;
}

// Write data to a partition
uint32_t ata_partition_write(disk_info_t* d, const void* buf, uint32_t bytes) {	
	if (!d)
		return -EFAULT;
	
	// Do nothing if we don't need to
	if (bytes == 0)
		return 0;
	
	// Calculate the correct address
	uint64_t addr = uint64_add(d->partition_offset, d->seek_offset);
	uint64_t sector_addr = uint64_shr(addr, NUMBER_OF_SHIFT_BITS_IN_SECTOR);

		
	// If we will write past the end of the partition, adjust bytes accordingly
	if (uint64_greater(uint64_add(d->seek_offset, uint64_make(0, bytes)), d->partition_size))
		bytes = uint64_sub(d->partition_size, d->seek_offset).low;
	
	// Get the best write function we can
	uint32_t (*write)(uint8_t, uint8_t, uint64_t, const void*, uint32_t) = ata_pio_write_blocks;
	uint32_t (*read)(uint8_t, uint8_t, uint64_t, void*, uint32_t, uint32_t, uint32_t) = ata_pio_read_blocks;
	if (ata_drives[d->bus * 2 + d->drive].dma) {
		write = ata_dma_write_blocks;
		read = ata_dma_read_blocks;
	}
	
	// Have an intermediate cache
	void* temp = kmalloc(BLOCK_SIZE);
	if (!temp)
		return -ENOMEM;
	
	uint64_t end_addr = uint64_add(addr, uint64_make(0, bytes));
	uint32_t num_sectors = uint64_sub(uint64_shr(uint64_sub(end_addr, uint64_make(0, 1)),
					NUMBER_OF_SHIFT_BITS_IN_SECTOR), uint64_shr(addr, NUMBER_OF_SHIFT_BITS_IN_SECTOR)).low + 1;
	uint32_t num_blocks = (num_sectors - 1) / (BLOCK_SIZE / ATA_SECTOR_SIZE) + 1;
	int z;							// For looping over the blocks
	uint32_t copy_pos = 0;			// For keeping track where in buf we are writing
	for (z = 0; z < num_blocks; z++) {
		uint32_t size = BLOCK_SIZE;			// Amount of data to be copied this round
		uint32_t b_offset = 0;				// Where to start in this block
		
		// Account for the offset into the first block
		bool needs_read = false;
		if (z == 0) {
			b_offset = addr.low % ATA_SECTOR_SIZE;
			size -= b_offset;
			needs_read = true;
		}
		// If we are on the last block, we only need to copy the remaining amount of data
		if (z == num_blocks - 1) {
			size = bytes - copy_pos;
			if (size > BLOCK_SIZE) {
				// Something went wrong
				kfree(temp);
				return -ENOMEM;
			}
			needs_read = true;
		}
		
		// Don't do anything if we don't need to
		if (size == 0)
			break;
		
		// Read the data into the temporary buffer if we need to
		if (needs_read) {
			if (read(d->bus, d->drive, sector_addr, temp, 1, 0, BLOCK_SIZE) != BLOCK_SIZE)
				break;
			
			// Copy over the data we need to into the buffer then write it
			memcpy(temp + b_offset, buf + copy_pos, size);
			if (write(d->bus, d->drive, sector_addr, temp, 1) != BLOCK_SIZE)
				break;
		} else {
			// Otherwise, just write it directly
			if (write(d->bus, d->drive, sector_addr, buf + copy_pos, 1) != BLOCK_SIZE)
				break;
		}
		
		// Increment our positions
		sector_addr = uint64_add(sector_addr, uint64_make(0, BLOCK_SIZE / ATA_SECTOR_SIZE));
		copy_pos += size;
	}
	
	// Free our intermediate buffer
	kfree(temp);
	
	// Update our new seek position
	d->seek_offset = uint64_add(d->seek_offset, uint64_make(0, copy_pos));
	
	// Return the number of bytes copied
	return copy_pos;
}

// Seek to a position in the partition
uint64_t ata_partition_llseek(disk_info_t* d, uint64_t offset, int whence) {
	// Assign the new position (guaranteed to be one of SEEK_SET, SEEK_CUR, SEEK_END)
	if (whence == SEEK_SET)
		d->seek_offset = offset;
	else if (whence == SEEK_CUR)
		d->seek_offset = uint64_add(d->seek_offset, offset);
	else
		d->seek_offset = uint64_add(d->partition_size, offset);
	
	// Check we still have a valid range
	if (uint64_greater(d->seek_offset, d->partition_size))
		d->seek_offset = d->partition_size;
	
	return d->seek_offset;
}

// Lock the disk
void ata_partition_lock(disk_info_t* d) {
	spin_lock(&d->lock);
}

// Unlock the disk
void ata_partition_unlock(disk_info_t* d) {
	spin_unlock(&d->lock);
}
