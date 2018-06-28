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

NSHandler::NSHandler(const std::function<void(NSThread*)>& function) {
	func = function;
}

void NSHandler::SetFunction(const std::function<void(NSThread*)>& function) {
	func = function;
}

const std::function<void(NSThread*)>& NSHandler::GetFunction() const {
	return func;
}

void NSHandler::Cancel() {
	canceled = true;
	if (event)
		event->Cancel();
}

bool NSHandler::IsCanceled() {
	return canceled;
}

// Posts to the main thread (asynchronous by default)
void NSHandler::Post() {
	Post(0);
}

void NSHandler::Post(uint32_t priority) {
	Post(NSThread::MainThread(), priority);
}

void NSHandler::Post(NSThread* thread, uint32_t priority) {
	Post(thread, priority, false);
}

void NSHandler::Post(NSThread* thread, uint32_t priority, bool sync) {
	if (canceled)
		return;
	
	event = NSEventFunction::Create([f = this->func, thread] {
		f(thread);
	}, priority);
	if (sync) {
		NSLock lock;
		NSConditionalLock cond;
		volatile bool done = false;
		
		thread->GetRunLoop()->AddEvent(event,
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
	} else {
		thread->GetRunLoop()->AddEvent(event);
	}
}

void NSHandler::Post(NSThread* thread, NSTimeInterval delay, uint32_t priority, bool sync) {
	if (canceled)
		return;
	
	// Has own lifetime
	event = NSEventFunction::Create([f = this->func, thread]() {
		f(thread);
	}, priority);
	if (sync) {
		NSLock lock;
		NSConditionalLock cond;
		volatile bool done = false;
		
		thread->GetRunLoop()->AddTimer(NSTimer::Create([e = event, thread, &lock, &cond, &done](NSTimer*) {
			thread->GetRunLoop()->AddEvent(e, [&lock, &cond, &done]() {
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
		thread->GetRunLoop()->AddTimer(NSTimer::Create([e = event, thread](NSTimer*) {
			thread->GetRunLoop()->AddEvent(e);
		}, delay, false, false));
	}
}
