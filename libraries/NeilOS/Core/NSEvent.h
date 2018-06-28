//
//  NSEvent.h
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSEVENT_H
#define NSEVENT_H

#include <functional>

class NSEvent {
public:
	static NSEvent* FromData(const uint8_t* data, uint32_t length);
	
	virtual ~NSEvent() {};
		
	virtual void Process() {};
	virtual uint8_t* Serialize(uint32_t* length_out) const { return NULL; };
	
	// 0 is the lowest / default priority
	void SetPriority(uint32_t p) { priority = p; };
	uint32_t GetPriority() const { return priority; };
private:
	uint32_t priority;
};

// For sorting by priority
struct NSEventCompare {
	bool operator()(const NSEvent* e1, const NSEvent* e2) {
		return (e1->GetPriority() > e2->GetPriority());
	}
};

class NSEventFunction : public NSEvent {
public:
	static NSEventFunction* Create(const std::function<void()>& function, uint32_t priority=0);
	
	void Cancel();
	bool IsCanceled() const;
	
	void Process() override;
	uint8_t* Serialize(uint32_t* length_out) const override;
private:
	NSEventFunction(const std::function<void()>& function, uint32_t priority);
	std::function<void()> function;
	bool canceled = false;
};

#endif /* NSEVENT_H */
