//
//  dma.h
//  NeilOS
//
//  Created by Neil Singh on 12/24/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#ifndef DMA_H
#define DMA_H

#include <common/types.h>

// Initialize the ATA for DMA
bool ata_dma_init();

// Read blocks from the specified sector address into a buffer (returns bytes read)
uint32_t ata_dma_read_blocks(uint8_t bus, uint8_t drive, uint64_t address, void* buffer, uint32_t blocks);

// Write blocks to the specified sector address from a buffer (returns bytes written)
uint32_t ata_dma_write_blocks(uint8_t bus, uint8_t drive, uint64_t address, const void* buffer, uint32_t blocks);

#endif
