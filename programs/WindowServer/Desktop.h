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
}

#endif /* DESKTOP_H */
