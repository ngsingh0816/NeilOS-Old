//
//  NSResponder.h
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSRESPONDER_H
#define NSRESPONDER_H

#include "NSEvent.h"
#include "NSTypes.h"

typedef unsigned int NSModifierFlags;
#define NSModifierNone		0
#define NSModifierControl	(1 << 0)
#define NSModifierOption	(1 << 1)
#define NSModifierShift		(1 << 2)
#define NSModifierCommand	(1 << 3)
#define NSModifierFunction	(1 << 4)

class NSResponder {
public:
	virtual ~NSResponder() {}
	
	virtual bool AcceptsFirstResponder() const { return false; };
	virtual void BecomeFirstResponder() {};
	virtual bool IsFirstResponder() const { return false; };
	virtual void ResignFirstResponder() {};
	
	virtual void MouseDown(NSEvent* event) {}
	virtual void MouseDragged(NSEvent* event) {}
	virtual void MouseUp(NSEvent* event) {}
	virtual void MouseMoved(NSEvent* event) {}
	virtual void MouseScrolled(NSEvent* event) {}
	
	virtual void keyDown(NSEvent* event) {}
	virtual void KeyUp(NSEvent* event) {}
private:
};

typedef enum {
	NSMouseTypeNone = 0,
	NSMouseTypeDown = 1,
	NSMouseTypeMoved = 2,
	NSMouseTypeDragged = 3,
	NSMouseTypeUp = 4
} NSMouseType;

typedef enum {
	NSMouseButtonNone = 0,
	NSMouseButtonLeft = 1,
	NSMouseButtonMiddle = 2,
	NSMouseButtonRight = 3
} NSMouseButton;

// Events
class NSEventMouse : public NSEvent {
public:
	static NSEventMouse* Create(NSPoint position, NSMouseType type, NSMouseButton button,
								uint32_t window_id=-1, uint32_t priority=0);
	
	NSPoint GetPosition() const;
	void SetPosition(NSPoint position);
	
	NSMouseType GetType() const;
	void SetType(NSMouseType type);
	
	NSMouseButton GetButton() const;
	void SetButton(NSMouseButton button);
	
	uint32_t GetWindowID() const;
	void SetWindowID(uint32_t wid);
	
	void Process() override;
	uint8_t* Serialize(uint32_t* length_out) const override;
private:
	NSEventMouse(NSPoint position, NSMouseType type, NSMouseButton button, uint32_t window_id, uint32_t priority);
	
	NSPoint position;
	NSMouseType type;
	NSMouseButton button;
	uint32_t window_id;
};

#endif /* NSRESPONDER_H */
