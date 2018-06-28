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
	
	NSLabel* label = NSLabel::Create("Hello\nWorld!", NSRect(15, 15, 200, 15));
	label->SetTextAlignment(NSTextAlignmentCentered);
	label->SizeToFit();
	window->GetContentView()->AddSubview(label);
	
	float y_off_img = label->GetFrame().origin.y + label->GetFrame().size.height;
	
	NSImageView* img_view = NSImageView::Create(new NSImage("/system/images/background.jpg"),
												NSRect(15, y_off_img + 15, 200, 200));
	img_view->SizeToFit();
	window->GetContentView()->AddSubview(img_view);
	y_off_img = img_view->GetFrame().origin.y + img_view->GetFrame().size.height;

	NSCheckBox* check = NSCheckBox::Create("Red", NSRect(15, y_off_img + 15, 100, NSCheckBoxDefaultSize.height));
	check->SizeToFit();
	check->SetAction([label, check](NSView*) {
		label->SetTextColor(check->GetState() ? NSColor<float>::RedColor() : NSColor<float>::BlackColor());
	});
	window->GetContentView()->AddSubview(check);
	
	NSRadioButton* rb1 = NSRadioButton::Create("Enabled", NSRect(15, y_off_img + 55, 100,
																 NSRadioButtonDefaultSize.height));
	rb1->SizeToFit();
	rb1->SetGroupID(1);
	rb1->SetState(NSButtonStateOn);
	rb1->SetAction([check, rb1](NSView*) {
		check->SetEnabled(rb1->GetState());
	});
	
	NSRadioButton* rb2 = NSRadioButton::Create("Disabled", NSRect(rb1->GetFrame().origin.x +
																  rb1->GetFrame().size.width + 15,
																  y_off_img + 55, 100,
																  NSRadioButtonDefaultSize.height));
	rb2->SizeToFit();
	rb2->SetGroupID(1);
	rb2->SetState(NSButtonStateOff);
	rb2->SetAction([check, rb1](NSView*) {
		check->SetEnabled(rb1->GetState());
	});
	window->GetContentView()->AddSubview(rb1);
	window->GetContentView()->AddSubview(rb2);
	
	NSProgressBar* progress = NSProgressBar::Create(NSRect(NSPoint(15, y_off_img + 95), NSProgressBarDefaultSize));
	progress->SetMinValue(0);
	progress->SetMaxValue(100);
	window->GetContentView()->AddSubview(progress);
	
	NSSlider* slider = NSSlider::Create(NSRect(NSPoint(15, y_off_img + 120), NSSliderDefaultSize));
	slider->SetIsContinuous(true);
	slider->SetMinValue(0);
	slider->SetMaxValue(100);
	slider->SetAction([progress](NSSlider* s) {
		NSColor<float> from = NSColor<float>::UITurquoiseColor();
		NSColor<float> to = NSColor<float>::UIBlueColor();
		progress->SetFillColor((s->GetValue() / 100.0) * (to - from) + from);
	});
	//slider->SetNumberOfTickMarks(4, true);
	window->GetContentView()->AddSubview(slider);
	
	NSTextField* textField = NSTextField::Create(NSRect(NSPoint(15, y_off_img + 155), NSTextFieldDefaultSize));
	textField->SetPlaceholder("Enter text");
	textField->SetAction([label, textField](NSView*) {
		label->SetText(textField->GetText());
		label->SizeToFit();
	});
	window->GetContentView()->AddSubview(textField);
	
	NSComboBox* combo = NSComboBox::Create(NSRect(NSPoint(30 + NSTextFieldDefaultSize.width,
																y_off_img + 120), NSComboBoxDefaultSize));
	combo->SetPlaceholder("Combo");
	combo->GetMenu()->AddItem(new NSMenuItem("Item 1"));
	combo->GetMenu()->AddItem(new NSMenuItem("Item 2"));
	combo->GetMenu()->AddItem(new NSMenuItem("Item 3"));
	combo->SetAction([label, combo](NSView*) {
		label->SetText(combo->GetText());
		label->SizeToFit();
	});
	window->GetContentView()->AddSubview(combo);
	
	NSPopupButton* popup = NSPopupButton::Create("Popup", NSRect(NSPoint(30 + NSTextFieldDefaultSize.width,
																y_off_img + 155), NSPopupButtonDefaultSize));
	popup->GetMenu()->AddItem(new NSMenuItem("Item 1"));
	popup->GetMenu()->AddItem(new NSMenuItem("Item 2"));
	popup->GetMenu()->AddItem(new NSMenuItem("Item 3"));
	popup->SetAction([label, popup](NSView*) {
		label->SetText(popup->GetText());
		label->SizeToFit();
	});
	window->GetContentView()->AddSubview(popup);
	
	NSBox* box = NSBox::Create(NSRect(630, 15, 155, window->GetContentFrame().size.height - 30));
	box->SetText("Box");
	
	// Label button
	NSButton* button = NSButton::Create("Label", NSRect(15, 15, 125, NSButtonDefaultSize.height));
	button->SetAction([label](NSView*) {
		static int clicked = 0;
		clicked++;
		label->SetText("Clicked " + std::to_string(clicked) + " times.");
		label->SizeToFit();
	});
	box->AddSubview(button);
	
	// Progress button
	NSButton* pbutton = NSButton::Create("Progress", NSRect(15, 60, 125, NSButtonDefaultSize.height));
	pbutton->SetAction([progress](NSView*) {
		float value = progress->GetValue() + 20;
		if (value > 100)
			value -= 100;
		progress->SetValue(value);
	});
	box->AddSubview(pbutton);
	
	// Textfield button
	NSButton* tbutton = NSButton::Create("Text Field", NSRect(15, 105, 125, NSButtonDefaultSize.height));
	tbutton->SetAction([label, textField](NSView*) {
		label->SetText(textField->GetText());
		label->SizeToFit();
	});
	box->AddSubview(tbutton);
	
	window->GetContentView()->AddSubview(box);
	
	window->Show();
}
