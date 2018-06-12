//
//  NSResponder.cpp
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSResponder.h"

#include "NSApplication.h"
#include "Events/NSEventDefs.cpp"
#include "../GUI/NSWindow.h"

#include <string.h>

namespace NSApplication {
	NSWindow* GetWindow(uint32_t window_id);
}

#define copy(x, len) { memcpy(&buffer[pos], x, (len)); pos += (len); }

NSEventMouse* NSEventMouse::Create(const NSPoint& position, NSMouseType type, NSMouseButton button,
								   uint32_t window_id, uint32_t priority) {
	return new NSEventMouse(position, type, button, window_id, priority);
}

NSEventMouse* NSEventMouse::FromData(uint8_t* data, uint32_t length) {
	if (length != sizeof(uint32_t) + sizeof(float) * 2 + sizeof(NSMouseType) + sizeof(NSMouseButton)
		+ sizeof(uint32_t) + sizeof(float) * 2)
		return NULL;
	uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
	if (buffer[0] != EVENT_MOUSE_ID)
		return NULL;
	
	uint32_t pos = sizeof(uint32_t);
	NSPoint position = NSPoint::FromData(&data[pos], sizeof(float) * 2);
	pos += sizeof(float) * 2;
	NSMouseType type;
	memcpy(&type, &data[pos], sizeof(NSMouseType));
	pos += sizeof(NSMouseType);
	NSMouseButton button;
	memcpy(&button, &data[pos], sizeof(NSMouseButton));
	pos += sizeof(NSMouseButton);
	uint32_t window_id;
	memcpy(&window_id, &data[pos], sizeof(uint32_t));
	pos += sizeof(uint32_t);
	float dx, dy;
	memcpy(&dx, &data[pos], sizeof(float));
	pos += sizeof(float);
	memcpy(&dy, &data[pos], sizeof(float));

	NSEventMouse* ret = Create(position, type, button, window_id);
	ret->delta_x = dx;
	ret->delta_y = dy;
	
	return ret;
}

NSPoint NSEventMouse::GetPosition() const {
	return position;
}

void NSEventMouse::SetPosition(const NSPoint& p) {
	position = p;
}

NSMouseType NSEventMouse::GetType() const {
	return type;
}

void NSEventMouse::SetType(NSMouseType t) {
	type = t;
}

NSMouseButton NSEventMouse::GetButton() const {
	return button;
}

void NSEventMouse::SetButton(NSMouseButton b) {
	button = b;
}

uint32_t NSEventMouse::GetWindowID() const {
	return window_id;
}

void NSEventMouse::SetWindowID(uint32_t wid) {
	window_id = wid;
}

float NSEventMouse::GetDeltaX() const {
	return delta_x;
}

void NSEventMouse::SetDeltaX(float dx) {
	delta_x = dx;
}

float NSEventMouse::GetDeltaY() const {
	return delta_y;
}

void NSEventMouse::SetDeltaY(float dy) {
	delta_y = dy;
}

void NSEventMouse::Process() {
	if (window_id == (unsigned int)-1)
		return;
	
	NSWindow* window = NSApplication::GetWindow(window_id);
	if (!window)
		return;
	
	switch (type) {
		case NSMouseTypeDown:
			window->MouseDown(this);
			break;
		case NSMouseTypeMoved:
			window->MouseMoved(this);
			break;
		case NSMouseTypeDragged:
			window->MouseDragged(this);
			break;
		case NSMouseTypeUp:
			window->MouseUp(this);
			break;
		case NSMouseTypeScrolled:
			window->MouseScrolled(this);
			break;
		default:
			break;
	}
}

