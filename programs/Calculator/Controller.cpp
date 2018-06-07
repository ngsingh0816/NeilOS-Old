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
	
	NSLabel* label = NSLabel::Create("Hello World!", NSRect(15, 15, 200, 15));
	label->SizeToFit();
	window->GetContentView()->AddSubview(label);
	
	NSImageView* img_view = NSImageView::Create(new NSImage("/system/images/background.jpg"), NSRect(15, 30, 200, 200));
	img_view->SizeToFit();
	window->GetContentView()->AddSubview(img_view);
	
	NSButton* button = NSButton::Create("Click Me!", NSRect(230, 15, 155, NSButtonDefaultSize.height));
	button->SetAction([label](NSControl*) {
		static int clicked = 0;
		clicked++;
		label->SetText("Clicked " + std::to_string(clicked) + " times.");
		label->SizeToFit();
	});
	window->GetContentView()->AddSubview(button);
	
	window->Show();
}
