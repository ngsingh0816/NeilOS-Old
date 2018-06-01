//
//  Controller.cpp
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "Controller.h"
#include <math.h>

NSWindow* mainWindow = NULL;

void Controller::DidFinishLaunching() {
	srand(time(NULL));
	int x = rand() % 1300 + 50;
	int y = rand() % 800 + 50;
	NSWindow* window = NSWindow::Create("Calculator", NSRect(x, y, 400, 300));
	
	NSLabel* label = NSLabel::Create("Hello World!", NSRect(15, 15, 76.5, 11));
	//label->SizeToFit();
	window->GetContentView()->AddSubview(label);
	
	window->Show();
}
