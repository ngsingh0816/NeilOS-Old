//
//  graphics.c
//  product
//
//  Created by Neil Singh on 2/5/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "graphics.h"
#include <unistd.h>
#include <sys/errno.h>

extern "C" uint8_t* sys_graphics_fb_map();
extern "C" void sys_graphics_fb_unmap(uint8_t* fb);
extern "C" uint32_t sys_graphics_info_get(graphics_info_t* info);
extern "C" uint32_t sys_graphics_info_set(graphics_info_t* info);
extern "C" uint32_t sys_graphics_update_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
extern "C" uint32_t sys_graphics_cursor_image_set(uint32_t hotspot_x, uint32_t hotspot_y, uint32_t width,
												  uint32_t height, void* data);
extern "C" uint32_t sys_graphics_fence_create();
extern "C" uint32_t sys_graphics_fence_sync(uint32_t fence);
extern "C" bool sys_graphics_fence_passed(uint32_t fence);

uint8_t* graphics_fb_map() {
	return sys_graphics_fb_map();
}

void graphics_fb_unmap(uint8_t* fb) {
	sys_graphics_fb_unmap(fb);
}

uint32_t graphics_info_get(graphics_info_t* info) {
	int32_t ret = sys_graphics_info_get(info);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

uint32_t graphics_info_set(graphics_info_t* info) {
	int32_t ret = sys_graphics_info_set(info);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

uint32_t graphics_update_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	int32_t ret = sys_graphics_update_rect(x, y, width, height);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

uint32_t graphics_cursor_image_set(uint32_t hotspot_x, uint32_t hotspot_y, uint32_t width, uint32_t height, void* data) {
	int32_t ret = sys_graphics_cursor_image_set(hotspot_x, hotspot_y, width, height, data);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

uint32_t graphics_fence_create() {
	return sys_graphics_fence_create();
}

uint32_t graphics_fence_sync(uint32_t fence) {
	int32_t ret = sys_graphics_fence_sync(fence);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

bool graphics_fence_passed(uint32_t fence) {
	return sys_graphics_fence_passed(fence);
}
