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
	context = graphics_context_create(size.width, size.height, 32, 16, 0);
	SetupContext();
	
	content_view = NSView::Create(NSRect(0, 0, f.size.width, f.size.height - WINDOW_TITLE_BAR_HEIGHT));
	content_view->window = this;
	MakeFirstResponder(content_view);
}

NSWindow::~NSWindow() {
	NSApplication::DeregisterWindow(this);
	
	auto event = NSEventWindowDestroy(getpid(), window_id);
	NSApplication::SendEvent(&event);
	
	delete content_view;
	
	graphics_context_destroy(&context);
}

void NSWindow::SetupContext() {
	float psf = NSApplication::GetPixelScalingFactor();
	bsf = psf;
	NSSize size = NSSize(frame.size.width * psf, (frame.size.height - WINDOW_TITLE_BAR_HEIGHT) * psf);
	SetupProjMatrix();
	NSMatrix iden = NSMatrix::Identity();
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_WORLD, iden);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, iden);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_BLENDENABLE, true);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_SRCBLEND, GRAPHICS_BLENDOP_SRCALPHA);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_DSTBLEND, GRAPHICS_BLENDOP_INVSRCALPHA);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_BLENDEQUATION, GRAPHICS_BLENDEQ_ADD);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_SCISSORTESTENABLE, true);
	//graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_MULTISAMPLEANTIALIAS, true);
	graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_MINFILTER, GRAPHICS_TEXTURE_FILTER_LINEAR);
	graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_MAGFILTER, GRAPHICS_TEXTURE_FILTER_LINEAR);
	graphics_clear(&context, 0x0, 1.0f, 0, GRAPHICS_CLEAR_COLOR, 0, 0, size.width, size.height);
}

void NSWindow::SetupProjMatrix() {
	NSMatrix proj = NSMatrix::Ortho2D(0, frame.size.width, frame.size.height - WINDOW_TITLE_BAR_HEIGHT, 0);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_PROJECTION, proj);
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

void NSWindow::SetFrameInternal(NSRect rect) {
	NSSize old_size = frame.size;
	frame = rect;
	
	if (old_size != rect.size) {
		graphics_context_resize(&context, frame.size.width * bsf, (frame.size.height - WINDOW_TITLE_BAR_HEIGHT) * bsf);
		SetupProjMatrix();
		
		enable_updates = false;
		content_view->SetFrame(NSRect(0, 0, frame.size.width, frame.size.height - WINDOW_TITLE_BAR_HEIGHT));
		enable_updates = true;
		AddUpdateRect(content_view->GetFrame());
	}
	
	for (auto& i : observers)
		i->SetFrame();
}

void NSWindow::SetFrame(NSRect rect) {
	SetFrameInternal(rect);
	
	auto event = NSEventWindowSetFrame(getpid(), window_id, frame);
	NSApplication::SendEvent(&event);
}

void NSWindow::SetFrameOrigin(NSPoint p) {
	SetFrame(NSRect(p, frame.size));
}

void NSWindow::SetFrameSize(NSSize size) {
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

void NSWindow::AddCursorRegion(const NSCursorRegion& region) {
	cursor_regions.push_back(region);
}

// Removes all regions that overlap the rect
void NSWindow::RemoveCursorRegion(const NSRect& region) {
	for (unsigned int z = 0; z < cursor_regions.size(); z++) {
		if (region.OverlapsRect(cursor_regions[z].GetRegion())) {
			cursor_regions.erase(cursor_regions.begin() + z);
			z--;
		}
	}
}

void NSWindow::RemoveAllCursorRegions() {
	cursor_regions.clear();
}

uint32_t NSWindow::GetNumberOfCursorRegions() {
	return cursor_regions.size();
}

NSCursorRegion NSWindow::GetCursorRegionAtIndex(uint32_t index) {
	return cursor_regions[index];
}

void NSWindow::CheckCursorRegions(const NSPoint& p) {
	bool found = false;
	for (unsigned int z = 0; z < cursor_regions.size(); z++) {
		NSCursorRegion& region = cursor_regions[z];
		if (region.GetRegion().ContainsPoint(p)) {
			found = true;
			if (region.IsActive())
				continue;
			
			NSImage* image = region.GetImage();
			if (image)
				NSCursor::SetCursor(image, region.GetHotspot());
			else
				NSCursor::SetCursor(region.GetCursor());
			current_cursor = region.GetCursor();
			region.SetIsActive(true);
		} else
			region.SetIsActive(false);
	}
	if (!found && current_cursor != NSCursor::CURSOR_DEFAULT) {
		NSCursor::SetCursor(NSCursor::CURSOR_DEFAULT);
		current_cursor = NSCursor::CURSOR_DEFAULT;
	}
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

void NSWindow::AddUpdateRect(NSRect rect) {
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
	
	NSEventWindowDraw* event = NSEventWindowDraw::Create(getpid(), window_id, rects);
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
	view->MouseDown(event);
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
	
	down_view->MouseUp(event);
	down_view = NULL;
}

void NSWindow::MouseMoved(NSEventMouse* event) {
	if (!content_view)
		return;
	
	CheckCursorRegions(event->GetPosition());
	
	std::function<void(NSView*)> recurse = [&recurse, event](NSView* view) {
		if (!view->visible)
			return;
		view->MouseMoved(event);
		for (unsigned int z = 0; z < view->subviews.size(); z++)
			recurse(view->subviews[z]);
	};
	recurse(content_view);
}

void NSWindow::MouseScrolled(NSEventMouse* event) {
	if (!content_view)
		return;
	
	NSView* view = content_view->GetViewAtPoint(event->GetPosition());
	if (!view)
		return;
	
	view->MouseScrolled(event);
};

void NSWindow::KeyDown(NSEventKey* event) {
	if (first_responder)
		first_responder->KeyDown(event);
}

void NSWindow::KeyUp(NSEventKey* event) {
	if (first_responder)
		first_responder->KeyDown(event);
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
