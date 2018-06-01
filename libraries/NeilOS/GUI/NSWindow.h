//
//  NSWindow.h
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSWINDOW_H
#define NSWINDOW_H

#include "../Core/NSLock.h"
#include "../Core/NSResponder.h"
#include "../Core/NSTypes.h"

#include <graphics/graphics.h>

#include <string>
#include <vector>

#define WINDOW_TITLE_BAR_HEIGHT		22

class NSView;
class NSWindowObserver;

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
	void ResignKeyWindow();
	
	NSView* FirstResponder() const;
	void MakeFirstResponder(NSView* view);
	
	void MouseDown(NSEvent* event) override;
	void MouseDragged(NSEvent* event) override;
	void MouseUp(NSEvent* event) override;
	void MouseMoved(NSEvent* event) override;
	void MouseScrolled(NSEvent* event) override;
	
	void KeyDown(NSEvent* event) override;
	void KeyUp(NSEvent* event) override;
	
	void AddObserver(NSWindowObserver* observer);
	void RemoveObserver(NSWindowObserver* observer);
	
private:
	friend class NSView;
	
	NSWindow(std::string title, NSRect frame);
	void SetupProjMatrix();
	
	void AddUpdateRect(NSRect rect);
	void AddUpdateRects(std::vector<NSRect> rects);
	void PushUpdates();
	
	NSView* content_view = NULL;
	NSView* first_responder = NULL;
	NSView* down_view = NULL;
	std::vector<NSWindowObserver*> observers;
	
	std::string title;
	NSRect frame;
	uint32_t window_id;
	bool dealloc = false;
	bool visible = false;
	
	graphics_context_t context;
	
	NSLock rect_lock;
	std::vector<NSRect> update_rects;
	bool update_requested = false;
};

class NSWindowObserver {
public:
	virtual void BecomeKeyWindow() {};
	virtual void ResignKeyWindow() {};
	virtual void Show() {};
	virtual void Close() {};
	virtual void SetFrame() {};
private:
};

#endif /* NSWINDOW_H */
