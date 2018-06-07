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

class NSEventMouse;
class NSEventKey;

class NSResponder {
public:
	virtual ~NSResponder() {}
	
	virtual bool AcceptsFirstResponder() const { return false; };
	virtual void BecomeFirstResponder() {};
	virtual bool IsFirstResponder() const { return false; };
	virtual void ResignFirstResponder() {};
	
	virtual void MouseDown(NSEventMouse* event) {}
	virtual void MouseDragged(NSEventMouse* event) {}
	virtual void MouseUp(NSEventMouse* event) {}
	virtual void MouseMoved(NSEventMouse* event) {}
	virtual void MouseScrolled(NSEventMouse* event) {}
	
	virtual void KeyDown(NSEventKey* event) {}
	virtual void KeyUp(NSEventKey* event) {}
private:
};

typedef enum {
	NSMouseTypeNone = 0,
	NSMouseTypeDown = 1,
	NSMouseTypeMoved = 2,
	NSMouseTypeDragged = 3,
	NSMouseTypeUp = 4,
	NSMouseTypeScrolled = 5,
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
	
	static NSEventMouse* FromData(uint8_t* data, uint32_t length);
	
	NSPoint GetPosition() const;
	void SetPosition(NSPoint position);
	
	NSMouseType GetType() const;
	void SetType(NSMouseType type);
	
	NSMouseButton GetButton() const;
	void SetButton(NSMouseButton button);
	
	uint32_t GetWindowID() const;
	void SetWindowID(uint32_t wid);
	
	float GetDeltaX() const;
	void SetDeltaX(float dx);
	
	float GetDeltaY() const;
	void SetDeltaY(float dy);
	
	void Process() override;
	uint8_t* Serialize(uint32_t* length_out) const override;
private:
	NSEventMouse(NSPoint position, NSMouseType type, NSMouseButton button, uint32_t window_id, uint32_t priority);
	
	NSPoint position;
	NSMouseType type;
	NSMouseButton button;
	uint32_t window_id;
	float delta_x;
	float delta_y;
};

#define NSKeyCodeNUL			0x00
#define NSKeyCodeEOF			0x04
#define NSKeyCodeBEL			0x07
#define NSKeyCodeBackspace		0x08
#define NSKeyCodeTab			0x09
#define NSKeyCodeEnter			0xA
#define NSKeyCodeVerticalTab 	0xB
#define NSKeyCodeNewPage		0xC
#define NSKeyCodeCarriageReturn	0xD
#define NSKeyCodeShiftOut		0xE
#define NSKeyCodeShiftIn		0xF
#define NSKeyCodeCancel			0x18
#define NSKeyCodeSubstitute		0x1A
#define NSKeyCodeEscape			0x1B
#define NSKeyCodeDelete			0x7F
#define NSKeyCodeF1				0x80
#define NSKeyCodeF2				0x81
#define NSKeyCodeF3				0x82
#define NSKeyCodeF4				0x83
#define NSKeyCodeF5				0x84
#define NSKeyCodeF6				0x85
#define NSKeyCodeF7				0x86
#define NSKeyCodeF8				0x87
#define NSKeyCodeF9				0x88
#define NSKeyCodeF10			0x89
#define NSKeyCodeF11			0x8A
#define NSKeyCodeF12			0x8B
#define NSKeyCodeLeftControl 	0x90
#define NSKeyCodeLeftShift		0x91
#define NSKeyCodeRightShift		0x92
#define NSKeyCodeCapsLock		0x93
#define NSKeyCodeLeftAlt		0x94
#define NSKeyCodeRightAlt		0x95
#define NSKeyCodeEnd			0x96
#define NSKeyCodeHome			0x97
#define NSKeyCodeInsert			0x98
#define NSKeyCodePageUp			0x99
#define NSKeyCodePageDown		0x9A
#define NSKeyCodePause			0x9B
#define NSKeyCodeCSI			0x9B
#define NSKeyCodeUpArrow		0x9C
#define NSKeyCodeDownArrow		0x9D
#define NSKeyCodeRightArrow		0x9E
#define NSKeyCodeLeftArrow		0x9F
#define NSKeyCodeRightControl	0xA0
#define NSKeyCodeNumLock		0xA1
#define NSKeyCodeScrollLock		0xA2
#define NSKeyCodeLeftCommand	0xA3
#define NSKeyCodeRightCommand	0xA4

class NSEventKey : public NSEvent {
public:
	static NSEventKey* Create(unsigned char key, bool down, NSModifierFlags flags, uint32_t window_id=-1);
	
	static NSEventKey* FromData(uint8_t* data, uint32_t length);
	
	unsigned char GetKey() const;
	void SetKey(unsigned char key);
	
	bool GetDown() const;
	void SetDown(bool down);
	
	NSModifierFlags GetModifierFlags() const;
	void SetModifierFlags(NSModifierFlags flags);
	
	uint32_t GetWindowID() const;
	void SetWindowID(uint32_t wid);
	
	void Process() override;
	uint8_t* Serialize(uint32_t* length_out) const override;
private:
	NSEventKey(unsigned char key, bool down, NSModifierFlags flags, uint32_t window_id);
	
	unsigned char key;
	bool down;
	NSModifierFlags flags;
	uint32_t window_id;
};

#endif /* NSRESPONDER_H */
