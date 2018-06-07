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
	std::function<void(NSThread*)> GetFunction() const;
	
	// Posts to the main thread (asynchronous by default)
	void Post() const;
	void Post(uint32_t priority) const;
	void Post(NSThread* thread, uint32_t priority) const;
	void Post(NSThread* thread, uint32_t priority, bool sync) const;
	void Post(NSThread* thread, NSTimeInterval delay, uint32_t priority, bool sync);
private:
	std::function<void(NSThread*)> func;
};

#endif /* NSHANDLER_H */
