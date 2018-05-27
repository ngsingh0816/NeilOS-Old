//
//  NSThread.h
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSTHREAD_H
#define NSTHREAD_H

#include "NSRunLoop.h"
#include "NSApplication.h"
#include <functional>
#include <queue>

struct pthread;

class NSThread {
public:
	static void Setup();

	// Globals
	static NSThread* MainThread();
	static NSThread* CurrentThread();
	
	// Constructors (automatically deletes self upon exit)
	static NSThread* Create();
	static NSThread* Create(std::function<void(NSThread*)> function);
	static NSThread* Create(std::queue<std::function<void(NSThread*)>> functions);
	
	// Exit thread
	static void Exit();
	
	~NSThread();
	
	NSRunLoop* GetRunLoop();
	
	// True by default
	void SetExitsWhenDone(bool exits);
	bool GetExitsWhenDone() const;
	
	void Start();
	bool IsExcuting() const;
	// Exits upon next function completion or if current function realizes it is canceled
	void Cancel();
	// Signals to function running on thread that it should exit
	bool IsCanceled() const;
private:
	NSThread();
	NSThread(std::function<void(NSThread*)> function);
	NSThread(std::queue<std::function<void(NSThread*)>> functions);
	
	static void* NSThreadFunction(void*);

	NSRunLoop loop;
	pthread* thread = NULL;
};

#endif /* NSTHREAD_H */
