//
//  NSHandler.h
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSHANDLER_H
#define NSHANDLER_H

#include "NSThread.h"
#include "NSTypes.h"
#include <functional>

#define NSHandlerDefaultPriortiy	0

// Use std::bind to bind arguments
class NSHandler {
public:
	NSHandler();
	NSHandler(const std::function<void(NSThread*)>& function);
	
	void SetFunction(const std::function<void(NSThread*)>& function);
	const std::function<void(NSThread*)>& GetFunction() const;
	
	void Cancel();
	bool IsCanceled();
	
	// Posts to the main thread (asynchronous by default)
	void Post();
	void Post(uint32_t priority);
	void Post(NSThread* thread, uint32_t priority);
	void Post(NSThread* thread, uint32_t priority, bool sync);
	void Post(NSThread* thread, NSTimeInterval delay, uint32_t priority, bool sync);
private:
	std::function<void(NSThread*)> func;
	NSEventFunction* event = NULL;
	bool canceled = false;
};

#endif /* NSHANDLER_H */
