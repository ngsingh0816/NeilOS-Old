//
//  main.cpp
//  WindowServer
//
//  Created by Neil Singh on 1/25/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include <NeilOS/NeilOS.h>

#include "Controller.h"

int main(int argc, const char* argv[]) {
	int res_x = 1280;//*2;
	int res_y = 800;//*2;
	graphics_info.resolution_x = res_x;
	graphics_info.resolution_y = res_y;
	graphics_info_set(&graphics_info);
	
	return NSApplication::Run(argc, argv, new Controller);
}
