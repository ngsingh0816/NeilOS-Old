//
//  NSHandler.cpp
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSHandler.h"
#include "NSLock.h"
#include "NSTimer.h"

NSHandler::NSHandler() {
}

NSHandler::NSHandler(std::function<void(NSThread*)> function) {
	func = function;
}

void NSHandler::SetFunction(std::function<void(NSThread*)> function) {
	func = function;
}

std::function<void(NSThread*)> NSHandler::GetFunction() const {
	return func;
}

// Posts to the main thread (asynchronous by default)
void NSHandler::Post() const {
	Post(0);
}

void NSHandler::Post(uint32_t priority) const {
	Post(NSThread::MainThread(), priority);
}

void NSHandler::Post(NSThread* thread, uint32_t priority) const {
	Post(thread, priority, false);
}

void NSHandler::Post(NSThread* thread, uint32_t priority, bool sync) const {
	if (sync) {
		NSLock lock;
		NSConditionalLock cond;
		volatile bool done = false;
		
		thread->GetRunLoop()->AddEvent(NSEventFunction::Create(std::bind(func, thread), priority),
									   [&lock, &cond, &done]() {
										   lock.Lock();
										   done = true;
										   cond.Signal();
										   lock.Unlock();
									   });
		lock.Lock();
		for (;;) {
			if (done)
				break;
			cond.Wait(&lock);
		}
		lock.Unlock();
	} else
		thread->GetRunLoop()->AddEvent(NSEventFunction::Create(std::bind(func, thread), priority));
}

void NSHandler::Post(NSThread* thread, NSTimeInterval delay, uint32_t priority, bool sync) {
	NSHandler copy = *this;
	if (sync) {
		NSLock lock;
		NSConditionalLock cond;
		volatile bool done = false;
				
		thread->GetRunLoop()->AddTimer(NSTimer::Create([copy, thread, priority, sync,
														&lock, &cond, &done](NSTimer*) {
			thread->GetRunLoop()->AddEvent(NSEventFunction::Create(std::bind(copy.func, thread), priority),
										   [&lock, &cond, &done]() {
											   lock.Lock();
											   done = true;
											   cond.Signal();
											   lock.Unlock();
										   });
		}, delay, false, false));
		
		lock.Lock();
		for (;;) {
			if (done)
				break;
			cond.Wait(&lock);
		}
		lock.Unlock();
	} else {
		thread->GetRunLoop()->AddTimer(NSTimer::Create([copy, thread, priority, sync](NSTimer*) {
			copy.Post(thread, priority, false);
		}, delay, false, false));
	}
}
