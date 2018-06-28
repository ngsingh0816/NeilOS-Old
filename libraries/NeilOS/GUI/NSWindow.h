//
//  NSWindow.h
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSWINDOW_H
#define NSWINDOW_H

#include "../Core/NSCursor.h"
#include "../Core/NSLock.h"
#include "../Core/NSResponder.h"
#include "../Core/NSTypes.h"

#include <graphics/graphics.h>

#include <string>
#include <vector>

#define WINDOW_TITLE_BAR_HEIGHT		22

#define NSWINDOW_BUTTON_MASK_CLOSE		(1 << 0)
#define NSWINDOW_BUTTON_MASK_MIN		(1 << 1)
#define NSWINDOW_BUTTON_MASK_MAX		(1 << 2)
#define NSWINDOW_BUTTON_MASK_ALL		0x7

class NSView;
class NSWindowObserver;

class NSViewContainer {
public:
	virtual ~NSViewContainer() {};
	virtual NSView* FirstResponder() const { return first_responder; };
	virtual void MakeFirstResponder(NSView* view);
	virtual void AddUpdateRects(const std::vector<NSRect>& rects) = 0;
	virtual graphics_context_t* GetContext() = 0;
	virtual float GetBackingScaleFactor() const;
	virtual void ViewAdded(NSView* view) {};
	virtual NSPoint GetOffset() const { return NSPoint(); };
	// Rect in view's coordinate system
	virtual NSRect PrepareDraw(NSView* view, const NSRect& rect) { return rect; };
	virtual void FinishDraw(NSView* view, const NSRect& rect) {};
	// Absolute coordinates
	virtual NSPoint GetScissorOffset(const NSRect&) const { return NSPoint(); };
protected:
	NSView* first_responder = NULL;
};

class NSWindow : public NSViewContainer, public NSResponder {
public:
	static NSWindow* Create(std::string title, const NSRect& frame);
	~NSWindow();
	
	NSView* GetContentView();
	void SetContentView(NSView* view);
	
	virtual graphics_context_t* GetContext() override;
	virtual float GetBackingScaleFactor() const override;
	
	std::string GetTitle() const;
	void SetTitle(std::string title);
	
	uint32_t GetWindowID() const;
	NSRect GetFrame() const;
	NSRect GetContentFrame() const;
	void SetFrame(const NSRect& rect);
	void SetFrameOrigin(const NSPoint& p);
	void SetFrameSize(const NSSize& size);
	virtual NSPoint GetOffset() const override;
	
	uint32_t GetButtonMask() const;
	void SetButtonMask(uint32_t mask);
	
	void SetDeallocsWhenClose(bool close);
	bool GetDeallocsWhenClose() const;
	
	void Show(bool animates = true);
	bool IsVisible();
	void Close();
	
	bool IsKeyWindow() const;
	// Makes this window the key window and moves it to the front
	void MakeKeyWindow();
	void ResignKeyWindow();
	
	virtual void MakeFirstResponder(NSView* view) override;
	virtual void AddUpdateRects(const std::vector<NSRect>& rects) override;
	
	void MouseDown(NSEventMouse* event) override;
	void MouseDragged(NSEventMouse* event) override;
	void MouseUp(NSEventMouse* event) override;
	void MouseMoved(NSEventMouse* event) override;
	void MouseScrolled(NSEventMouse* event) override;
	
	void KeyDown(NSEventKey* event) override;
	void KeyUp(NSEventKey* event) override;
	
	void AddObserver(NSWindowObserver* observer);
	void RemoveObserver(NSWindowObserver* observer);
	
private:
	friend class NSEventWindowSetFrame;
	friend class NSEventWindowMakeKey;
	
	NSWindow(std::string title, const NSRect& frame);
	
	void PushUpdates();
	
	void SetFrameInternal(const NSRect& frame);
	void MakeKeyInternal(bool key);
	
	void CheckCursorRegions(const NSPoint& p);
		
	NSView* content_view = NULL;
	NSView* first_responder = NULL;
	std::vector<NSWindowObserver*> observers;
	uint32_t button_mask = NSWINDOW_BUTTON_MASK_ALL;
	
	std::string title;
	NSRect frame;
	uint32_t window_id;
	bool dealloc = false;
	bool visible = false;
	bool is_key = false;
	NSCursorRegion* current_region = NULL;
	NSCursor::Cursor current_cursor = NSCursor::CURSOR_DEFAULT;
	
	graphics_context_t context;
	float bsf = 1;
	
	NSLock rect_lock;
	std::vector<NSRect> update_rects;
	bool update_requested = false;
	bool enable_updates = true;
};

class NSWindowObserver {
public:
	virtual void BecomeKeyWindow(NSWindow*) {};
	virtual void ResignKeyWindow(NSWindow*) {};
	virtual void Show(NSWindow*) {};
	virtual void Close(NSWindow*) {};
	virtual void SetFrame(NSWindow*) {};
private:
};

#endif /* NSWINDOW_H */
