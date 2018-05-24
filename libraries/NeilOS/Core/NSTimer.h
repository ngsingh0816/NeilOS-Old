//
//  NSTimer.h
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSTIMER_H
#define NSTIMER_H

#include "NSTypes.h"
#include "NSRunLoop.h"

#include <chrono>
#include <functional>

class NSTimer {
public:
	// Constructors
	static NSTimer* Create();
	static NSTimer* Create(std::function<void(NSTimer*)> function, NSTimeInterval interval,
						   bool repeats=false, bool schedule=true);
	
	~NSTimer();
	
	void SetInterval(NSTimeInterval interval);
	NSTimeInterval GetInterval() const;
	void SetRepeats(bool repeats);
	bool GetRepeats() const;
	void SetFunction(std::function<void(NSTimer*)> function);
	std::function<void(NSTimer*)> GetFunction() const;
	
	// Cancels a repeating timer
	void Invalidate();
	bool IsInvalidated();
private:
	friend class NSRunLoop;
	
	NSTimer();
	// If schedule==true, there is no need to call NSRunLoop::AddTimer
	NSTimer(std::function<void(NSTimer*)> function, NSTimeInterval interval, bool repeats, bool schedule);
	
	std::function<void(NSTimer*)> function;
	std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> next_iteration;
	NSTimeInterval interval;
	bool repeats;
	bool invalidated;
};

#endif /* NSTIMER_H */
