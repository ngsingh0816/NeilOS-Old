//
//  NSThread.cpp
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSThread.h"
#include <pthread.h>
#include <map>

#define MAIN_THREAD_TID				1

namespace {
	// Maps tid to NSThread
	std::unordered_map<uint32_t, NSThread*> threads;
	pthread_mutex_t threads_lock = PTHREAD_MUTEX_INITIALIZER;
};

void NSThread::Setup() {
	NSThread* main = new NSThread;
	main->thread = pthread_self();
	main->SetExitsWhenDone(false);
	threads[MAIN_THREAD_TID] = main;
}

NSThread* NSThread::MainThread() {
	pthread_mutex_lock(&threads_lock);
	NSThread* ret = threads[MAIN_THREAD_TID];
	pthread_mutex_unlock(&threads_lock);
	return ret;
}

NSThread* NSThread::CurrentThread() {
	uint32_t tid = gettid();
	pthread_mutex_lock(&threads_lock);
	NSThread* ret = threads[tid];
	pthread_mutex_unlock(&threads_lock);
	return ret;
}

void NSThread::Exit() {
	CurrentThread()->GetRunLoop()->Stop();
}

NSThread* NSThread::Create() {
	return new NSThread();
}

NSThread* NSThread::Create(const std::function<void(NSThread*)>& function) {
	return new NSThread(function);
}

NSThread* NSThread::Create(std::queue<std::function<void(NSThread*)>> functions) {
	return new NSThread(functions);
}

NSThread::NSThread() {
	thread = NULL;
	SetExitsWhenDone(true);
}

NSThread::NSThread(const std::function<void(NSThread*)>& function) {
	thread = NULL;
	loop.AddEvent(NSEventFunction::Create(std::bind(function, this)));
}

NSThread::NSThread(std::queue<std::function<void(NSThread*)>> functions) {
	while (!functions.empty()) {
		loop.AddEvent(NSEventFunction::Create(std::bind(functions.front(), this)));
		functions.pop();
	}
}

NSThread::~NSThread() {
	loop.Stop();
	while (loop.IsRunning())
		sched_yield();
	pthread_join(thread, NULL);
}

NSRunLoop* NSThread::GetRunLoop() {
	return &loop;
}

// True by default
void NSThread::SetExitsWhenDone(bool e) {
	loop.SetExitsWhenIdle(e);
}

bool NSThread::GetExitsWhenDone() const {
	return loop.GetExitsWhenIdle();
}

void* NSThread::NSThreadFunction(void* info) {
	NSThread* thread = reinterpret_cast<NSThread*>(info);

	uint32_t tid = gettid();
	pthread_mutex_lock(&threads_lock);
	threads[tid] = thread;
	pthread_mutex_unlock(&threads_lock);
	
	thread->GetRunLoop()->Run();
	
	pthread_mutex_lock(&threads_lock);
	threads.erase(tid);
	pthread_mutex_unlock(&threads_lock);
	
	thread->thread = NULL;
	
	delete thread;
	
	return NULL;
}

void NSThread::Start() {
	if (thread)
		return;
	
	pthread_create(&thread, NULL, NSThread::NSThreadFunction, this);
}

bool NSThread::IsExcuting() const {
	return loop.IsRunning();
}

// Exits upon next function completion or if current function realizes it is canceled
void NSThread::Cancel() {
	loop.Stop();
}

// Signals to function running on thread that it should exit
bool NSThread::IsCanceled() const {
	return loop.IsStopping();
}
