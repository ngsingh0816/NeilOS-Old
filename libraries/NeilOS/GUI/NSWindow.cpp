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
	NSSize size = NSSize(f.size.width * psf, (f.size.height - WINDOW_TITLE_BAR_HEIGHT) * psf);
	context = graphics_context_create(size.width, size.height, 32, 16, 0);
	SetupProjMatrix();
	NSMatrix iden = NSMatrix::Identity();
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_WORLD, iden);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, iden);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_BLENDENABLE, true);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_SRCBLEND, GRAPHICS_BLENDOP_SRCALPHA);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_DSTBLEND, GRAPHICS_BLENDOP_INVSRCALPHA);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_BLENDEQUATION, GRAPHICS_BLENDEQ_ADD);
	//graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_MULTISAMPLEANTIALIAS, true);
	graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_MINFILTER, GRAPHICS_TEXTURE_FILTER_LINEAR);
	graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_MAGFILTER, GRAPHICS_TEXTURE_FILTER_LINEAR);
	graphics_clear(&context, 0x0, 1.0f, 0, GRAPHICS_CLEAR_COLOR, 0, 0, size.width, size.height);
	
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
	
	for (auto& i : observers)
		i->SetFrame();
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
	
	content_view->DrawRect(NSRect(NSPoint(), content_view->frame.size));
	
	uint32_t pid = getpid();
	NSEvent* event = new NSEventWindowShow(pid, window_id, true);
	NSApplication::SendEvent(event);
	delete event;
	
	for (auto& i : observers)
		i->Show();
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
	
	for (auto& i : observers)
		i->Close();
	
	if (dealloc)
		delete this;
}

bool NSWindow::IsKeyWindow() const {
	return false;
}

void NSWindow::MakeKeyWindow() {
	uint32_t pid = getpid();
	NSEvent* event = new NSEventWindowMakeKey(pid, window_id, true);
	NSApplication::SendEvent(event);
	delete event;
	
	for (auto& i : observers)
		i->BecomeKeyWindow();
}

void NSWindow::ResignKeyWindow() {
	uint32_t pid = getpid();
	NSEvent* event = new NSEventWindowMakeKey(pid, window_id, false);
	NSApplication::SendEvent(event);
	delete event;
	
	for (auto& i : observers)
		i->ResignKeyWindow();
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

void NSWindow::MouseDown(NSEvent* event) {
	if (!content_view)
		return;
	
	NSEventMouse* e = dynamic_cast<NSEventMouse*>(event);
	
	NSView* view = content_view->GetViewAtPoint(e->GetPosition());
	if (!view)
		return;
	
	down_view = view;
	view->MouseDown(event);
}

void NSWindow::MouseDragged(NSEvent* event) {
	if (!down_view)
		return;
	
	down_view->MouseDragged(event);
}

void NSWindow::MouseUp(NSEvent* event) {
	if (!down_view)
		return;
	
	down_view->MouseUp(event);
	down_view = NULL;
}

void NSWindow::MouseMoved(NSEvent* event) {
	if (!content_view)
		return;
	
	NSEventMouse* e = dynamic_cast<NSEventMouse*>(event);
	
	NSView* view = content_view->GetViewAtPoint(e->GetPosition());
	if (!view)
		return;
	
	view->MouseMoved(event);
}

void NSWindow::MouseScrolled(NSEvent* event) {
	if (!content_view)
		return;
	
	NSEventMouse* e = dynamic_cast<NSEventMouse*>(event);
	
	NSView* view = content_view->GetViewAtPoint(e->GetPosition());
	if (!view)
		return;
	
	view->MouseScrolled(event);
};

void NSWindow::KeyDown(NSEvent* event) {
	if (first_responder)
		first_responder->KeyDown(event);
}

void NSWindow::KeyUp(NSEvent* event) {
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
