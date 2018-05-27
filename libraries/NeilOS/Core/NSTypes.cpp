//
//  NSTypes.cpp
//  product
//
//  Created by Neil Singh on 1/29/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSTypes.h"

#include <string.h>

#include <algorithm>

NSPoint::NSPoint() {
	x = 0;
	y = 0;
}

NSPoint::NSPoint(float _x, float _y) {
	x = _x;
	y = _y;
}

bool NSPoint::operator ==(const NSPoint& p2) const {
	return (x == p2.x && y == p2.y);
}

bool NSPoint::operator !=(const NSPoint& p2) const {
	return !(x == p2.x && y == p2.y);
}

NSPoint NSPoint::operator +(const NSPoint& p2) const {
	return NSPoint(x + p2.x, y + p2.y);
}

NSPoint& NSPoint::operator +=(const NSPoint& p2) {
	x += p2.x;
	y += p2.y;
	return *this;
}

NSPoint NSPoint::operator -(const NSPoint& p2) const {
	return NSPoint(x - p2.x, y - p2.y);
}

NSPoint& NSPoint::operator -=(const NSPoint& p2) {
	x -= p2.x;
	y -= p2.y;
	return *this;
}

NSPoint NSPoint::operator *(const float& scalar) const {
	return NSPoint(x * scalar, y * scalar);
}

NSPoint& NSPoint::operator *=(const float& scalar) {
	x *= scalar;
	y *= scalar;
	return *this;
}

// Piecewise multiplication
NSPoint NSPoint::operator *(const NSPoint& p2) const {
	return NSPoint(x * p2.x, y * p2.y);
}

NSPoint& NSPoint::operator *=(const NSPoint& p2) {
	x *= p2.x;
	y *= p2.y;
	return *this;
}

NSPoint NSPoint::operator /(const float& scalar) const {
	return NSPoint(x / scalar, y / scalar);
}

NSPoint& NSPoint::operator /=(const float& scalar) {
	x /= scalar;
	y /= scalar;
	return *this;
}

bool NSPoint::InsideRect(const NSRect& rect) const {
	return rect.ContainsPoint(*this);
}

uint8_t* NSPoint::Serialize(uint32_t* length_out) const {
	uint32_t length = sizeof(float) * 2;
	uint8_t* buffer = new uint8_t[length];
	memcpy(buffer, &x, sizeof(float));
	memcpy(&buffer[sizeof(float)], &y, sizeof(float));
	if (length_out)
		*length_out = length;
	return buffer;
}

NSPoint NSPoint::FromData(const uint8_t* data, uint32_t length) {
	if (length != sizeof(float) * 2)
		return NSPoint();
	
	NSPoint ret;
	memcpy(&ret.x, data, sizeof(float));
	memcpy(&ret.y, &data[sizeof(float)], sizeof(float));
	
	return ret;
}

NSSize::NSSize() {
	width = 0;
	height = 0;
}

NSSize::NSSize(float _width, float _height) {
	width = _width;
	height = _height;
}

bool NSSize::operator ==(const NSSize& p2) const {
	return (width == p2.width && height == p2.height);
}

bool NSSize::operator !=(const NSSize& p2) const {
	return !(width == p2.width && height == p2.height);
}

NSSize NSSize::operator +(const NSSize& p2) const {
	return NSSize(width + p2.width, height + p2.height);
}

NSSize& NSSize::operator +=(const NSSize& p2) {
	width += p2.width;
	height += p2.height;
	return *this;
}

NSSize NSSize::operator -(const NSSize& p2) const {
	return NSSize(width - p2.width, height - p2.height);
}

NSSize& NSSize::operator -=(const NSSize& p2) {
	width -= p2.width;
	height -= p2.height;
	return *this;
}

NSSize NSSize::operator *(const float& scalar) const {
	return NSSize(width * scalar, height * scalar);
}

NSSize& NSSize::operator *=(const float& scalar) {
	width *= scalar;
	height *= scalar;
	return *this;
}

// Piecewise multiplication
NSSize NSSize::operator *(const NSSize& p2) const {
	return NSSize(width + p2.width, height + p2.height);
}

NSSize& NSSize::operator *=(const NSSize& p2) {
	width *= p2.width;
	height *= p2.height;
	return *this;
}

NSSize NSSize::operator /(const float& scalar) const {
	return NSSize(width / scalar, height / scalar);
}

NSSize& NSSize::operator /=(const float& scalar) {
	width /= scalar;
	height /= scalar;
	return *this;
}

uint8_t* NSSize::Serialize(uint32_t* length_out) const {
	uint32_t length = sizeof(float) * 2;
	uint8_t* buffer = new uint8_t[length];
	memcpy(buffer, &width, sizeof(float));
	memcpy(&buffer[sizeof(float)], &height, sizeof(float));
	if (length_out)
		*length_out = length;
	return buffer;
}

NSSize NSSize::FromData(const uint8_t* data, uint32_t length) {
	if (length != sizeof(float) * 2)
		return NSSize();
	
	NSSize ret;
	memcpy(&ret.width, data, sizeof(float));
	memcpy(&ret.height, &data[sizeof(float)], sizeof(float));
	
	return ret;
}

NSRect::NSRect() {
	origin = NSPoint();
	size = NSSize();
}

NSRect::NSRect(float _x, float _y, float _width, float _height) {
	origin = NSPoint(_x, _y);
	size = NSSize(_width, _height);
}

