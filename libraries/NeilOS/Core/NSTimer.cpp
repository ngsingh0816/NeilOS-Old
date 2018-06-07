//
//  NSTimer.cpp
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSTimer.h"
#include "NSThread.h"

NSTimer* NSTimer::Create() {
	return new NSTimer();
}

NSTimer* NSTimer::Create(const std::function<void(NSTimer*)>& func, NSTimeInterval i, bool r, bool s) {
	return new NSTimer(func, i, r, s);
}

NSTimer::NSTimer() {
	invalidated = false;
	repeats = false;
	interval = 0.0;
}

// If schedule==true, there is no need to call NSRunLoop::AddTimer
NSTimer::NSTimer(const std::function<void(NSTimer*)>& func, NSTimeInterval i, bool r, bool schedule) {
	invalidated = false;
	
	function = func;
	interval = i;
	repeats = r;
	if (schedule)
		NSThread::CurrentThread()->GetRunLoop()->AddTimer(this);
}

NSTimer::~NSTimer() {
}

void NSTimer::SetInterval(NSTimeInterval i) {
	interval = i;
}

NSTimeInterval NSTimer::GetInterval() const {
	return interval;
}

void NSTimer::SetRepeats(bool r) {
	repeats = r;
}

bool NSTimer::GetRepeats() const {
	return repeats;
}

void NSTimer::SetFunction(const std::function<void(NSTimer*)>& func) {
	function = func;
}

std::function<void(NSTimer*)> NSTimer::GetFunction() const {
	return function;
}

// Cancels a repeating timer
void NSTimer::Invalidate() {
	invalidated = true;
}

bool NSTimer::IsInvalidated() {
	return invalidated;
}
