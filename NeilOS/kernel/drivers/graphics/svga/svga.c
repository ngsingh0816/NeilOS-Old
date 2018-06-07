//
//  svga.c
//  product
//
//  Created by Neil Singh on 1/22/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "svga.h"
#include "include/svga_reg.h"
#include "svga_3d.h"
#include <drivers/pci/pci.h>
#include <common/lib.h>
#include <memory/memory.h>
#include <common/concurrency/semaphore.h>
#include <program/task.h>

#define SVGA_PCI_ENABLE_BUSMASTER		0x07

#define DEFAULT_RESOLUTION_WIDTH		1280
#define DEFAULT_RESOLUTION_HEIGHT		720
// Must be 32 for QEMU
#define DEFAULT_BPP						32

// Settings
uint32_t res_width = DEFAULT_RESOLUTION_WIDTH;
uint32_t res_height = DEFAULT_RESOLUTION_HEIGHT;
uint32_t bpp = DEFAULT_BPP;

// Device Regs
uint32_t io_base = 0;
uint8_t* fb_base = NULL;
uint32_t* fifo_base = NULL;
uint32_t vram_size = 0;
uint32_t fb_size = 0;
uint32_t fifo_size = 0;
uint32_t capabilities = 0;
uint32_t pitch = 0;
bool enabled = false;

// FIFO
semaphore_t fifo_lock = MUTEX_UNLOCKED;
uint32_t fifo_reserved_size = 0;
uint32_t next_fence = 0;
uint8_t* bounce_buffer = NULL;
void* svga_fifo_reserve(uint32_t bytes);
void* svga_fifo_reserve_cmd(uint32_t type, uint32_t bytes);
void* svga_fifo_reserve_escape(uint32_t nsid, uint32_t bytes);
void svga_fifo_commit(uint32_t bytes);
void svga_fifo_commit_all();

static inline void svga_write(uint32_t reg, uint32_t data) {
	outl(reg, io_base + SVGA_INDEX_PORT);
	outl(data, io_base + SVGA_VALUE_PORT);
}

static inline uint32_t svga_read(uint32_t reg) {
	outl(reg, io_base + SVGA_INDEX_PORT);
	return inl(io_base + SVGA_VALUE_PORT);
}