NSRect::NSRect(const NSPoint& o, const NSSize& s) {
	origin = o;
	size = s;
}

bool NSRect::operator ==(const NSRect& p2) const {
	return (origin == p2.origin && size == p2.size);
}

bool NSRect::operator !=(const NSRect& p2) const {
	return !(origin == p2.origin && size == p2.size);
}

NSRect NSRect::operator +(const NSRect& p2) const {
	return NSRect(origin + p2.origin, size + p2.size);
}

NSRect& NSRect::operator +=(const NSRect& p2) {
	origin += p2.origin;
	size += p2.size;
	return *this;
}

NSRect NSRect::operator -(const NSRect& p2) const {
	return NSRect(origin - p2.origin, size - p2.size);
}

NSRect& NSRect::operator -=(const NSRect& p2) {
	origin -= p2.origin;
	size -= p2.size;
	return *this;
}

NSRect NSRect::operator *(const float& scalar) const {
	return NSRect(origin * scalar, size * scalar);
}

NSRect& NSRect::operator *=(const float& scalar) {
	origin *= scalar;
	size *= scalar;
	return *this;
}

NSRect NSRect::operator /(const float& scalar) const {
	return NSRect(origin / scalar, size / scalar);
}

NSRect& NSRect::operator /=(const float& scalar) {
	origin /= scalar;
	size *= scalar;
	return *this;
}

bool NSRect::ContainsPoint(const NSPoint& p) const {
	return (p.x >= origin.x && p.y >= origin.y &&
			p.x < origin.x + size.width && p.y < origin.y + size.height);
}

bool NSRect::ContainsRect(const NSRect& r) const {
	return ContainsPoint(r.origin) && ContainsPoint(r.origin + NSPoint(r.size.width, r.size.height));
}

bool NSRect::OverlapsRect(const NSRect& r) const {
	// Increase 0.5 on both sides
	return (origin.x <= r.origin.x + r.size.width && origin.x + size.width >= r.origin.x &&
			origin.y <= r.origin.y + r.size.height && origin.y + size.height >= r.origin.y);
}

NSRect NSRect::Intersection(const NSRect& r) const {
	if (!OverlapsRect(r))
		return NSRect();
	
	float x5 = std::max(origin.x, r.origin.x);
	float y5 = std::max(origin.y, r.origin.y);
	float x6 = std::min(origin.x + size.width, r.origin.x + r.size.width);
	float y6 = std::min(origin.y + size.height, r.origin.y + r.size.height);
	
	return NSRect(x5, y5, x6 - x5, y6 - y5);
}

// Provides rect that is rounded to nearest integers that contains the original rect
NSRect NSRect::IntegerRect() const {
	// Update to integer offsets
	NSRect rect = *this;
	rect.origin.x -= rect.origin.x - (int)rect.origin.x;
	rect.origin.y -= rect.origin.y - (int)rect.origin.y;
	rect.size.width = (int)rect.size.width + 1;
	rect.size.height = (int)rect.size.height + 1;
	return rect;
}

uint8_t* NSRect::Serialize(uint32_t* length_out) const {
	uint32_t length = sizeof(float) * 4;
	uint8_t* buffer = new uint8_t[length];
	memcpy(buffer, &origin.x, sizeof(float));
	memcpy(&buffer[sizeof(float)], &origin.y, sizeof(float));
	memcpy(&buffer[sizeof(float)*2], &size.width, sizeof(float));
	memcpy(&buffer[sizeof(float)*3], &size.height, sizeof(float));
	if (length_out)
		*length_out = length;
	return buffer;
}

NSRect NSRect::FromData(const uint8_t* data, uint32_t length) {
	if (length != sizeof(float) * 4)
		return NSRect();
	
	NSRect ret;
	memcpy(&ret.origin.x, data, sizeof(float));
	memcpy(&ret.origin.y, &data[sizeof(float)], sizeof(float));
	memcpy(&ret.size.width, &data[sizeof(float)*2], sizeof(float));
	memcpy(&ret.size.height, &data[sizeof(float)*3], sizeof(float));
	
	return ret;
}

// Clamp a rectangle to the clamp rectangle
bool NSRectClamp(NSRect rect, NSRect clamp, NSRect* rect_out) {
	if (!rect.OverlapsRect(clamp))
		return false;
	if (rect.origin.x < clamp.origin.x) {
		rect.size.width += rect.origin.x - clamp.origin.x;
		rect.origin.x = clamp.origin.x;
	}
	if (rect.origin.y < clamp.origin.x) {
		rect.size.height += rect.origin.y - clamp.origin.y;
		rect.origin.y = clamp.origin.y;
	}
	if (rect.origin.x + rect.size.width >= clamp.origin.x + clamp.size.width)
		rect.size.width = clamp.origin.x + clamp.size.width - rect.origin.x;
	if (rect.origin.y + rect.size.height >= clamp.origin.y + clamp.size.height)
		rect.size.height = clamp.origin.y + clamp.size.height - rect.origin.y;
	
	if (rect_out)
		*rect_out = rect;
	return true;
}

// Correct negative dimensions
NSRect NSRectCorrected(NSRect rect) {
	if (rect.size.width < 0) {
		rect.origin.x += rect.size.width;
		rect.size.width = -rect.size.width;
	}
	if (rect.size.height < 0) {
		rect.origin.y += rect.size.height;
		rect.size.height = -rect.size.height;
	}
	return rect;
}
