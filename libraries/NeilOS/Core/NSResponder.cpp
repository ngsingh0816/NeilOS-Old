//
//  NSResponder.cpp
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSResponder.h"

#include <string.h>

#define copy(x, len) { memcpy(&buffer[pos], x, (len)); pos += (len); }

#define MOUSE_EVENT_ID		2

NSEventMouse* NSEventMouse::Create(NSPoint position, NSMouseType type, NSMouseButton button,
								   uint32_t window_id, uint32_t priority) {
	return new NSEventMouse(position, type, button, window_id, priority);
}

NSPoint NSEventMouse::GetPosition() const {
	return position;
}

void NSEventMouse::SetPosition(NSPoint p) {
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

void NSEventMouse::Process() {
	// TODO: implement
}

uint8_t* NSEventMouse::Serialize(uint32_t* length_out) const {
	uint32_t total_length = 0;
	total_length += sizeof(uint32_t);// event id
	total_length += sizeof(NSPoint);
	total_length += sizeof(NSMouseType);
	total_length += sizeof(NSMouseButton);
	total_length += sizeof(uint32_t);
	uint8_t* buffer = new uint8_t[total_length];
	uint32_t pos = 0;
	
	uint32_t event_id = MOUSE_EVENT_ID;
	copy(&event_id, sizeof(uint32_t));
	copy(&position, sizeof(NSPoint));
	copy(&type, sizeof(NSMouseType));
	copy(&button, sizeof(NSMouseButton));
	copy(&window_id, sizeof(uint32_t));
	
	*length_out = total_length;
	
	return buffer;
}

NSEventMouse::NSEventMouse(NSPoint p, NSMouseType t, NSMouseButton b, uint32_t wid, uint32_t pr) {
	position = p;
	type = t;
	button = b;
	window_id = wid;
	SetPriority(pr);
}
