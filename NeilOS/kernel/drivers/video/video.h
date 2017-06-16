#ifndef VIDEO_H
#define VIDEO_H

#include <common/types.h>

#define RESOLUTION_X		1024
#define RESOLUTION_Y		768
#define BITS_PER_PIXEL		24

void video_init();
void video_deinit();

void set_pixel(uint16_t x, uint16_t y, uint8_t red, uint8_t green, uint8_t blue);
void set_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
				   uint8_t red, uint8_t green, uint8_t blue);
void update_screen();

#endif
