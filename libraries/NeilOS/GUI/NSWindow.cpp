//
//  NSWindow.cpp
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSWindow.h"

#include "NSView.h"
#include "../Core/NSApplication.h"
#include "../Core/Events/NSEventDefs.cpp"
#include "../Core/NSLock.h"

#include <fcntl.h>
#include <graphics/graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

using std::string;

static uint32_t win_id = 0;
static NSLock win_id_lock;

namespace NSApplication {
	void RegisterWindow(NSWindow* window);
	void DeregisterWindow(NSWindow* window);
}

NSWindow* NSWindow::Create(string t, NSRect r) {
	NSWindow* ret = new NSWindow(t, r);
	
	auto event = NSEventWindowCreate(ret->title, ret->frame, getpid(), ret->window_id, &ret->context);
	NSApplication::SendEvent(&event);
	
	return ret;
}

NSWindow::NSWindow(string t, NSRect f) {
	title = t;
	frame = f;
	win_id_lock.Lock();
	window_id = ++win_id;
	win_id_lock.Unlock();
	
	NSApplication::RegisterWindow(this);
	
	float psf = NSApplication::GetPixelScalingFactor();
	bsf = psf;
	NSSize size = NSSize(f.size.width * psf, (f.size.height - WINDOW_TITLE_BAR_HEIGHT) * psf);
	context = graphics_context_create(size.width, size.height, 24, 15, 1);
	NSView::SetupContext(&context, NSSize(f.size.width, (f.size.height - WINDOW_TITLE_BAR_HEIGHT)));
	
	content_view = NSView::Create(NSRect(0, 0, f.size.width, f.size.height - WINDOW_TITLE_BAR_HEIGHT));
	content_view->SetWindow(this);
	MakeFirstResponder(content_view);
}

NSWindow::~NSWindow() {
	NSApplication::DeregisterWindow(this);
	
	auto event = NSEventWindowDestroy(getpid(), window_id);
	NSApplication::SendEvent(&event);
	
	delete content_view;
	
	graphics_context_destroy(&context);
}

NSView* NSWindow::GetContentView() {
	return content_view;
}

void NSWindow::SetContentView(NSView* view) {
	if (!view)
		return;
	
	delete content_view;
	content_view = view;
}

graphics_context_t* NSWindow::GetContext() {
	return &context;
}

float NSWindow::GetBackingScaleFactor() const {
	return bsf;
}

uint32_t NSWindow::GetWindowID() const {
	return window_id;
}

string NSWindow::GetTitle() const {
	return title;
}

void NSWindow::SetTitle(string t) {
	title = t;
	
	auto event = NSEventWindowSetTitle(getpid(), window_id, title);
	NSApplication::SendEvent(&event);
}

NSRect NSWindow::GetFrame() const {
	return frame;
}

NSRect NSWindow::GetContentFrame() const {
	return NSRect(frame.origin + NSPoint(0, WINDOW_TITLE_BAR_HEIGHT),
				  NSSize(frame.size.width, frame.size.height - WINDOW_TITLE_BAR_HEIGHT));
}

void NSWindow::SetFrameInternal(const NSRect& rect) {
	NSSize old_size = frame.size;
	frame = rect;
	
	if (old_size != rect.size) {
		graphics_context_resize(&context, frame.size.width * bsf, (frame.size.height - WINDOW_TITLE_BAR_HEIGHT) * bsf);
		NSView::SetupContext(&context, NSSize(frame.size.width, frame.size.height - WINDOW_TITLE_BAR_HEIGHT));
		
		enable_updates = false;
		content_view->SetFrame(NSRect(0, 0, frame.size.width, frame.size.height - WINDOW_TITLE_BAR_HEIGHT));
		enable_updates = true;
		AddUpdateRect(content_view->GetFrame());
	}
	
	for (auto& i : observers)
		i->SetFrame();
}

void NSWindow::SetFrame(const NSRect& rect) {
	SetFrameInternal(rect);
	
	auto event = NSEventWindowSetFrame(getpid(), window_id, frame);
	NSApplication::SendEvent(&event);
}

void NSWindow::SetFrameOrigin(const NSPoint& p) {
	SetFrame(NSRect(p, frame.size));
}

void NSWindow::SetFrameSize(const NSSize& size) {
	SetFrame(NSRect(frame.origin, size));
}

void NSWindow::SetDeallocsWhenClose(bool close) {
	dealloc = close;
}

bool NSWindow::GetDeallocsWhenClose() const {
	return dealloc;
}

void NSWindow::Show() {
	if (visible)
		return;
	
	visible = true;
	is_key = true;
	
	content_view->DrawRect(NSRect(NSPoint(), content_view->frame.size));
	
	auto event = NSEventWindowShow(getpid(), window_id, true);
	NSApplication::SendEvent(&event);
	
	for (auto& i : observers)
		i->Show();
}

bool NSWindow::IsVisible() {
	return visible;
}

void NSWindow::Close() {
	auto event = NSEventWindowShow(getpid(), window_id, false);
	NSApplication::SendEvent(&event);
	
	visible = false;
	
	for (auto& i : observers)
		i->Close();
	
	if (dealloc)
		delete this;
}

