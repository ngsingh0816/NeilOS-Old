//
//  NSRunLoop.h
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSRUNLOOP_H
#define NSRUNLOOP_H

#include "NSEvent.h"
#include "NSLock.h"
#include <chrono>
#include <functional>
#include <queue>
#include <utility>
#include <vector>

class NSThread;
class NSTimer;

struct NSEventPairCompare {
	bool operator()(const std::pair<NSEvent*, std::function<void()>>& e1,
					const std::pair<NSEvent*, std::function<void()>>& e2) {
		return (e1.first->GetPriority() > e2.first->GetPriority());
	}
};

class NSRunLoop {
public:
	NSRunLoop();
	~NSRunLoop();
	
	// Takes ownership of the event (deallocs when processed)
	void AddEvent(NSEvent* event);
	// Callback occurs in the run loop's thread
	void AddEvent(NSEvent* event, std::function<void()> callback);
	uint32_t GetNumberOfEventsPending() const;
	
	// Takes ownership of the timer (deallocs when done)
	void AddTimer(NSTimer* timer);
	uint32_t GetNumberOfTimers() const;
	
	void SetExitsWhenIdle(bool exits);
	bool GetExitsWhenIdle() const;
	
	void Run();
	void Stop();
	bool IsRunning() const;
	bool IsStopping() const;
	bool IsIdle() const;
private:
	std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> ProcessTimers(bool* any);
	
	std::priority_queue<std::pair<NSEvent*, std::function<void()>>,
						std::vector<std::pair<NSEvent*, std::function<void()>>>,
						NSEventPairCompare> events;
	std::vector<NSTimer*> timers;
	NSLock lock;
	NSConditionalLock cond;
	volatile bool stopped = false;
	volatile bool running = false;
	volatile bool exits = false;
};

#endif /* NSRUNLOOP_H */
