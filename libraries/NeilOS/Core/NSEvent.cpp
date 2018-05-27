//
//  NSEvent.cpp
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSEvent.h"
#include "NSEventDefs.cpp"

std::function<void()> NSEventInitResp::function;

NSEvent* NSEvent::FromData(uint8_t* data, uint32_t length) {
	if (length < sizeof(uint32_t))
		return NULL;
	uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
	uint32_t code = buffer[0];
	uint32_t sub_code = 0;
	if (length >= sizeof(uint32_t) * 2)
		sub_code = buffer[1];
	switch (code) {
		case EVENT_WINDOW_ID:
			break;
		case EVENT_MOUSE_ID:
			break;
		case EVENT_KEY_ID:
			break;
		case EVENT_INIT_ID:
			if (sub_code == INIT_EVENT_RESP)
				return NSEventInitResp::FromData(data, length);
			break;
		default:
			break;
	}
	return NULL;
}

NSEventFunction* NSEventFunction::Create(std::function<void()> func, uint32_t priority) {
	return new NSEventFunction(func, priority);
}

NSEventFunction::NSEventFunction(std::function<void()> func, uint32_t priority) {
	function = func;
	SetPriority(priority);
}

void NSEventFunction::Process() {
	function();
}

uint8_t* NSEventFunction::Serialize(uint32_t* length_out) const {
	return NULL;
}
