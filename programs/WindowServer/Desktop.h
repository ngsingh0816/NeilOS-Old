//
//  Desktop.h
//  product
//
//  Created by Neil Singh on 2/16/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef DESKTOP_H
#define DESKTOP_H

#include "Application.h"

#include <NeilOS/NeilOS.h>

#include <vector>

extern graphics_info_t graphics_info;
extern graphics_context_t context;

namespace Desktop {
	typedef enum {
		CURSOR_DEFAULT = 0,
		CURSOR_IBEAM,
		
		CURSOR_MOVE,
		CURSOR_CROSS,
		CURSOR_DRAG_AND_DROP,
		CURSOR_SCREENSHOT,
		
		CURSOR_OPEN_HAND,
		CURSOR_CLOSED_HAND,
		CURSOR_POINTING_HAND,
		
		CURSOR_RESIZE_NORTH,
		CURSOR_RESIZE_NORTHEAST,
		CURSOR_RESIZE_NORTHWEST,
		CURSOR_RESIZE_SOUTH,
		CURSOR_RESIZE_SOUTHEAST,
		CURSOR_RESIZE_SOUTHWEST,
		CURSOR_RESIZE_NORTHEAST_SOUTHWEST,
		CURSOR_RESIZE_NORTHWEST_SOUTHEAST,
		CURSOR_RESIZE_NORTH_SOUTH,
		CURSOR_RESIZE_EAST,
		CURSOR_RESIZE_WEST,
		CURSOR_RESIZE_EAST_WEST,
		
		CURSOR_RESIZE_UP,
		CURSOR_RESIZE_DOWN,
		CURSOR_RESIZE_UP_DOWN,
		CURSOR_RESIZE_LEFT,
		CURSOR_RESIZE_RIGHT,
		CURSOR_RESIZE_LEFT_RIGHT,
		
		CURSOR_ZOOM_IN,
		CURSOR_ZOOM_OUT,
		
		CURSOR_MAX
	} Cursor;
	
	void Load(volatile float* percent, float percent_start, float percent_end);
	void Start(NSThread*);
	void Draw(NSThread*);
	void UpdateRect(NSRect rect);
	void UpdateRects(const std::vector<NSRect>& rects);
	void ForceUpdate();
	bool RectsIntersect(const std::vector<NSRect>& rects, NSRect rect);
	void MouseDown(NSPoint p, NSMouseButton mouse);
	void MouseMoved(NSPoint p);
	void MouseUp(NSPoint p, NSMouseButton mouse);
	void MouseScrolled(NSPoint p, float delta_x, float delta_y);
	void KeyDown(unsigned char key);
	void KeyUp(unsigned char key);
	bool IsMouseDown();
	
	float GetPixelScalingFactor();
	void SetPixelScalingFactor(float factor);
	
	void RegisterApplication(Application::App* app);
	void UnregisterApplication(Application::App* app);
	
	void UpdateMenu();
	
	void SetCursor(Cursor cursor);
	void SetCursor(NSImage* image, NSPoint hotspot = NSPoint());
}

#endif /* DESKTOP_H */