// Initializes the VMWare SVGA-II device and enables it
bool svga_init() {
	pci_device_t dev = pci_device_info_vendor(PCI_VENDOR_ID_VMWARE, PCI_DEVICE_ID_VMWARE_SVGA2);
	if (dev.bus == (uint8_t)-1)
		return false;
	pci_set_command(&dev, SVGA_PCI_ENABLE_BUSMASTER);
	io_base = pci_interpret_bar(dev.bar[0]);
	fb_base = (uint8_t*)pci_interpret_bar(dev.bar[1]);
	fifo_base = (uint32_t*)pci_interpret_bar(dev.bar[2]);
	
	// Set the max supported version
	uint32_t version = SVGA_ID_2;
	svga_write(SVGA_REG_ID, version);
	if (svga_read(SVGA_REG_ID) != version)
		return false;
	
	// Get some info
	vram_size = svga_read(SVGA_REG_VRAM_SIZE);
	fb_size = svga_read(SVGA_REG_FB_SIZE);
	fifo_size = svga_read(SVGA_REG_MEM_SIZE);
	capabilities = svga_read(SVGA_REG_CAPABILITIES);
	
	// Map in the virtual memory for the memory mapped I/O
	// this will not get unmapped so we don't have to worry about it (because they are kernel pages)
	// TODO: maybe map these in on demand?
	vm_lock();
	uint32_t vaddr = vm_get_next_unmapped_pages((fb_size - 1) / FOUR_MB_SIZE + 1, VIRTUAL_MEMORY_KERNEL);
	uint32_t paddr = (uint32_t)fb_base - ((uint32_t)fb_base % FOUR_MB_SIZE);
	for (uint32_t z = 0; z < fb_size; z += FOUR_MB_SIZE)
		vm_map_page(vaddr + z, paddr + z, MEMORY_RW | MEMORY_KERNEL, false);
	fb_base = (uint8_t*)((uint32_t)fb_base - paddr + vaddr);
	vaddr = vm_get_next_unmapped_pages((fifo_size - 1) / FOUR_MB_SIZE + 1, VIRTUAL_MEMORY_KERNEL);
	paddr = (uint32_t)fifo_base - ((uint32_t)fifo_base % FOUR_MB_SIZE);
	for (uint32_t z = 0; z < fifo_size; z += FOUR_MB_SIZE)
		vm_map_page(vaddr + z, paddr + z, MEMORY_RW | MEMORY_KERNEL, false);
	fifo_base = (uint32_t*)((uint32_t)fifo_base - paddr + vaddr);
	vm_unlock();
	
	/*printf("VRAM size: 0x%x\n", vram_size);
	printf("FB size: 0x%x\n", fb_size);
	printf("FIFO size: 0x%x\n", fifo_size);
	printf("Capabilities: 0x%x\n", capabilities);
	
	if (capabilities & SVGA_CAP_GMR2) {
		printf("Max GMR IDs: 0x%x\n", svga_read(SVGA_REG_GMR_MAX_IDS));
		printf("Max GMR Pages: 0x%x\n", svga_read(SVGA_REG_GMRS_MAX_PAGES));
		printf("Memory Size: 0x%x\n", svga_read(SVGA_REG_MEMORY_SIZE));
	}
	 
	if (capabilities & SVGA_CAP_GBOBJECTS) {
		printf("GB Mem Size: 0x%x\n", svga_read(SVGA_REG_SUGGESTED_GBOBJECT_MEM_SIZE_KB));
		if (!(capabilities & SVGA_CAP_3D))
			printf("3D Not Supported???\n");
		printf("Primary BB Mem: 0x%x\n", svga_read(SVGA_REG_MAX_PRIMARY_BOUNDING_BOX_MEM));
		printf("Max Mob Size: 0x%x\n", svga_read(SVGA_REG_MOB_MAX_SIZE));
		printf("Screen Target Width: 0x%x\n", svga_read(SVGA_REG_SCREENTARGET_MAX_WIDTH));
		printf("Screen Target Height: 0x%x\n", svga_read(SVGA_REG_SCREENTARGET_MAX_HEIGHT));
		svga_write(SVGA_REG_DEV_CAP, SVGA3D_DEVCAP_MAX_TEXTURE_WIDTH);
		printf("Max Texture Width: 0x%x\n", svga_read(SVGA_REG_DEV_CAP));
		svga_write(SVGA_REG_DEV_CAP, SVGA3D_DEVCAP_MAX_TEXTURE_HEIGHT);
		printf("Max Texture Height: 0x%x\n", svga_read(SVGA_REG_DEV_CAP));
		svga_write(SVGA_REG_DEV_CAP, SVGA3D_DEVCAP_3D);
		printf("Has 3D: 0x%x\n", svga_read(SVGA_REG_DEV_CAP));
		svga_write(SVGA_REG_DEV_CAP, SVGA3D_DEVCAP_DX);
		printf("Has DX: 0x%x\n", svga_read(SVGA_REG_DEV_CAP));
	}*/
	
	// Init FIFO
	svga_write(SVGA_REG_ENABLE, SVGA_REG_ENABLE_ENABLE | SVGA_REG_ENABLE_HIDE);
	svga_write(SVGA_REG_TRACES, false);
	
	uint32_t min = SVGA_FIFO_NUM_REGS * sizeof(uint32_t);
	fifo_base[SVGA_FIFO_MIN] = min;
	fifo_base[SVGA_FIFO_MAX] = fifo_size;
	fifo_base[SVGA_FIFO_NEXT_CMD] = min;
	fifo_base[SVGA_FIFO_STOP] = min;
	fifo_base[SVGA_FIFO_BUSY] = 0;
	svga_write(SVGA_REG_CONFIG_DONE, true);
	fifo_base[SVGA_FIFO_FENCE] = 0;
	
	return svga_enable();
}

// Enables the device
bool svga_enable() {
	// Enable FIFO and device
	svga_write(SVGA_REG_ENABLE, SVGA_REG_ENABLE_ENABLE);
	enabled = true;
	
	if (fifo_base[SVGA_FIFO_3D_HWVERSION] == 0) {
		svga_write(SVGA_REG_ENABLE, SVGA_REG_ENABLE_DISABLE);
		enabled = false;
		
		return false;
	}
	return true;
}

// Disables the device (returns to text printing)
void svga_disable() {
	svga_write(SVGA_REG_ENABLE, SVGA_REG_ENABLE_DISABLE);
	enabled = false;
}

bool svga_enabled() {
	return enabled;
}

// Pointer to the framebuffer
uint8_t* svga_framebuffer() {
	return &fb_base[svga_read(SVGA_REG_FB_OFFSET)];
}

uint32_t svga_framebuffer_size() {
	return fb_size;
}

