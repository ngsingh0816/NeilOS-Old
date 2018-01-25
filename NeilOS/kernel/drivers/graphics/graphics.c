//
//  graphics.c
//  product
//
//  Created by Neil Singh on 1/24/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "graphics.h"
#include <syscalls/interrupt.h>
#include "svga/svga.h"

// Initialize the graphics
bool graphics_init() {
	return svga_init();
}

void graphics_enable() {
	svga_enable();
}

void graphics_disable() {
	svga_disable();
}

bool graphics_enabled() {
	return svga_enabled();
}

// Map the screen's framebuffer into memory (updates are not seen until update rect is called)
uint8_t* graphics_fb_map() {
	uint32_t fb_base = (uint32_t)vm_virtual_to_physical((uint32_t)svga_framebuffer());
	uint32_t fb_size = svga_framebuffer_size();
	
	down(&current_pcb->lock);
	vm_lock();
	
	uint32_t vaddr = vm_get_next_unmapped_pages((fb_size - 1) / FOUR_MB_SIZE + 1, VIRTUAL_MEMORY_USER);
	if (!vaddr) {
		vm_unlock();
		return NULL;
	}
	uint32_t paddr = fb_base - (fb_base % FOUR_MB_SIZE);
	for (uint32_t z = 0; z < fb_size; z += FOUR_MB_SIZE) {
		page_list_t* t = page_list_get_no_mem(&current_pcb->page_list, vaddr + z, MEMORY_RW, true);
		t->paddr = paddr + z;
		page_list_map(t, false);
	}
	
	vm_unlock();
	up(&current_pcb->lock);

	fb_base = (fb_base - paddr + vaddr);
	
	return (uint8_t*)fb_base;
}

// Unmap the screen's framebuffer from memory
void graphics_fb_unmap(uint8_t* fb) {
	uint32_t fb_base = (uint32_t)fb;
	uint32_t fb_size = svga_framebuffer_size();
	vm_lock();
	vm_unmap_pages(fb_base, fb_base + fb_size);
	vm_unlock();
}

// Get graphics info
uint32_t graphics_info_get(graphics_info_t* info) {
	if (!info)
		return -EINVAL;
	
	info->resolution_x = svga_resolution_get_x();
	info->resolution_y = svga_resolution_get_y();
	
	return 0;
}

// Set graphics info
uint32_t graphics_info_set(graphics_info_t* info) {
	if (!info)
		return -EINVAL;
	
	svga_set_mode(info->resolution_x, info->resolution_y, svga_bpp_get());
	
	return 0;
}

// Update a portion of the screen
uint32_t graphics_update_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	svga_update(x, y, width, height);
	return 0;
}

// Set cursor position (internal - not syscall)
uint32_t graphics_cursor_position_set(uint32_t x, uint32_t y, bool visible) {
	svga_set_cursor_position(x, y, visible);
	return 0;
}

// Set cursor image
uint32_t graphics_cursor_image_set(uint32_t hotspot_x, uint32_t hotspot_y, uint32_t width, uint32_t height, void* data) {
	svga_set_cursor_image(hotspot_x, hotspot_y, width, height, data);
	return 0;
}

// Create a fence
uint32_t graphics_fence_create() {
	return svga_fence_insert();
}

// Sync to a fence
uint32_t graphics_fence_sync(uint32_t fence) {
	svga_fence_sync(fence);
	return 0;
}

// Has a fence passed
bool graphics_fence_passed(uint32_t fence) {
	return svga_fence_passed(fence);
}
