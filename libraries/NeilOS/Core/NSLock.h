//
//  NSLock.h
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSLOCK_H
#define NSLOCK_H

#include "NSTypes.h"
#include <unordered_map>

class NSLock {
public:
	// Mutex (1 resource)
	NSLock();
	NSLock(unsigned int resources);
	~NSLock();
	
	void Lock();
	void Unlock();
	bool TryLock();
private:
	void* mutex;
	void* cond;
	volatile unsigned int count;
};

class NSConditionalLock {
public:
	NSConditionalLock();
	~NSConditionalLock();
	
	void Signal();
	void Broadcast();
	void Wait(NSLock* lock);
	// Returns false if timeouts
	bool TimedWait(NSLock* lock, NSTimeInterval interval);
private:
	std::unordered_map<unsigned int, bool> map;
	NSLock lock;
};

#endif /* NSLOCK_H */
