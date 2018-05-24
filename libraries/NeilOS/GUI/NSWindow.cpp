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
#include "../Core/NSLock.h"
#include "NSView.h"

using std::string;

static uint32_t win_id = 0;
static NSLock win_id_lock;

#define WINDOW_EVENT_ID					1

#define WINDOW_EVENT_CREATE_ID			0
#define WINDOW_EVENT_DESTROY_ID			1
#define WINDOW_EVENT_SHOW_ID			2
#define WINDOW_EVENT_MAKE_KEY_ID		3
#define WINDOW_EVENT_SET_TITLE_ID		4
#define WINDOW_EVENT_SET_FRAME_ID		5

#define copy(x, len) { memcpy(&buffer[pos], x, (len)); pos += (len); }

// Window events
class NSEventWindowCreate : public NSEvent {
public:
	NSEventWindowCreate(string t, NSRect f, uint32_t p, uint32_t win_id, graphics_context_t* c) {
		title = t;
		frame = f;
		pid = p;
		window_id = win_id;
		context = *c;
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// title.length()
		total_length += title.length();
		total_length += sizeof(NSRect);		// frame
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// window_id
		total_length += sizeof(graphics_context_t);
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = WINDOW_EVENT_ID;
		uint32_t event_sub_id = WINDOW_EVENT_CREATE_ID;
		uint32_t title_len = title.length();
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&title_len, sizeof(uint32_t));
		copy(title.c_str(), title_len);
		copy(&frame, sizeof(NSRect));
		copy(&pid, sizeof(uint32_t));
		copy(&window_id, sizeof(uint32_t));
		copy(&context, sizeof(graphics_context_t));
		
		*length_out = total_length;
		return buffer;
	}
private:
	string title;
	NSRect frame;
	uint32_t pid;
	uint32_t window_id;
	graphics_context_t context;
};

class NSEventWindowShow : public NSEvent {
public:
	NSEventWindowShow(uint32_t p, uint32_t id, bool s) {
		pid = p;
		window_id = id;
		show = s;
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// window_id
		total_length += sizeof(bool);		// show;
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = WINDOW_EVENT_ID;
		uint32_t event_sub_id = WINDOW_EVENT_SHOW_ID;
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&pid, sizeof(uint32_t));
		copy(&window_id, sizeof(uint32_t));
		copy(&show, sizeof(bool));
		
		*length_out = total_length;
		return buffer;
	}
private:
	uint32_t pid;
	uint32_t window_id;
	bool show;
};

class NSEventWindowDestroy : public NSEvent {
public:
	NSEventWindowDestroy(uint32_t p, uint32_t id) {
		pid = p;
		window_id = id;
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// window_id
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = WINDOW_EVENT_ID;
		uint32_t event_sub_id = WINDOW_EVENT_DESTROY_ID;
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&pid, sizeof(uint32_t));
		copy(&window_id, sizeof(uint32_t));
		
		*length_out = total_length;
		return buffer;
	}
private:
	uint32_t pid;
	uint32_t window_id;
};

class NSEventWindowMakeKey : public NSEvent {
public:
	NSEventWindowMakeKey(uint32_t p, uint32_t id) {
		pid = p;
		window_id = id;
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// window_id
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = WINDOW_EVENT_ID;
		uint32_t event_sub_id = WINDOW_EVENT_MAKE_KEY_ID;
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&pid, sizeof(uint32_t));
		copy(&window_id, sizeof(uint32_t));
		
		*length_out = total_length;
		return buffer;
	}
private:
	uint32_t pid;
	uint32_t window_id;
};

class NSEventWindowSetTitle : public NSEvent {
public:
	NSEventWindowSetTitle(uint32_t p, uint32_t id, string t) {
		pid = p;
		window_id = id;
		title = t;
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// window_id
		total_length += sizeof(uint32_t);	// title length
		total_length += title.length();
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = WINDOW_EVENT_ID;
		uint32_t event_sub_id = WINDOW_EVENT_SET_TITLE_ID;
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&pid, sizeof(uint32_t));
		copy(&window_id, sizeof(uint32_t));
		uint32_t title_len = title.length();
		copy(&title_len, sizeof(uint32_t));
		copy(title.c_str(), title_len);
		
		*length_out = total_length;
		return buffer;
	}
private:
	uint32_t pid;
	uint32_t window_id;
	string title;
};

class NSEventWindowSetFrame : public NSEvent {
public:
	NSEventWindowSetFrame(uint32_t p, uint32_t id, NSRect f) {
		pid = p;
		window_id = id;
		frame = f;
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// window_id
		total_length += sizeof(NSRect);		// frame
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = WINDOW_EVENT_ID;
		uint32_t event_sub_id = WINDOW_EVENT_SET_FRAME_ID;
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&pid, sizeof(uint32_t));
		copy(&window_id, sizeof(uint32_t));
		copy(&frame, sizeof(NSRect));
		
		*length_out = total_length;
		return buffer;
	}
private:
	uint32_t pid;
	uint32_t window_id;
	NSRect frame;
};

#undef copy

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
	
	context = graphics_context_create(f.size.width, f.size.height - WINDOW_TITLE_BAR_HEIGHT, 32, 16, 0);
	SetupProjMatrix();
	NSMatrix iden = NSMatrix::Identity();
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_WORLD, iden);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, iden);
	
	content_view = NSView::Create(NSRect(0, 0, f.size.width, f.size.height - WINDOW_TITLE_BAR_HEIGHT));
	content_view->window = this;
	MakeFirstResponder(content_view);
}

NSWindow::~NSWindow() {
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
