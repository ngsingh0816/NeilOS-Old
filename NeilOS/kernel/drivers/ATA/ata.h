#ifndef ATA_H
#define ATA_H

#include <common/types.h>
#include <syscalls/descriptor.h>
#include <common/concurrency/semaphore.h>

// Each block is 1024
#define BLOCK_SIZE							1024
#define NUMBER_OF_SHIFT_BITS_IN_SECTOR		9			// 512 = 2^9
#define NUMBER_OF_SHIFT_BITS_IN_BLOCK		10			// 1024 = 2^10

// Sector size in bytes
#define ATA_SECTOR_SIZE				512

// Ports for ATA
#define ATA_DATA_PORT(i)			(0x170 + (0x80 * !i))
#define ATA_FEATURES_PORT(i)		(0x171 + (0x80 * !i))
#define ATA_ERROR_PORT(i)			(0x171 + (0x80 * !i))
#define ATA_SECTOR_COUNT_PORT(i)	(0x172 + (0x80 * !i))
#define ATA_LBA_LOW_PORT(i)			(0x173 + (0x80 * !i))
#define ATA_LBA_MID_PORT(i)			(0x174 + (0x80 * !i))
#define ATA_LBA_HIGH_PORT(i)		(0x175 + (0x80 * !i))
#define ATA_DRIVE_PORT(i)			(0x176 + (0x80 * !i))
#define ATA_COMMAND_PORT(i)			(0x177 + (0x80 * !i))
#define ATA_STATUS_PORT(i)			(0x177 + (0x80 * !i))
#define ATA_CONTROL_PORT(i)			(0x376 + (0x80 * !i))
#define ATA_ALT_STATUS_PORT(i)		(0x376 + (0x80 * !i))

// ATA Status bitmask
#define ATA_STATUS_ERROR					0x01
#define ATA_STATUS_DATA_REQUEST_READY		0x08
#define ATA_STATUS_DRIVE_ERROR				0x20
#define ATA_STATUS_DRIVE_READY				0x40
#define ATA_STATUS_BUSY						0x80

// ATA Control Values
#define ATA_CONTROL_INTERRUPT_DISABLE	0x02
#define ATA_CONTROL_NONE				0x00

// ATA Commands
#define ATA_COMMAND_IDENTIFY				0xEC
#define ATA_COMMAND_SET_FEATURES			0xEF

// Features subcommand
#define ATA_FEATURES_SET_MODE				0x03

// Identify data
#define ATA_IDENTIFY_DEVICE_TYPE		0
#define ATA_IDENTIFY_CYLINDERS			2
#define ATA_IDENTIFY_HEADS				6
#define ATA_IDENTIFY_SECTORS			12
#define ATA_IDENTIFY_SERIAL				20
#define ATA_IDENTIFY_MODEL				54
#define ATA_IDENTIFY_CAPABILITIES		98
#define ATA_IDENTIFY_FIELD_VALID		106
#define ATA_IDENTIFY_MAX_LBA			120
#define ATA_IDENTIFY_DMA_SUPPORTED		126
#define ATA_IDENTIFY_COMMAND_SETS		164
#define ATA_IDENTIFY_UDMA_SUPPORTED		176
#define ATA_IDENTIFY_MAX_LBA_EXT		200

#define ATA_DRIVE_SELECT_IDENTIFY(x)	(0xA0 + (0x10 * x))
#define ATA_DRIVE_SELECT(x)				(0x40 + (0x10 * x))
#define ATA_DRIVE_SELECT28(x)			(0xE0 + (0x10 * x))
#define ATA_MODEL_LENGTH				40
#define ATA_SOFTWARE_RESET				0x2

typedef struct {
	// Which device on the IDE controller
	uint8_t bus;
	uint8_t drive;
	
	// If the device is present
	bool present;
	
	// Supports DMA
	bool dma;
	// Supports 48bit LBA
	bool ext;
	
	char model[ATA_MODEL_LENGTH + 1];
	uint32_t num_sectors;
} ata_drive_t;

// The four possible drives
extern ata_drive_t ata_drives[4];
extern ata_drive_t* ata_previous_drive;			// The last used drive

// Initialize the disk
bool ata_init();

// Open a handle to the disk
file_descriptor_t* ata_open(const char* filename, uint32_t mode);

// Read data from the disk
uint32_t ata_read(int32_t fd, void* buf, uint32_t bytes);
// Write data to the disk
uint32_t ata_write(int32_t fd, const void* buf, uint32_t nbytes);
// Seek to a disk's position
uint64_t ata_llseek(int32_t fd, uint64_t offset, int whence);

// Get info
uint32_t ata_stat(int32_t fd, sys_stat_type* data);

// Duplicate the file handle
file_descriptor_t* ata_duplicate(file_descriptor_t* f);

// Close a handle to the disk
uint32_t ata_close(file_descriptor_t* fd);

// Equivalent's for kernel
typedef struct {
	uint8_t bus;
	uint8_t drive;
	
	uint8_t partition;
	uint64_t partition_size;
	
	uint64_t partition_offset;
	uint64_t seek_offset;
	
	// Disk lock
	spinlock_t lock;
} disk_info_t;

// Open a particular partition on a particular disk  (partion 0 is the raw disk)
// Will return a disk_info_t with bus = -1 if the disk or partition is not valid
disk_info_t ata_open_partition(uint8_t disk, uint8_t partition);

// Read data from a partition
uint32_t ata_partition_read(disk_info_t* d, void* buf, uint32_t bytes);
// Write data to a partition
uint32_t ata_partition_write(disk_info_t* d, const void* buf, uint32_t bytes);
// Seek to a position in the partition
uint64_t ata_partition_llseek(disk_info_t* d, uint64_t offset, int whence);

// Helpers for locking / unlocking the disk
void ata_partition_lock(disk_info_t* d);
void ata_partition_unlock(disk_info_t* d);

#endif
