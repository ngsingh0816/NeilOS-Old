#ifndef PIO_H
#define PIO_H

#include <common/types.h>

// Initialize PIO on the ATA
bool ata_pio_init();

// Read blocks from the specified sector address into the buffer (returns number of bytes read)
// Blocks (size = 4KB) can be from 0 to 8192 (or 64 if only 28bit addressing)
uint32_t ata_pio_read_blocks(uint8_t bus, uint8_t drive, uint64_t address, void* buffer, uint32_t blocks,
							 uint32_t offset, uint32_t length);

// Write blocks to the specified sector address from a buffer (returns bytes written)
// Blocks (size = 4KB) can be from 0 to 8192 (or 64 if only 28bit addressing)
uint32_t ata_pio_write_blocks(uint8_t bus, uint8_t drive, uint64_t address, const void* buffer, uint32_t blocks);

#endif
