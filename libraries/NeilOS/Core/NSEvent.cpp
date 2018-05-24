//
//  NSEvent.cpp
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSEvent.h"

NSEvent* NSEvent::FromData(uint8_t* data, uint32_t length) {
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
