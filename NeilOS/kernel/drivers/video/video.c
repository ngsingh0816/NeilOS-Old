#include "video.h"
#include <common/lib.h>

#define VBE_INDEX_PORT		0x01CE
#define VBE_DATA_PORT		0x01CF

#define VBE_INDEX_ID			0
#define VBE_INDEX_XRES			1
#define VBE_INDEX_YRES			2
#define VBE_INDEX_BPP			3
#define VBE_INDEX_ENABLE		4
#define VBE_INDEX_BANK			5
#define VBE_INDEX_VIRT_WIDTH	6
#define VBE_INDEX_VIRT_HEIGHT	7
#define VBE_INDEX_X_OFFSET		8
#define VBE_INDEX_Y_OFFSET		9

#define VBE_DISABLED			0
#define VBE_ENABLED				1

#define VBE_24BPP				0x18

// This mode gives 8MB of VRAM
#define VBE_MODE				0xB0C4

#define VRAM_ADDRESS			0xA0000
#define BANK_SIZE				(64 * 1024)

uint8_t screen[RESOLUTION_X * RESOLUTION_Y * 3];

uint32_t current_bank = -1;

// Helper function to quickly write to a port
static inline void vbe_write_data(uint16_t index, uint16_t data) {
	outw(index, VBE_INDEX_PORT);
	outw(data, VBE_DATA_PORT);
}

static inline uint16_t vbe_read_data(uint16_t index) {
	outw(index, VBE_INDEX_PORT);
	return inw(VBE_DATA_PORT);
}

// Initialize the VBE video
void video_init() {
	// Disable the VBE
	vbe_write_data(VBE_INDEX_ENABLE, VBE_DISABLED);
	
	// Choose 1280 x 1024, 24bpp
	vbe_write_data(VBE_INDEX_ID, VBE_MODE);
	vbe_write_data(VBE_INDEX_XRES, RESOLUTION_X);
	vbe_write_data(VBE_INDEX_YRES, RESOLUTION_Y);
	vbe_write_data(VBE_INDEX_BPP, VBE_24BPP);
	
	// Enable the VBE
	vbe_write_data(VBE_INDEX_ENABLE, VBE_ENABLED);
	
	// Clear the screen to black
	memset(screen, 0, RESOLUTION_Y * RESOLUTION_X * 3);
	/*int val = 0;
	for (;;) {
		int x,y;
		for (y = 0; y < RESOLUTION_Y; y++) {
			for (x = 0; x < RESOLUTION_X; x++) {
				set_pixel(x, y, 255, 0, 0);
			}
		}
		
		val += 10;
		if (val >= RESOLUTION_X)
			val = 0;
		
		for (y = 0; y < 5; y++) {
			for (x = 0; x < 20; x++) {
				set_pixel((x + val) % RESOLUTION_X, RESOLUTION_Y / 2 - 2 + y, 0, 0, 0);
			}
		}
		
		update_screen();
	}*/
}

#define ORIGINAL_X	320
#define ORIGINAL_Y	240

// Go back to normal text mode
void video_deinit() {
	// Disable the VBE
	vbe_write_data(VBE_INDEX_ENABLE, VBE_DISABLED);
	vbe_write_data(VBE_INDEX_XRES, ORIGINAL_X);
	vbe_write_data(VBE_INDEX_YRES, ORIGINAL_Y);
}

// Sets a pixel at the desired location
void set_pixel(uint16_t x, uint16_t y, uint8_t red, uint8_t green, uint8_t blue) {
	return;
	// Write the data
	uint32_t offset = (x + RESOLUTION_X * y) * 3;
	uint32_t bank = offset / BANK_SIZE;
	offset = offset % BANK_SIZE;
	
	if (current_bank != bank) {
		current_bank = bank;
		vbe_write_data(VBE_INDEX_BANK, bank);
	}
	
	uint8_t* address = (uint8_t*)(VRAM_ADDRESS + offset);
	*(address++) = blue;
	*(address++) = green;
	*(address++) = red;
	
	/*uint32_t offset = (x + RESOLUTION_X * y) * 3;
	screen[offset] = blue;
	screen[offset + 1] = green;
	screen[offset + 2] = red;*/
}

void set_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
				   uint8_t red, uint8_t green, uint8_t blue) {
	//return;
	/*int sy;
	int ey = y + height;
	uint32_t addr = (x + RESOLUTION_X * y) * 3;
	for (sy = y; sy < ey; sy++) {
		uint32_t offset = addr;
		addr += RESOLUTION_X * 3;
		int sx;
		for (sx = 0; sx < width; sx++) {
			screen[offset++] = blue;
			screen[offset++] = green;
			screen[offset++] = red;
		}
	}*/
	
	/*int sy, sx;
	for (sy = y; sy < y + height; sy++) {
		for (sx = x; sx < x + width; sx++)
			set_pixel(sx, sy, red, green, blue);
	}*/
}

// Copies the screen over
void update_screen() {
	/*return;
	set_rectangle(8, 16, 8, 16, 255, 255, 255);
	
	//int flags = 0;
	//cli_and_save(flags);
				 
	// Figure out how many full banks to copy over
	int bank;
	int totalBanks = (RESOLUTION_X * RESOLUTION_Y * 3 / BANK_SIZE);
	int pos = 0;
	
	// Copy in 64 KB banks
	for (bank = 0; bank < totalBanks; bank++) {
		vbe_write_data(VBE_INDEX_BANK, bank);
		memcpy((uint8_t*)VRAM_ADDRESS, &screen[pos], BANK_SIZE);
		pos += BANK_SIZE;
	}
	
	// Copy the last bank
	vbe_write_data(VBE_INDEX_BANK, bank);
	memcpy((uint8_t*)VRAM_ADDRESS, &screen[pos], RESOLUTION_X * RESOLUTION_Y * 3 - pos);
	
	//restore_flags(flags);*/
}
