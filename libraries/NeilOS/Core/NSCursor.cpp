//
//  NSCursor.cpp
//  product
//
//  Created by Neil Singh on 6/7/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSCursor.h"
#include "Events/Application/NSEventApplicationSetCursor.cpp"

namespace NSApplication {
	NSPoint GetMousePosition();
}

NSPoint NSCursor::GetPosition() {
	return NSApplication::GetMousePosition();
}

void NSCursor::SetCursor(Cursor cursor) {
	auto e = NSEventApplicationSetCursor::Create(cursor);
	if (!e)
		return;
	NSApplication::SendEvent(e);
	delete e;
}

void NSCursor::SetCursor(NSImage* image, NSPoint hotspot) {
	auto e = NSEventApplicationSetCursor::Create(image, hotspot);
	if (!e)
		return;
	NSApplication::SendEvent(e);
	delete e;
}

NSCursorRegion::NSCursorRegion(const NSRect& r, NSCursor::Cursor c) {
	region = r;
	cursor = c;
}

NSCursorRegion::NSCursorRegion(const NSRect& r, NSImage* i, NSPoint h) {
	region = r;
	image = new NSImage(i->GetPixelData(), i->GetSize(), 32);
	hotspot = h;
	cursor = NSCursor::CURSOR_MAX;
}

NSCursorRegion::NSCursorRegion(const NSCursorRegion& c) {
	*this = c;
}

NSCursorRegion& NSCursorRegion::operator=(const NSCursorRegion& c) {
	if (this == &c)
		return *this;
	
	region = c.region;
	cursor = c.cursor;
	if (c.image)
		image = new NSImage(c.image->GetPixelData(), c.image->GetSize(), 32);
	hotspot = c.hotspot;
	
	return *this;
}

NSCursorRegion* NSCursorRegion::FromData(const uint8_t* data, uint32_t length, uint32_t* length_used) {
	if (length < sizeof(float) * 6 + sizeof(NSCursor::Cursor) + sizeof(uint32_t) * 2)
		return NULL;
	
	uint32_t pos = 0;
	NSRect region = NSRect::FromData(data, sizeof(float) * 4);
	pos += sizeof(float) * 4;
	NSPoint hotspot = NSPoint::FromData(&data[pos], sizeof(float) * 2);
	pos += sizeof(float) * 2;
	NSCursor::Cursor cursor;
	memcpy(&cursor, &data[pos], sizeof(NSCursor::Cursor));
	pos += sizeof(NSCursor::Cursor);
	uint32_t img_len = 0;
	memcpy(&img_len, &data[pos], sizeof(uint32_t));
	pos += sizeof(uint32_t);
	uint32_t img_name_len = 0;
	memcpy(&img_name_len, &data[pos], sizeof(uint32_t));
	pos += sizeof(uint32_t);
	if (pos + img_name_len >= length)
		return NULL;
	
	NSImage* image = NULL;
	if (img_name_len != 0) {
		char buf[img_name_len + 1];
		memcpy(buf, &data[pos], img_name_len);
		pos += img_name_len;
		buf[img_name_len] = 0;
		uint8_t* d = new uint8_t[img_len];
		if (NSEventResource::GetSharedResource(buf, d, img_len))
			image = new NSImage(d, img_len);
		delete[] d;
		NSEventResource::DeleteSharedResource(buf);
	}
	
	if (length_used)
		*length_used = pos;
	if (image)
		return new NSCursorRegion(region, image, hotspot);
	return new NSCursorRegion(region, cursor);
}

uint8_t* NSCursorRegion::Serialize(uint32_t* length_out) const {
#define copy(x, len) { memcpy(&buffer[pos], x, (len)); pos += (len); }
#define copy_serialize(x, l) { uint8_t* temp = x.Serialize(NULL); memcpy(&buffer[pos], temp, (l));\
								pos += (l); delete[] temp; }

	
	uint32_t total_length = sizeof(float) * 4;		// region
	total_length += sizeof(float) * 2;				// hotspot
	total_length += sizeof(NSCursor::Cursor);		// cursor
	uint32_t image_length = 0;
	std::string image_name;
	if (image) {
		// Don't abandon if representing image fails
		uint8_t* data = image->RepresentationUsingType(NSImage::NSImageTypePNG, &image_length);
		if (data) {
			image_name = NSEventResource::CreateSharedResource(data, image_length);
			delete[] data;
		}
	}
	total_length += sizeof(uint32_t);				// image length
	total_length += sizeof(uint32_t);				// image name length
	total_length += image_name.length();
	
	uint8_t* buffer = new uint8_t[total_length];
	uint32_t pos = 0;
	
	copy_serialize(region, sizeof(float) * 4);
	copy_serialize(hotspot, sizeof(float) * 2);
	copy(&cursor, sizeof(NSCursor::Cursor));
	uint32_t img_name_length = image_name.length();
	copy(&image_length, sizeof(uint32_t));
	copy(&img_name_length, sizeof(uint32_t));
	copy(image_name.c_str(), img_name_length);
	
#undef copy
#undef copy_serialize
	
	if (length_out)
		*length_out = total_length;
	return buffer;
}

NSCursorRegion::~NSCursorRegion() {
	if (image)
		delete image;
}

NSRect NSCursorRegion::GetRegion() const {
	return region;
}

void NSCursorRegion::SetRegion(const NSRect& r) {
	region = r;
}

NSCursor::Cursor NSCursorRegion::GetCursor() const {
	return cursor;
}

void NSCursorRegion::SetCursor(NSCursor::Cursor c) {
	cursor = c;
}

NSImage* NSCursorRegion::GetImage() const {
	return image;
}

// Makes copy
void NSCursorRegion::SetImage(NSImage* i) {
	if (image)
		delete image;
	image = new NSImage(i->GetPixelData(), i->GetSize(), 32);
	cursor = NSCursor::CURSOR_MAX;
}

NSPoint NSCursorRegion::GetHotspot() const {
	return hotspot;
}

void NSCursorRegion::SetHotspot(const NSPoint& h) {
	hotspot = h;
}

bool NSCursorRegion::IsActive() const {
	return active;
}

void NSCursorRegion::SetIsActive(bool a) {
	active = a;
}
