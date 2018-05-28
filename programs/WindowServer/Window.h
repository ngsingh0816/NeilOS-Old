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
	
	bool MouseDown(NSPoint p, int mouse);
	bool MouseMoved(NSPoint p);
	bool MouseUp(NSPoint p, int mouse);
	bool KeyDown(unsigned char key);
	bool KeyUp(unsigned char key);
};

#endif /* WINDOW_H */
