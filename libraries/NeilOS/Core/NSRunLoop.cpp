//
//  NSRunLoop.cpp
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSRunLoop.h"
#include "NSTimer.h"
#include <sched.h>

using namespace std::chrono;

namespace {
	typedef time_point<system_clock, milliseconds> tp;
}

NSRunLoop::NSRunLoop() {
	stopped = false;
	running = false;
	exits = false;
}

NSRunLoop::~NSRunLoop() {
	Stop();
	while (IsRunning())
		sched_yield();
	while (!events.empty()) {
		delete events.top().first;
		events.pop();
	}
}

// Takes ownership of the event (deallocs when processed)
void NSRunLoop::AddEvent(NSEvent* event) {
	AddEvent(event, std::function<void()>());
}

// Callback occurs in the run loop's thread
void NSRunLoop::AddEvent(NSEvent* event, std::function<void()> callback) {
	lock.Lock();
	events.push(std::make_pair(event, callback));
	cond.Signal();
	lock.Unlock();
}

uint32_t NSRunLoop::GetNumberOfEventsPending() const {
	return events.size();
}

// Takes ownership of the timer (deallocs when done)
void NSRunLoop::AddTimer(NSTimer* timer) {
	lock.Lock();
	NSTimeInterval interval = timer->interval;
	timer->next_iteration = time_point_cast<tp::duration>(system_clock::time_point(system_clock::now()))
							+ seconds((uint32_t)interval) + milliseconds(uint32_t((interval - uint32_t(interval)) * 1000));
	timers.push_back(timer);
	cond.Signal();
	lock.Unlock();
}

uint32_t NSRunLoop::GetNumberOfTimers() const {
	return 0;
}

void NSRunLoop::SetExitsWhenIdle(bool e) {
	exits = e;
}

bool NSRunLoop::GetExitsWhenIdle() const {
	return exits;
}

tp NSRunLoop::ProcessTimers(bool* out) {
	tp min;
	bool any = false;
	for (uint32_t z = 0; z < timers.size(); z++) {
		if (timers[z]->invalidated) {
			NSTimer* t = timers[z];
			timers.erase(timers.begin() + z);
			delete t;
			z--;
			continue;
		}
		auto t = time_point_cast<tp::duration>(system_clock::time_point(system_clock::now()));
		if (t >= timers[z]->next_iteration) {
			timers[z]->function(timers[z]);
			if (timers[z]->repeats && !timers[z]->invalidated) {
				NSTimeInterval interval = timers[z]->interval;
				timers[z]->next_iteration += seconds((uint32_t)interval) +
												milliseconds(uint32_t((interval - uint32_t(interval)) * 1000));
			} else {
				NSTimer* t = timers[z];
				timers.erase(timers.begin() + z);
				delete t;
				z--;
				continue;
			}
		}
		if (!any || min >= timers[z]->next_iteration) {
			any = true;
			min = timers[z]->next_iteration;
		}
	}
	
	*out = any;
	return min;
}

void NSRunLoop::Run() {
	lock.Lock();
	stopped = false;
	running = true;
	
	for (;;) {
		if (exits && events.empty() && timers.empty())
			break;
		
		// Find out when the closest timer is
		bool any = false;
		auto next_timer = ProcessTimers(&any);
		
		while (!stopped && events.empty()) {
			if (any) {
				auto t = next_timer - time_point_cast<tp::duration>(system_clock::time_point(system_clock::now()));
				NSTimeInterval interval = t.count() / 1000.0;
				if (interval > 0)
					cond.TimedWait(&lock, interval);
			} else
				cond.Wait(&lock);
			
			next_timer = ProcessTimers(&any);
		}
		
		if (stopped)
			break;
		
		auto pair = events.top();
		events.pop();
		lock.Unlock();

		NSEvent* event = pair.first;
		std::function<void()> callback = pair.second;
		event->Process();
		if (callback)
			callback();
		delete event;
		
		lock.Lock();
	}
	
	running = false;
	lock.Unlock();
}

void NSRunLoop::Stop() {
	lock.Lock();
	stopped = true;
	cond.Signal();
	lock.Unlock();
}

bool NSRunLoop::IsRunning() const {
	return running;
}

bool NSRunLoop::IsStopping() const {
	return stopped;
}

bool NSRunLoop::IsIdle() const {
	return (GetNumberOfEventsPending() == 0);
}
