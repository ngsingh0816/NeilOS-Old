//
//  graphics.h
//  product
//
//  Created by Neil Singh on 1/24/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <common/types.h>

typedef struct {
	uint32_t resolution_x;
	uint32_t resolution_y;
} graphics_info_t;


// Initialize the graphics
bool graphics_init();
void graphics_enable();
void graphics_disable();
bool graphics_enabled();

// Map the screen's framebuffer into memory (updates are not seen until update rect is called)
uint8_t* graphics_fb_map();

// Unmap the screen's framebuffer from memory
void graphics_fb_unmap(uint8_t* fb);

// Get graphics info
uint32_t graphics_info_get(graphics_info_t* info);

// Set graphics info
uint32_t graphics_info_set(graphics_info_t* info);

// Update a portion of the screen
uint32_t graphics_update_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

// Set cursor position (internal - not syscall)
uint32_t graphics_cursor_position_set(uint32_t x, uint32_t y, bool visible);

// Set cursor image
uint32_t graphics_cursor_image_set(uint32_t hotspot_x, uint32_t hotspot_y, uint32_t width, uint32_t height, void* data);

// Create a fence
uint32_t graphics_fence_create();

// Sync to a fence
uint32_t graphics_fence_sync(uint32_t fence);

// Has a fence passed
bool graphics_fence_passed(uint32_t fence);

#endif /* GRAPHICS_H */
