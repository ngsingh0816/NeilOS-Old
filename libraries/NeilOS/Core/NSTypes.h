//
//  NSTypes.h
//  NeilOS
//
//  Created by Neil Singh on 1/29/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSTYPES_H
#define NSTYPES_H

#include <stdint.h>

#include <vector>

typedef double NSTimeInterval;

class NSRect;

class NSPoint {
public:
	NSPoint();
	NSPoint(float _x, float _y);
	
	bool operator ==(const NSPoint& p2) const;
	bool operator !=(const NSPoint& p2) const;
	NSPoint operator +(const NSPoint& p2) const;
	NSPoint& operator +=(const NSPoint& p2);
	NSPoint operator -(const NSPoint& p2) const;
	NSPoint& operator -=(const NSPoint& p2);
	NSPoint operator *(const float& scalar) const;
	NSPoint& operator *=(const float& scalar);
	// Piecewise multiplication
	NSPoint operator *(const NSPoint& p2) const;
	NSPoint& operator *=(const NSPoint& p2);
	NSPoint operator /(const float& scalar) const;
	NSPoint& operator /=(const float& scalar);
	
	bool InsideRect(const NSRect& rect) const;
	
	uint8_t* Serialize(uint32_t* length_out) const;
	static NSPoint FromData(const uint8_t* data, uint32_t length);

	float x;
	float y;
};

class NSSize {
public:
	NSSize();
	NSSize(float _width, float _height);
	
	bool operator ==(const NSSize& p2) const;
	bool operator !=(const NSSize& p2) const;
	NSSize operator +(const NSSize& p2) const;
	NSSize& operator +=(const NSSize& p2);
	NSSize operator -(const NSSize& p2) const;
	NSSize& operator -=(const NSSize& p2);
	NSSize operator *(const float& scalar) const;
	NSSize& operator *=(const float& scalar);
	// Piecewise multiplication
	NSSize operator *(const NSSize& p2) const;
	NSSize& operator *=(const NSSize& p2);
	// Piecewise division
	NSSize operator /(const NSSize& p2) const;
	NSSize& operator /=(const NSSize& p2);
	NSSize operator /(const float& scalar) const;
	NSSize& operator /=(const float& scalar);
	
	uint8_t* Serialize(uint32_t* length_out) const;
	static NSSize FromData(const uint8_t* data, uint32_t length);
	
	float width;
	float height;
};

class NSRect {
public:
	NSRect();
	NSRect(float _x, float _y, float _width, float _height);
	NSRect(const NSPoint& o, const NSSize& size);
	
	bool operator ==(const NSRect& p2) const;
	bool operator !=(const NSRect& p2) const;
	NSRect operator +(const NSRect& p2) const;
	NSRect& operator +=(const NSRect& p2);
	NSRect operator -(const NSRect& p2) const;
	NSRect& operator -=(const NSRect& p2);
	NSRect operator *(const float& scalar) const;
	NSRect& operator *=(const float& scalar);
	NSRect operator /(const float& scalar) const;
	NSRect& operator /=(const float& scalar);
	
	bool ContainsPoint(const NSPoint& p) const;
	bool ContainsRect(const NSRect& r) const;
	bool OverlapsRect(const NSRect& r) const;
	NSRect Intersection(const NSRect& r) const;
	// Provides rect that is rounded to nearest integers that contains the original rect
	NSRect IntegerRect() const;
	
	uint8_t* Serialize(uint32_t* length_out) const;
	static NSRect FromData(const uint8_t* data, uint32_t length);
	
	NSPoint origin;
	NSSize size;
};

// Clamp a rect to be within the clamp bounds
// Returns false if result is an empty rect
bool NSRectClamp(const NSRect& rect, NSRect clamp, NSRect* rect_out);
// Correct negative dimensions
NSRect NSRectCorrected(const NSRect& rect);

// Given a vector of NSRects, return the minimum number of nonoverlapping rects that cover the same region
std::vector<NSRect> NSRectConsolidate(const std::vector<NSRect>& rects);
// For adding one at a time
void NSRectAddConsolidated(std::vector<NSRect>& rects, const NSRect& rect);

#endif /* NSTYPES_H */
