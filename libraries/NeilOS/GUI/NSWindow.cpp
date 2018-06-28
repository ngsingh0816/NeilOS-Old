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

void NSViewContainer::MakeFirstResponder(NSView* view) {
	if (view && !view->AcceptsFirstResponder())
		return;
	
	if (first_responder)
		first_responder->ResignFirstResponder();
	
	first_responder = view;
	if (first_responder)
		first_responder->BecomeFirstResponder();
}

float NSViewContainer::GetBackingScaleFactor() const {
	return NSApplication::GetPixelScalingFactor();
}

NSWindow* NSWindow::Create(string t, const NSRect& r) {
	NSWindow* ret = new NSWindow(t, r);
	
	auto event = NSEventWindowCreate(ret->title, ret->frame, getpid(), ret->window_id, &ret->context);
	NSApplication::SendEvent(&event);
	
	return ret;
}

NSWindow::NSWindow(string t, const NSRect& f) {
	title = t;
	frame = f;
	win_id_lock.Lock();
	window_id = ++win_id;
	win_id_lock.Unlock();
	
	NSApplication::RegisterWindow(this);
	
	float psf = NSApplication::GetPixelScalingFactor();
	bsf = psf;
	NSSize size = NSSize(f.size.width * psf, (f.size.height - WINDOW_TITLE_BAR_HEIGHT) * psf);
	context = graphics_context_create(size.width, size.height, 24, 15, 1, 0);
	NSView::SetupContext(&context, NSSize(f.size.width, (f.size.height - WINDOW_TITLE_BAR_HEIGHT)));
	
	content_view = NSView::Create(NSRect(0, 0, f.size.width, f.size.height - WINDOW_TITLE_BAR_HEIGHT));
	content_view->SetViewContainer(this);
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
	
	first_responder = NULL;
	if (content_view)
		delete content_view;
	content_view = view;
	content_view->SetViewContainer(this);
	content_view->SetFrame(NSRect(NSPoint(), GetContentFrame().size));
	MakeFirstResponder(content_view);
	content_view->SetNeedsDisplay();
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

NSPoint NSWindow::GetOffset() const {
	return GetContentFrame().origin;
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
		AddUpdateRects({ content_view->GetFrame() });
	}
	
	for (auto& i : observers)
		i->SetFrame(this);
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

uint32_t NSWindow::GetButtonMask() const {
	return button_mask;
}

void NSWindow::SetButtonMask(uint32_t mask) {
	button_mask = mask;
	
	auto event = NSEventWindowSetButtonMask(getpid(), window_id, mask);
	NSApplication::SendEvent(&event);
}

void NSWindow::SetDeallocsWhenClose(bool close) {
	dealloc = close;
}

bool NSWindow::GetDeallocsWhenClose() const {
	return dealloc;
}


void NSWindow::Show(bool animates) {
	if (visible)
		return;
	
	visible = true;
	is_key = true;
	
	content_view->SetNeedsDisplay();
	
	auto event = NSEventWindowShow(getpid(), window_id, true, animates);
	NSApplication::SendEvent(&event);
	
	for (auto& i : observers)
		i->Show(this);
}

bool NSWindow::IsVisible() {
	return visible;
}

void NSWindow::Close() {
	auto event = NSEventWindowShow(getpid(), window_id, false, false);
	NSApplication::SendEvent(&event);
	
	visible = false;
	is_key = false;
		
	for (auto& i : observers)
		i->Close(this);
	
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
			i->BecomeKeyWindow(this);
	} else {
		is_key = false;
		
		for (auto& i : observers)
			i->ResignKeyWindow(this);
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

void NSWindow::MakeFirstResponder(NSView* view) {
	if (view && !view->AcceptsFirstResponder())
		return;
	
	if (first_responder)
		first_responder->ResignFirstResponder();
	
	first_responder = view ? view : content_view;
	view->BecomeFirstResponder();
}

void NSWindow::AddUpdateRects(const std::vector<NSRect>& rects) {
	std::vector<NSRect> real;
	real.reserve(rects.size());
	for (unsigned int z = 0; z < rects.size(); z++) {
		NSRect rect = rects[z];
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
	
	MakeFirstResponder(view);
	content_view->MouseDown(event);
}

void NSWindow::MouseDragged(NSEventMouse* event) {
	CheckCursorRegions(event->GetPosition());
	
	content_view->MouseDragged(event);
}

void NSWindow::MouseUp(NSEventMouse* event) {
	content_view->MouseUp(event);
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
