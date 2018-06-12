//
//  graphics.h
//  product
//
//  Created by Neil Singh on 2/5/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef GRAPHICS_H
#define GRAPHICS_H
	
#include <stdbool.h>
#include <stdint.h>
#include "defs.h"
#include "NSMatrix.h"

// 2D

// Map the screen's framebuffer into memory (updates are not seen until update rect is called)
uint8_t* graphics_fb_map(void);

// Unmap the screen's framebuffer from memory
void graphics_fb_unmap(uint8_t* fb);

// Get graphics info
uint32_t graphics_info_get(graphics_info_t* info);

// Set graphics info
uint32_t graphics_info_set(graphics_info_t* info);

// Update a portion of the screen
uint32_t graphics_update_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

// Set cursor image
uint32_t graphics_cursor_image_set(uint32_t hotspot_x, uint32_t hotspot_y, uint32_t width, uint32_t height, void* data);

// Create a fence
uint32_t graphics_fence_create(void);

// Sync to a fence
uint32_t graphics_fence_sync(uint32_t fence);

// Has a fence passed
bool graphics_fence_passed(uint32_t fence);
	
// 3D

// Creates a new context with the specified options (returns the context id or context.cid = 0 if error)
graphics_context_t graphics_context_create(uint32_t width, uint32_t height, uint32_t color_bits, uint32_t depth_bits,
										   uint32_t stencil_bits);
// Resize a graphics context
void graphics_context_resize(graphics_context_t* context, uint32_t width, uint32_t height);
// Destroys a context
void graphics_context_destroy(graphics_context_t* context);

// Creates a new buffer (in VRAM) with the specified options (returns buffer id or 0 if error).
uint32_t graphics_buffer_create(uint32_t size, uint32_t flags);

// Creates a new buffer (in VRAM) with the specified options and format
uint32_t graphics_buffer_create(uint32_t size, uint32_t flags, uint32_t format);

// Creates a new 2D buffer (in VRAM) with the specified options and format
uint32_t graphics_buffer_create(uint32_t width, uint32_t height, uint32_t flags, uint32_t format);

// Upload data to a buffer
void graphics_buffer_data(uint32_t bid, const void* data, uint32_t size);

// Upload 2d data to a buffer
void graphics_buffer_data(uint32_t bid, const void* data, uint32_t width, uint32_t height, uint32_t size);

// Read data from a buffer
void graphics_read_data(uint32_t bid, void* data, uint32_t size);

// Read 2d data form a buffer
void graphics_read_data(uint32_t bid, void* data, uint32_t width, uint32_t height, uint32_t size);

// Upload data to a buffer at an offset
void graphics_buffer_sub_data(uint32_t bid, const void* data, uint32_t offset, uint32_t size);

// Upload 2d data to a buffer at an offset
void graphics_buffer_sub_data(uint32_t bid, const void* data, uint32_t x, uint32_t y,
							  uint32_t width, uint32_t height, uint32_t size);

// Change size / format of buffer
// Note: may invalidate buffer contents
void graphics_buffer_reformat(uint32_t bid, uint32_t size, uint32_t flags);
void graphics_buffer_reformat(uint32_t bid, uint32_t size, uint32_t flags, uint32_t format);
void graphics_buffer_reformat(uint32_t bid, uint32_t width, uint32_t height, uint32_t flags, uint32_t format);

// Copy data from one buffer to another
void graphics_buffer_copy(uint32_t dest_bid, uint32_t src_bid, uint32_t dest_x, uint32_t dest_y,
						  uint32_t src_x, uint32_t src_y, uint32_t width, uint32_t height);

// Destroy a buffer
void graphics_buffer_destroy(uint32_t bid);

// Clear section
void graphics_clear(graphics_context_t* context, uint32_t color, float depth, uint32_t stencil, uint32_t bits,
					uint32_t x, uint32_t y, uint32_t width, uint32_t height);

// Draw primitives
void graphics_draw(graphics_context_t* context, uint32_t primitive_type, uint32_t primitive_count,
				   graphics_vertex_array_t* arrays, uint32_t num_arrays);

// Draw indexed primitives
void graphics_draw_elements(graphics_context_t* context, uint32_t primitive_type, uint32_t primitive_count,
							graphics_vertex_array_t* indices, graphics_vertex_array_t* arrays, uint32_t num_arrays);

// Set matrix transform
void graphics_transform_set(graphics_context_t* context, uint32_t type, const NSMatrix& matrix);

// Set render state
void graphics_renderstate_seti(graphics_context_t* context, uint32_t type, uint32_t value);
void graphics_renderstate_setf(graphics_context_t* context, uint32_t type, float value);

// Set texture state
void graphics_texturestate_seti(graphics_context_t* context, uint32_t stage, uint32_t type, uint32_t value);
void graphics_texturestate_setf(graphics_context_t* context, uint32_t stage, uint32_t type, uint32_t value);

// Set render target
void graphics_rendertarget_set(graphics_context_t* context, int target, int bid);

// Set viewport
void graphics_viewport_set(graphics_context_t* context, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

// Set scissor rect
void graphics_scissor_rect_set(graphics_context_t* context, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

// Present surface to screen
void graphics_present(uint32_t sid, uint32_t screen_x, uint32_t screen_y,
					  uint32_t src_x, uint32_t src_y, uint32_t width, uint32_t height);

// Update the 2D framebuffer with the most recent data
void graphics_present_readback(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

#endif /* GRAPHICS_H */

