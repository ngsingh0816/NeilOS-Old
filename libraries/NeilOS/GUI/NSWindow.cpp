//
//  NSWindow.cpp
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSWindow.h"

#include <fcntl.h>
#include <graphics/graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../Core/NSApplication.h"
#include "../Core/NSEventDefs.cpp"
#include "../Core/NSLock.h"
#include "NSView.h"

using std::string;

static uint32_t win_id = 0;
static NSLock win_id_lock;

namespace NSApplication {
	void RegisterWindow(NSWindow* window);
	void DeregisterWindow(NSWindow* window);
}

NSWindow* NSWindow::Create(string t, NSRect r) {
	NSWindow* ret = new NSWindow(t, r);
	
	uint32_t pid = getpid();
	NSEvent* event = new NSEventWindowCreate(ret->title, ret->frame, pid, ret->window_id, &ret->context);
	NSApplication::SendEvent(event);
	delete event;
	
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
	context = graphics_context_create(f.size.width * psf, (f.size.height - WINDOW_TITLE_BAR_HEIGHT) * psf, 32, 16, 0);
	SetupProjMatrix();
	NSMatrix iden = NSMatrix::Identity();
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_WORLD, iden);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, iden);
	
	content_view = NSView::Create(NSRect(0, 0, f.size.width, f.size.height - WINDOW_TITLE_BAR_HEIGHT));
	content_view->window = this;
	MakeFirstResponder(content_view);
}

NSWindow::~NSWindow() {
	NSApplication::DeregisterWindow(this);
	
	uint32_t pid = getpid();
	NSEvent* event = new NSEventWindowDestroy(pid, window_id);
	NSApplication::SendEvent(event);
	delete event;
	
	delete content_view;
	
	graphics_context_destroy(&context);
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

uint32_t NSWindow::GetWindowID() const {
	return window_id;
}

string NSWindow::GetTitle() const {
	return title;
}

void NSWindow::SetTitle(string t) {
	title = t;
	
	uint32_t pid = getpid();
	NSEvent* event = new NSEventWindowSetTitle(pid, window_id, title);
	NSApplication::SendEvent(event);
	delete event;
}

NSRect NSWindow::GetFrame() const {
	return frame;
}

void NSWindow::SetFrame(NSRect rect) {
	frame = rect;
	
	graphics_context_resize(&context, frame.size.width, frame.size.height - WINDOW_TITLE_BAR_HEIGHT);
	SetupProjMatrix();
	
	content_view->SetFrame(NSRect(0, 0, frame.size.width, frame.size.height - WINDOW_TITLE_BAR_HEIGHT));
	content_view->DrawRect(content_view->frame);
	
	uint32_t pid = getpid();
	NSEvent* event = new NSEventWindowSetFrame(pid, window_id, frame);
	NSApplication::SendEvent(event);
	delete event;
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
	
	content_view->DrawRect(content_view->frame);
	
	uint32_t pid = getpid();
	NSEvent* event = new NSEventWindowShow(pid, window_id, true);
	NSApplication::SendEvent(event);
	delete event;
}

bool NSWindow::IsVisible() {
	return visible;
}

void NSWindow::Close() {
	uint32_t pid = getpid();
	NSEvent* event = new NSEventWindowShow(pid, window_id, false);
	NSApplication::SendEvent(event);
	delete event;
	
	visible = false;
	if (dealloc)
		delete this;
}

bool NSWindow::IsKeyWindow() const {
	return false;
}

void NSWindow::MakeKeyWindow() {
	uint32_t pid = getpid();
	NSEvent* event = new NSEventWindowMakeKey(pid, window_id);
	NSApplication::SendEvent(event);
	delete event;
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
		}).Post(NSThread::MainThread(), 1 / 60.0f, 0, false);
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