// Set the resolution and bpp
void svga_set_mode(uint32_t width, uint32_t height, uint32_t bits) {
	res_width = width;
	res_height = height;
	bpp = bits;
	
	svga_write(SVGA_REG_WIDTH, width);
	svga_write(SVGA_REG_HEIGHT, height);
	svga_write(SVGA_REG_BITS_PER_PIXEL, bpp);
	svga_write(SVGA_REG_ENABLE, true);
	pitch = svga_read(SVGA_REG_BITS_PER_PIXEL);
}

uint32_t svga_resolution_get_x() {
	return res_width;
}

uint32_t svga_resolution_get_y() {
	return res_height;
}

uint32_t svga_bpp_get() {
	return bpp;
}

// Reserve space for fifo commands
void* svga_fifo_reserve(uint32_t bytes) {
	// Unlocked in fifo_commit
	down(&fifo_lock);
	uint32_t max = fifo_base[SVGA_FIFO_MAX];
	uint32_t min = fifo_base[SVGA_FIFO_MIN];
	uint32_t next_cmd = fifo_base[SVGA_FIFO_NEXT_CMD];
	
	fifo_reserved_size = bytes;
	for (;;) {
		uint32_t stop = fifo_base[SVGA_FIFO_STOP];
		bool success = false;
		if (next_cmd >= stop) {
			if ((next_cmd + bytes < max) || ((next_cmd + bytes == max) && stop > min)) {
				// There is space in the FIFO
				success = true;
			} else if (!((max - next_cmd) + (stop - min) <= bytes)) {
				// Need bounce buffer
				bounce_buffer = kmalloc(bytes);
				return bounce_buffer;
			}
		} else if (next_cmd + bytes < stop) {
			// There is space in the FIFO
			success = true;
		}
		
		if (success) {
			fifo_base[SVGA_FIFO_RESERVED] = bytes;
			return (void*)(next_cmd + (uint8_t*)fifo_base);
		} else {
			// Sync the FIFO and keep trying
			svga_write(SVGA_REG_SYNC, 1);
			svga_read(SVGA_REG_BUSY);
		}
	}
	
	return NULL;
}

// Reserve space for a fifo command of the given type
void* svga_fifo_reserve_cmd(uint32_t type, uint32_t bytes) {
	uint32_t* cmd = svga_fifo_reserve(sizeof(type) + bytes);
	cmd[0] = type;
	return (void*)(cmd + 1);
}

// Reserve space for an escape fifo command
void* svga_fifo_reserve_escape(uint32_t nsid, uint32_t bytes) {
	uint32_t padd = (bytes + 3) & ~3;
	struct svga_fifo_escape {
		uint32_t cmd;
		uint32_t nsid;
		uint32_t size;
	} __attribute__ ((packed));
	struct svga_fifo_escape* header = svga_fifo_reserve(padd + sizeof(struct svga_fifo_escape));
	header->cmd = SVGA_CMD_ESCAPE;
	header->nsid = nsid;
	header->size = bytes;
	return (void*)(header + 1);
}

// Commit the specified number of fifo commands
void svga_fifo_commit(uint32_t bytes) {
	uint32_t max = fifo_base[SVGA_FIFO_MAX];
	uint32_t min = fifo_base[SVGA_FIFO_MIN];
	uint32_t next_cmd = fifo_base[SVGA_FIFO_NEXT_CMD];
	
	if (bounce_buffer) {
		uint32_t size = (bytes < (max - next_cmd)) ? bytes : (max - next_cmd);
		fifo_base[SVGA_FIFO_RESERVED] = bytes;
		memcpy(next_cmd + (uint8_t*)fifo_base, bounce_buffer, size);
		memcpy(min + (uint8_t*)fifo_base, &bounce_buffer[size], bytes - size);
		kfree(bounce_buffer);
		bounce_buffer = NULL;
	}
	fifo_reserved_size = 0;
	next_cmd += bytes;
	if (next_cmd >= max)
		next_cmd -= max - min;
	fifo_base[SVGA_FIFO_NEXT_CMD] = next_cmd;
	fifo_base[SVGA_FIFO_RESERVED] = 0;
	
	// Locked in fifo reserve
	up(&fifo_lock);
}

// Commit all fifo commands
void svga_fifo_commit_all() {
	svga_fifo_commit(fifo_reserved_size);
}

// Update a region of the screen
void svga_update(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	SVGAFifoCmdUpdate* cmd = svga_fifo_reserve_cmd(SVGA_CMD_UPDATE, sizeof(SVGAFifoCmdUpdate));
	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	svga_fifo_commit_all();
}

