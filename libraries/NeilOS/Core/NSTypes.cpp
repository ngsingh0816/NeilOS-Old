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
	return NSSize(width * p2.width, height * p2.height);
}

NSSize& NSSize::operator *=(const NSSize& p2) {
	width *= p2.width;
	height *= p2.height;
	return *this;
}

// Piecewise division
NSSize NSSize::operator /(const NSSize& p2) const {
	return NSSize(width / p2.width, height / p2.height);
}

NSSize& NSSize::operator /=(const NSSize& p2) {
	width /= p2.width;
	height /= p2.height;
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
bool NSRectClamp(const NSRect& r, NSRect clamp, NSRect* rect_out) {
	NSRect rect = r;
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
NSRect NSRectCorrected(const NSRect& r) {
	NSRect rect = r;
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

namespace {
	bool OverlapsRectNonInclusive(const NSRect& s, const NSRect& r) {
		return (s.origin.x < r.origin.x + r.size.width && s.origin.x + s.size.width > r.origin.x &&
				s.origin.y < r.origin.y + r.size.height && s.origin.y + s.size.height > r.origin.y);
	}
	
	std::vector<NSRect> FindIndependentOverlap(const NSRect& from, const NSRect& value,
											   bool olt, bool olb, bool oll, bool olr) {
		std::vector<NSRect> ret;
		if (olt) {
			// Overlaps top
			ret.push_back(NSRect(value.origin.x, value.origin.y,
								 value.size.width, from.origin.y - value.origin.y));
		}
		if (olb) {
			// Overlaps bottom
			ret.push_back(NSRect(value.origin.x, from.origin.y + from.size.height, value.size.width,
								 value.origin.y + value.size.height - (from.origin.y + from.size.height)));
		}
		if (oll) {
			// Overlaps left
			ret.push_back(NSRect(value.origin.x, value.origin.y,
								 from.origin.x - value.origin.x, value.size.height));
		}
		if (olr) {
			// Overlaps right
			ret.push_back(NSRect(from.origin.x + from.size.width, value.origin.y,
								value.origin.x + value.size.width - (from.origin.x + from.size.width),
								 value.size.height));
		}
		return ret;
	}
	
	std::vector<NSRect> FindDependentOverlap(const NSRect& from, const NSRect& value,
											 bool olt, bool olb, bool oll, bool olr) {
		std::vector<NSRect> ret;
		ret.resize(2);
		if (olt && olr) {
			// Top-right
			ret[0] = NSRect(value.origin.x, value.origin.y,
							value.size.width, from.origin.y - value.origin.y);
			ret[1] = NSRect(from.origin.x + from.size.width, from.origin.y,
							value.origin.x + value.size.width - (from.origin.x + from.size.width),
							value.origin.y + value.size.height - from.origin.y);
		} else if (olt && oll) {
			// Top-left
			ret[0] = NSRect(value.origin.x, value.origin.y,
							value.size.width, from.origin.y - value.origin.y);
			ret[1] = NSRect(value.origin.x, from.origin.y, from.origin.x - value.origin.x,
							value.origin.y + value.size.height - from.origin.y);
		} else if (olb && olr) {
			// Bottom-right
			ret[0] = NSRect(value.origin.x, from.origin.y + from.size.height, value.size.width,
							value.origin.y + value.size.height - (from.origin.y + from.size.height));
			ret[1] = NSRect(from.origin.x + from.size.width, value.origin.y,
							value.origin.x + value.size.width - (from.origin.x + from.size.width),
							from.origin.y + from.size.height - value.origin.y);
		} else if (olb && oll) {
			// Bottom-left
			ret[0] = NSRect(value.origin.x, from.origin.y + from.size.height, value.size.width,
							value.origin.y + value.size.height - (from.origin.y + from.size.height));
			ret[1] = NSRect(value.origin.x, value.origin.y, from.origin.x - value.origin.x,
							from.origin.y + from.size.height - value.origin.y);
		}
		return ret;
	}
	
	std::vector<NSRect> RectSubtract(const NSRect& from, const NSRect& value, bool* remove) {
		*remove = false;

		/* Cases (| = from, - = value, + = overlap)
		 0) value completely in from -> 0 rects
		 |||||||||
		 |||+++|||
		 |||+++|||
		 |||||||||
		 1) value goes out a single side -> 1 rect
		    ---
		 |||+++|||
		 |||+++|||
		 |||||||||
		 2.1) value goes out two opposite sides  -> 2 rects
		    ---
		 |||+++|||
		 |||+++|||
		 |||+++|||
		    ---
		 2.2) value goes out two adjacent sides -> 2 rects - these rects could potentially overlap though
		        ----
		 |||||||++--
		 |||||||++--
		 |||||||||
		 3) value goes out three sides -> 1 rect (reverse)
		    |||
		 ---+++---
		 ---+++---
		 ---------
		 4) from completely in value -> 0 rects (reverse)
		 ---------
		 ---+++---
		 ---+++---
		 ---------
		 */
		
		// Find the number of overlaps and which ones
		bool overlaps_top = (value.origin.y < from.origin.y) && (value.origin.y + value.size.height > from.origin.y);
		bool overlaps_bottom = (value.origin.y < from.origin.y + from.size.height) &&
								(value.origin.y + value.size.height > from.origin.y + from.size.height);
		bool overlaps_left = (value.origin.x < from.origin.x) && (value.origin.x + value.size.width > from.origin.x);
		bool overlaps_right = (value.origin.x < from.origin.x + from.size.width) &&
								(value.origin.x + value.size.width > from.origin.x + from.size.width);
		
		int num_overlaps = 0;
		if (overlaps_top)
			num_overlaps++;
		if (overlaps_bottom)
			num_overlaps++;
		if (overlaps_left)
			num_overlaps++;
		if (overlaps_right)
			num_overlaps++;
		
		// Cases
		switch (num_overlaps) {
			default:
			case 0: {
				return std::vector<NSRect>();
			}
			case 1:
				return FindIndependentOverlap(from, value, overlaps_top, overlaps_bottom,
											overlaps_left, overlaps_right);
			case 2:
				// Case 2.1
				if ((overlaps_top && overlaps_bottom) || (overlaps_left && overlaps_right)) {
					return FindIndependentOverlap(from, value, overlaps_top, overlaps_bottom,
												overlaps_left, overlaps_right);
				}
				// Case 2.2
				return FindDependentOverlap(from, value, overlaps_top, overlaps_bottom,
											  overlaps_left, overlaps_right);
			case 3: {
				bool other_remove = false;
				std::vector<NSRect> other = RectSubtract(value, from, &other_remove);
				other.push_back(value);
				*remove = true;
				return other;
			}
			case 4: {
				*remove = true;
				std::vector<NSRect> ret = { value };
				return ret;
			}
		}
	}
	
	void AddRectangle(std::vector<NSRect>& rects, const NSRect& rect) {
		for (unsigned int z = 0; z < rects.size(); z++) {
			const NSRect& original = rects[z];
			if (OverlapsRectNonInclusive(rect, original)) {
				// Split up
				bool remove = false;
				auto new_rects = RectSubtract(original, rect, &remove);
				if (remove) {
					// Remove original
					rects.erase(rects.begin() + z);
				}
				// Add new
				for (unsigned int z = 0; z < new_rects.size(); z++)
						AddRectangle(rects, new_rects[z]);
				return;
			}
		}
		rects.push_back(rect);
	}
}

// Given a vector of NSRects, return the minimum number of nonoverlapping rects that cover the same region
std::vector<NSRect> NSRectConsolidate(const std::vector<NSRect>& rects) {
	// TODO: Apparently this problem is NP-Hard, so
	// real algorithm seems to be here - rectangle dissection of axis aligned polygon:
	// https://stackoverflow.com/questions/5919298/algorithm-for-finding-the-fewest-rectangles-to-cover-a-set-of-rectangles-without
	
	// For now, take the slow and non-optimal approach - O(exp(n))?
	// 1) Add each rectangle one at a time
	// 2) See if it overlaps any other rectangle
	// 3) If it does cut up the single rectangle and get multiple non overlapping rectangles
	// 4) Add ^those rectangles one at a time (recursively)
	
	std::vector<NSRect> output;
	for (unsigned int z = 0; z < rects.size(); z++)
		AddRectangle(output, rects[z]);
	
	return output;
}

// For adding one at a time
void NSRectAddConsolidated(std::vector<NSRect>& rects, const NSRect& rect) {
	AddRectangle(rects, rect);
}