uint8_t* NSEventMouse::Serialize(uint32_t* length_out) const {
	uint32_t total_length = 0;
	total_length += sizeof(uint32_t);// event id
	total_length += sizeof(float) * 2;
	total_length += sizeof(NSMouseType);
	total_length += sizeof(NSMouseButton);
	total_length += sizeof(uint32_t);
	total_length += sizeof(float) * 2;	// dx, dy
	uint8_t* buffer = new uint8_t[total_length];
	uint32_t pos = 0;
	
	uint32_t event_id = EVENT_MOUSE_ID;
	copy(&event_id, sizeof(uint32_t));
	uint8_t* buf = position.Serialize(NULL);
	copy(buf, sizeof(float) * 2);
	delete[] buf;
	copy(&type, sizeof(NSMouseType));
	copy(&button, sizeof(NSMouseButton));
	copy(&window_id, sizeof(uint32_t));
	copy(&delta_x, sizeof(float));
	copy(&delta_y, sizeof(float));
	
	*length_out = total_length;
	
	return buffer;
}

NSEventMouse::NSEventMouse(const NSPoint& p, NSMouseType t, NSMouseButton b, uint32_t wid, uint32_t pr) {
	position = p;
	type = t;
	button = b;
	window_id = wid;
	SetPriority(pr);
}

NSEventKey* NSEventKey::Create(unsigned char key, bool down, NSModifierFlags flags, uint32_t window_id) {
	return new NSEventKey(key, down, flags, window_id);
}

NSEventKey* NSEventKey::FromData(uint8_t* data, uint32_t length) {
	if (length != sizeof(uint32_t) + sizeof(unsigned char) + sizeof(bool) + sizeof(NSModifierFlags) + sizeof(uint32_t))
		return NULL;
	uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
	if (buffer[0] != EVENT_KEY_ID)
		return NULL;
	
	uint32_t pos = sizeof(uint32_t);
	unsigned char key;
	memcpy(&key, &data[pos], sizeof(unsigned char));
	pos += sizeof(unsigned char);
	bool pressed;
	memcpy(&pressed, &data[pos], sizeof(bool));
	pos += sizeof(bool);
	NSModifierFlags flags;
	memcpy(&flags, &data[pos], sizeof(NSModifierFlags));
	pos += sizeof(NSModifierFlags);
	uint32_t window_id;
	memcpy(&window_id, &data[pos], sizeof(uint32_t));
	
	return Create(key, pressed, flags, window_id);
}

unsigned char NSEventKey::GetKey() const {
	return key;
}

void NSEventKey::SetKey(unsigned char k) {
	key = k;
}

bool NSEventKey::GetDown() const {
	return down;
}

void NSEventKey::SetDown(bool d) {
	down = d;
}

NSModifierFlags NSEventKey::GetModifierFlags() const {
	return flags;
}

void NSEventKey::SetModifierFlags(NSModifierFlags f) {
	flags = f;
}

uint32_t NSEventKey::GetWindowID() const {
	return window_id;
}

void NSEventKey::SetWindowID(uint32_t wid) {
	window_id = wid;
}

void NSEventKey::Process() {
	if (window_id == (unsigned int)-1)
		return;
	
	NSWindow* window = NSApplication::GetWindow(window_id);
	if (!window)
		return;
	
	if (down)
		window->KeyDown(this);
	else
		window->KeyUp(this);
}

uint8_t* NSEventKey::Serialize(uint32_t* length_out) const {
	uint32_t total_length = 0;
	total_length += sizeof(uint32_t);// event id
	total_length += sizeof(unsigned char);		// key
	total_length += sizeof(bool);				// pressed
	total_length += sizeof(NSModifierFlags);	// modifier flags
	total_length += sizeof(uint32_t);			// window id
	uint8_t* buffer = new uint8_t[total_length];
	uint32_t pos = 0;
	
	uint32_t event_id = EVENT_KEY_ID;
	copy(&event_id, sizeof(uint32_t));
	copy(&key, sizeof(unsigned char));
	copy(&down, sizeof(bool));
	copy(&flags, sizeof(NSModifierFlags));
	copy(&window_id, sizeof(uint32_t));
	
	*length_out = total_length;
	
	return buffer;
}

NSEventKey::NSEventKey(unsigned char k, bool d, NSModifierFlags f, uint32_t wid) {
	key = k;
	down = d;
	flags = f;
	window_id = wid;
}
