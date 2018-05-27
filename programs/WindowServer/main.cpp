//
//  main.cpp
//  WindowServer
//
//  Created by Neil Singh on 1/25/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include <NeilOS/NeilOS.h>

#include "Boot.h"
#include "Desktop.h"
#include "Event.h"

int main(int argc, const char* argv[]) {
	int res_x = 1440 * Desktop::GetPixelScalingFactor();
	int res_y = 900 * Desktop::GetPixelScalingFactor();
	graphics_info.resolution_x = res_x;
	graphics_info.resolution_y = res_y;
	graphics_info_set(&graphics_info);
	graphics_info.resolution_x /= Desktop::GetPixelScalingFactor();
	graphics_info.resolution_y /= Desktop::GetPixelScalingFactor();
	
	NSThread::Setup();
	NSRunLoop* loop = NSThread::MainThread()->GetRunLoop();
	loop->AddEvent(NSEventFunction::Create([]() {
		Event::Setup();
		Boot::Load(NSThread::MainThread());
	}));
	loop->Run();
	
	return 0;
}