// Set the cursor image (premultiplied 32-bit BGRA image)
void svga_set_cursor_image(uint32_t hotspot_x, uint32_t hotspot_y, uint32_t width, uint32_t height, uint32_t* data) {
	if ((svga_read(SVGA_REG_CAPABILITIES) & SVGA_CAP_ALPHA_CURSOR)) {
		uint32_t size = width * height * sizeof(uint32_t);
		SVGAFifoCmdDefineAlphaCursor* cursor = svga_fifo_reserve_cmd(SVGA_CMD_DEFINE_ALPHA_CURSOR,
																	 sizeof(SVGAFifoCmdDefineAlphaCursor) + size);
		cursor->id = 0;
		cursor->hotspotX = hotspot_x;
		cursor->hotspotY = hotspot_y;
		cursor->width = width;
		cursor->height = height;
		memcpy((void*)(cursor + 1), data, size);
		svga_fifo_commit_all();
	} else {
		// Fall back to more complicated implementation
		uint32_t bits = 1;
		uint32_t alpha_row = ((width + 31) >> 5) << 2;
		uint32_t pix_row = ((width * bits + 31) >> 5) << 2;
		
		SVGAFifoCmdDefineCursor* cursor = svga_fifo_reserve_cmd(SVGA_CMD_DEFINE_CURSOR,
										sizeof(SVGAFifoCmdDefineCursor) + (alpha_row + pix_row) * height);
		cursor->id = 0;
		cursor->hotspotX = hotspot_x;
		cursor->hotspotY = hotspot_y;
		cursor->width = width;
		cursor->height = height;
		cursor->andMaskDepth = bits;
		cursor->xorMaskDepth = bits ;
		// Extract alpha bits in a mask
		uint32_t* alpha_mask = (uint32_t*)(cursor + 1);
		memset(alpha_mask, 0x00, alpha_row * height);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				uint32_t pos = y * width + x;
				uint32_t alpha = ((data[pos] >> 24) & 0xFF) < 0x7F;
				alpha_mask[pos / 32] |= (alpha << (31 - (pos % 32)));
			}
		}
		// Copy image data
		uint32_t* color = (uint32_t*)((uint8_t*)alpha_mask + alpha_row * height);
		memcpy(color, data, width * height * sizeof(uint32_t));
		svga_fifo_commit_all();
	}
}

// Set cursor position
void svga_set_cursor_position(uint32_t x, uint32_t y, bool visible) {
	fifo_base[SVGA_FIFO_CURSOR_X] = x;
	fifo_base[SVGA_FIFO_CURSOR_Y] = y;
	fifo_base[SVGA_FIFO_CURSOR_ON] = visible;
	fifo_base[SVGA_FIFO_CURSOR_COUNT]++;
}

// Insert a fence into the FIFO
uint32_t svga_fence_insert() {
	if (!(fifo_base[SVGA_FIFO_CAPABILITIES] & SVGA_FIFO_CAP_FENCE))
		return 1;
	
	struct svga_fence_cmd {
		uint32_t id;
		uint32_t fence;
	} __attribute__ ((packed));
	struct svga_fence_cmd* cmd;
	
	if (next_fence == 0 || next_fence >= 0x7FFFFFFF)
		next_fence = 1;
	uint32_t fence = next_fence++;
	
	cmd = svga_fifo_reserve(sizeof(struct svga_fence_cmd));
	cmd->id = SVGA_CMD_FENCE;
	cmd->fence = fence;
	svga_fifo_commit_all();
	
	return fence;
}

// Wait until this fence has been processed
void svga_fence_sync(uint32_t fence) {
	if (!(fifo_base[SVGA_FIFO_CAPABILITIES] & SVGA_FIFO_CAP_FENCE)) {
		svga_write(SVGA_REG_SYNC, 1);
		while (svga_read(SVGA_REG_BUSY))
			schedule();
		return;
	}
	
	bool busy = true;
	svga_write(SVGA_REG_SYNC, 1);
	while (!svga_fence_passed(fence) && busy) {
		schedule();
		busy = (svga_read(SVGA_REG_BUSY) != 0);
	}
}

// Returns true if this fence has already been processed
bool svga_fence_passed(uint32_t fence) {
	if (!fence || !(fifo_base[SVGA_FIFO_CAPABILITIES] & SVGA_FIFO_CAP_FENCE))
		return true;
	return ((int32_t)(fifo_base[SVGA_FIFO_FENCE] - fence)) >= 0;
}