bool NSWindow::IsKeyWindow() const {
	return is_key;
}

void NSWindow::MakeKeyInternal(bool key) {
	if (key) {
		is_key = true;
		
		for (auto& i : observers)
			i->BecomeKeyWindow();
	} else {
		is_key = false;
		
		for (auto& i : observers)
			i->ResignKeyWindow();
	}
}

void NSWindow::MakeKeyWindow() {
	auto event = NSEventWindowMakeKey(getpid(), window_id, true);
	NSApplication::SendEvent(&event);
	
	MakeKeyInternal(true);
}

void NSWindow::ResignKeyWindow() {
	auto event = NSEventWindowMakeKey(getpid(), window_id, false);
	NSApplication::SendEvent(&event);
	
	MakeKeyInternal(false);
}

void NSWindow::CheckCursorRegions(const NSPoint& p) {
	NSCursorRegion* region = content_view->GetCursorRegionAtPoint(p);
	if (region) {
		if (current_region != region) {
			NSImage* image = region->GetImage();
			if (image)
				NSCursor::SetCursor(image, region->GetHotspot());
			else
				NSCursor::SetCursor(region->GetCursor());
			current_cursor = region->GetCursor();
		}
	}
	else if (current_cursor != NSCursor::CURSOR_DEFAULT) {
		NSCursor::SetCursor(NSCursor::CURSOR_DEFAULT);
		current_cursor = NSCursor::CURSOR_DEFAULT;
	}
	current_region = region;
}

NSView* NSWindow::FirstResponder() const {
	return first_responder;
}

void NSWindow::MakeFirstResponder(NSView* view) {
	if (!view->AcceptsFirstResponder())
		return;
	
	if (first_responder)
		first_responder->ResignFirstResponder();
	
	first_responder = view;
	view->BecomeFirstResponder();
}

void NSWindow::AddUpdateRect(const NSRect& rect) {
	std::vector<NSRect> rects = { rect };
	AddUpdateRects(rects);
}

void NSWindow::AddUpdateRects(std::vector<NSRect> rects) {
	std::vector<NSRect> real;
	real.reserve(rects.size());
	for (unsigned int z = 0; z < rects.size(); z++) {
		NSRect& rect = rects[z];
		if (!NSRectClamp(rect, NSRect(0, 0, frame.size.width, frame.size.height - WINDOW_TITLE_BAR_HEIGHT), &rect))
			continue;
		real.emplace_back(rect);
	}
	
	rect_lock.Lock();
	update_rects.insert(update_rects.end(), real.begin(), real.end());
	if (update_rects.size() != 0 && !update_requested) {
		NSHandler([this](NSThread*) {
			this->PushUpdates();
		}).Post(NSThread::MainThread(), 1 / 60.0f, NSHandlerDefaultPriortiy, false);
		update_requested = true;
	}
	rect_lock.Unlock();
}

void NSWindow::PushUpdates() {
	rect_lock.Lock();
	if (update_rects.size() == 0) {
		rect_lock.Unlock();
		return;
	}
	std::vector<NSRect> rects = update_rects;
	update_rects.clear();
	update_requested = false;
	rect_lock.Unlock();
	
	if (!visible)
		return;
	
	NSEventWindowDraw* event = NSEventWindowDraw::Create(getpid(), window_id, NSRectConsolidate(rects));
	if (!event)
		return;
	
	NSApplication::SendEvent(event);
	delete event;
}

void NSWindow::MouseDown(NSEventMouse* event) {
	if (!content_view)
		return;
	
	NSView* view = content_view->GetViewAtPoint(event->GetPosition());
	if (!view)
		return;
	
	down_view = view;
	MakeFirstResponder(down_view);
	content_view->MouseDown(event);
}

void NSWindow::MouseDragged(NSEventMouse* event) {
	if (!down_view)
		return;
	
	CheckCursorRegions(event->GetPosition());
	
	down_view->MouseDragged(event);
}

void NSWindow::MouseUp(NSEventMouse* event) {
	if (!down_view)
		return;
	
	content_view->MouseUp(event);
	down_view = NULL;
}

void NSWindow::MouseMoved(NSEventMouse* event) {
	if (!content_view)
		return;
	
	CheckCursorRegions(event->GetPosition());
	
	content_view->MouseMoved(event);
}

void NSWindow::MouseScrolled(NSEventMouse* event) {
	if (!content_view)
		return;
	
	content_view->MouseScrolled(event);
};

void NSWindow::KeyDown(NSEventKey* event) {
	if (!content_view)
		return;
	
	content_view->KeyDown(event);
}

void NSWindow::KeyUp(NSEventKey* event) {
	if (!content_view)
		return;
	
	content_view->KeyUp(event);
}

void NSWindow::AddObserver(NSWindowObserver* observer) {
	observers.push_back(observer);
}

void NSWindow::RemoveObserver(NSWindowObserver* observer) {
	for (unsigned int z = 0; z < observers.size(); z++) {
		if (observers[z] == observer) {
			observers.erase(observers.begin() + z);
			break;
		}
	}
}
