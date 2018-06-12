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
	NSWindow* window = NSWindow::Create("Calculator", NSRect((1440 - 800) / 2.0, (900 - 600) / 2.0, 800, 600));
	
	NSLabel* label = NSLabel::Create("Hello World!", NSRect(15, 15, 200, 15));
	label->SizeToFit();
	window->GetContentView()->AddSubview(label);
	
	NSImageView* img_view = NSImageView::Create(new NSImage("/system/images/background.jpg"), NSRect(15, 30, 200, 200));
	img_view->SizeToFit();
	window->GetContentView()->AddSubview(img_view);
	
	NSBox* box = NSBox::Create(NSRect(230, 15, 155, 250));
	box->SetText("Box");
	
	NSButton* button = NSButton::Create("Click Me!", NSRect(15, 15, 125, NSButtonDefaultSize.height));
	button->SetResizingMask(NSViewMaxXMargin | NSViewWidthSizable | NSViewMinYMargin | NSViewMinXMargin);
	button->SetAction([label](NSControl*) {
		static int clicked = 0;
		clicked++;
		label->SetText("Clicked " + std::to_string(clicked) + " times.");
		label->SizeToFit();
	});
	box->AddSubview(button);
	window->GetContentView()->AddSubview(box);
	
	
	window->Show();
}
