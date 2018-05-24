//
//  NSWindow.h
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSWINDOW_H
#define NSWINDOW_H

#include "../Core/NSResponder.h"
#include "../Core/NSTypes.h"

#include <graphics/graphics.h>

#include <string>

#define WINDOW_TITLE_BAR_HEIGHT		22

class NSView;

class NSWindow : public NSResponder {
public:
	static NSWindow* Create(std::string title, NSRect frame);
	~NSWindow();
	
	NSView* GetContentView();
	void SetContentView(NSView* view);
	
	graphics_context_t* GetContext();
	
	std::string GetTitle() const;
	void SetTitle(std::string title);
	
	uint32_t GetWindowID() const;
	NSRect GetFrame() const;
	void SetFrame(NSRect rect);
	void SetFrameOrigin(NSPoint p);
	void SetFrameSize(NSSize size);
	
	void SetDeallocsWhenClose(bool close);
	bool GetDeallocsWhenClose() const;
	
	void Show();
	bool IsVisible();
	void Close();
	
	bool IsKeyWindow() const;
	// Makes this window the key window and moves it to the front
	void MakeKeyWindow();
	
	NSView* FirstResponder() const;
	void MakeFirstResponder(NSView* view);
private:
	NSWindow(std::string title, NSRect frame);
	void SetupProjMatrix();
	
	NSView* content_view;
	NSView* first_responder;
	
	std::string title;
	NSRect frame;
	uint32_t window_id;
	bool dealloc;
	bool visible;
	
	graphics_context_t context;
};

#endif /* NSWINDOW_H */