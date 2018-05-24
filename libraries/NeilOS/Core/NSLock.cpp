//
//  NSLock.cpp
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSLock.h"
#include <pthread.h>
#include <chrono>

using namespace std::chrono;

NSLock::NSLock() {
	count = 1;
	mutex = new pthread_mutex_t;
	*(pthread_mutex_t*)mutex = PTHREAD_MUTEX_INITIALIZER;
	cond = new pthread_cond_t;
	*(pthread_cond_t*)cond = PTHREAD_COND_INITIALIZER;
}

NSLock::NSLock(unsigned int resources) {
	count = resources;
	mutex = new pthread_mutex_t;
	*(pthread_mutex_t*)mutex = PTHREAD_MUTEX_INITIALIZER;
	cond = new pthread_cond_t;
	*(pthread_cond_t*)cond = PTHREAD_COND_INITIALIZER;
}

NSLock::~NSLock() {
	delete (pthread_mutex_t*)mutex;
	delete (pthread_cond_t*)cond;
}

void NSLock::Lock() {
	pthread_mutex_lock((pthread_mutex_t*)mutex);
	while (count == 0)
		pthread_cond_wait((pthread_cond_t*)cond, (pthread_mutex_t*)mutex);
	count--;
	pthread_mutex_unlock((pthread_mutex_t*)mutex);
}

void NSLock::Unlock() {
	pthread_mutex_lock((pthread_mutex_t*)mutex);
	count++;
	pthread_cond_signal((pthread_cond_t*)cond);
	pthread_mutex_unlock((pthread_mutex_t*)mutex);
}

bool NSLock::TryLock() {
	bool ret = false;
	pthread_mutex_lock((pthread_mutex_t*)mutex);
	if (count != 0) {
		count--;
		ret = true;
	}
	pthread_mutex_unlock((pthread_mutex_t*)mutex);
	return ret;
}

NSConditionalLock::NSConditionalLock() {
}

NSConditionalLock::~NSConditionalLock() {
}

void NSConditionalLock::Signal() {
	lock.Lock();
	if (map.size() != 0)
		map.begin()->second = false;
	lock.Unlock();
}

void NSConditionalLock::Broadcast() {
	lock.Lock();
	for (auto it = map.begin(); it != map.end(); it++)
		it->second = false;
	lock.Unlock();
}

void NSConditionalLock::Wait(NSLock* l) {
	// TODO: should we switch the order of these?
	l->Unlock();
	lock.Lock();
	
	uint32_t tid = gettid();
	map[tid] = true;
	while (map[tid]) {
		lock.Unlock();
		sched_yield();
		lock.Lock();
	}
	map.erase(tid);
	
	lock.Unlock();
	l->Lock();
}

bool NSConditionalLock::TimedWait(NSLock* l, NSTimeInterval interval) {
	typedef time_point<system_clock, milliseconds> t;
	auto end_time = time_point_cast<t::duration>(system_clock::time_point(system_clock::now()))
			+ seconds((uint32_t)interval) + milliseconds(uint32_t((interval - uint32_t(interval)) * 1000));
	// TODO: should we switch the order of these?
	l->Unlock();
	lock.Lock();
	
	uint32_t tid = gettid();
	map[tid] = true;
	while (map[tid]) {
		lock.Unlock();
				
		if (time_point_cast<t::duration>(system_clock::time_point(system_clock::now())) >= end_time)
			return false;
		
		sched_yield();
		lock.Lock();
	}
	map.erase(tid);
	
	lock.Unlock();
	l->Lock();
	
	return true;
}
