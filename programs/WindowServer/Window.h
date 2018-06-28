//
//  Window.h
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef WINDOW_H
#define WINDOW_H

#include <NeilOS/NeilOS.h>

#include <vector>

namespace Window {
	void Load(volatile float* percent, float percent_start, float percent_end);

	void ProcessEvent(uint8_t* data, uint32_t length);
	
	void Draw(const std::vector<NSRect>& rects);
	
	void MakeKeyWindow(uint32_t pid, uint32_t wid);
	bool IsVisible(uint32_t pid, uint32_t wid);
	uint32_t FindLastVisibleWindow(uint32_t pid);
	uint32_t FindLastVisiblePid(uint32_t not_pid);
	
	void DestroyWindows(uint32_t pid);
	
	bool MouseDown(NSPoint p, NSMouseButton mouse, uint32_t click_count);
	bool MouseMoved(NSPoint p);
	bool MouseUp(NSPoint p, NSMouseButton mouse);
	bool MouseScrolled(NSPoint p, float delta_x, float delta_y);
	bool KeyDown(unsigned char key, NSModifierFlags flags);
	bool KeyUp(unsigned char key, NSModifierFlags flags);
};

#endif /* WINDOW_H */
