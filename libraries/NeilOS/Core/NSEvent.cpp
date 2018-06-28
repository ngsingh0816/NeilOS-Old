//
//  NSEvent.cpp
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSEvent.h"
#include "Events/NSEventDefs.cpp"

std::function<void()> NSEventInitResp::function;

NSEvent* NSEvent::FromData(const uint8_t* data, uint32_t length) {
	if (length < sizeof(uint32_t))
		return NULL;
	const uint32_t* buffer = reinterpret_cast<const uint32_t*>(data);
	uint32_t code = buffer[0];
	uint32_t sub_code = 0;
	if (length >= sizeof(uint32_t) * 2)
		sub_code = buffer[1];
	switch (code) {
		case EVENT_WINDOW_ID:
			switch (sub_code) {
				case WINDOW_EVENT_CREATE_ID:
					return NSEventWindowCreate::FromData(data, length);
				case WINDOW_EVENT_DESTROY_ID:
					return NSEventWindowDestroy::FromData(data, length);
				case WINDOW_EVENT_SHOW_ID:
					return NSEventWindowShow::FromData(data, length);
				case WINDOW_EVENT_MAKE_KEY_ID:
					return NSEventWindowMakeKey::FromData(data, length);
				case WINDOW_EVENT_SET_TITLE_ID:
					return NSEventWindowSetTitle::FromData(data, length);
				case WINDOW_EVENT_SET_FRAME_ID:
					return NSEventWindowSetFrame::FromData(data, length);
				case WINDOW_EVENT_SET_BUTTON_MASK_ID:
					return NSEventWindowSetButtonMask::FromData(data, length);
				case WINDOW_EVENT_DRAW_ID:
					return NSEventWindowDraw::FromData(data, length);
			}
			break;
		case EVENT_MOUSE_ID:
			return NSEventMouse::FromData(data, length);
		case EVENT_KEY_ID:
			return NSEventKey::FromData(data, length);
		case EVENT_INIT_ID:
			if (sub_code == INIT_EVENT_RESP)
				return NSEventInitResp::FromData(data, length);
			break;
		case EVENT_SETTINGS_ID:
			if (sub_code == SETTINGS_EVENT_PSF)
				return NSEventSettingsPixelScalingFactor::FromData(data, length);
			break;
		case EVENT_APPLICATION_ID:
			switch (sub_code) {
				case APPLICATION_EVENT_QUIT:
					return NSEventApplicationQuit::FromData(data, length);
				case APPLICATION_EVENT_SET_MENU:
					return NSEventApplicationSetMenu::FromData(data, length);
				case APPLICATION_EVENT_MENU_EVENT:
					return NSEventApplicationMenuEvent::FromData(data, length);
				case APPLICATION_EVENT_FOCUS:
					return NSEventApplicationFocus::FromData(data, length);
				case APPLICATION_EVENT_OPEN_FILE:
					return NSEventApplicationOpenFile::FromData(data, length);
				case APPLICATION_EVENT_SET_CURSOR:
					return NSEventApplicationSetCursor::FromData(data, length);
				case APPLICATION_EVENT_CONTEXT_MENU:
					return NSEventApplicationContextMenu::FromData(data, length);
			}
			break;
		default:
			break;
	}
	return NULL;
}

NSEventFunction* NSEventFunction::Create(const std::function<void()>& func, uint32_t priority) {
	return new NSEventFunction(func, priority);
}

NSEventFunction::NSEventFunction(const std::function<void()>& func, uint32_t priority) {
	function = func;
	SetPriority(priority);
}

void NSEventFunction::Cancel() {
	canceled = true;
}

bool NSEventFunction::IsCanceled() const {
	return canceled;
}

void NSEventFunction::Process() {
	if (!canceled)
		function();
}

uint8_t* NSEventFunction::Serialize(uint32_t* length_out) const {
	return NULL;
}
