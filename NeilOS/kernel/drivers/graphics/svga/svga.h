//
//  svga.h
//  product
//
//  Created by Neil Singh on 1/22/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef SVGA_H
#define SVGA_H

#include <common/types.h>

// Initializes the VMWare SVGA-II device and enables it
bool svga_init();
// Enables the device
void svga_enable();
// Disables the device (returns to text printing)
void svga_disable();
bool svga_enabled();

// Pointer to the framebuffer
uint8_t* svga_framebuffer();
uint32_t svga_framebuffer_size();

// Set the resolution and bpp
void svga_set_mode(uint32_t width, uint32_t height, uint32_t bpp);
uint32_t svga_resolution_get_x();
uint32_t svga_resolution_get_y();
uint32_t svga_bpp_get();

// Update a region of the screen
void svga_update(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
// Set the cursor image (premultiplied 32-bit BGRA image)
void svga_set_cursor_image(uint32_t hotspot_x, uint32_t hotspot_y, uint32_t width, uint32_t height, uint32_t* data);
// Set cursor position
void svga_set_cursor_position(uint32_t x, uint32_t y, bool visible);

// Insert a fence into the FIFO
uint32_t svga_fence_insert();
// Wait until this fence has been processed
void svga_fence_sync(uint32_t fence);
// Returns true if this fence has already been processed
bool svga_fence_passed(uint32_t fence);

#endif /* SVGA_H */
