//
//  NSCursor.h
//  product
//
//  Created by Neil Singh on 6/7/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSCURSOR_H
#define NSCURSOR_H

#include "NSImage.h"
#include "NSTypes.h"

namespace NSCursor {
	typedef enum {
		CURSOR_DEFAULT = 0,
		CURSOR_IBEAM,
		
		CURSOR_MOVE,
		CURSOR_CROSS,
		CURSOR_DRAG_AND_DROP,
		CURSOR_SCREENSHOT,
		
		CURSOR_OPEN_HAND,
		CURSOR_CLOSED_HAND,
		CURSOR_POINTING_HAND,
		
		CURSOR_RESIZE_NORTH,
		CURSOR_RESIZE_NORTHEAST,
		CURSOR_RESIZE_NORTHWEST,
		CURSOR_RESIZE_SOUTH,
		CURSOR_RESIZE_SOUTHEAST,
		CURSOR_RESIZE_SOUTHWEST,
		CURSOR_RESIZE_NORTHEAST_SOUTHWEST,
		CURSOR_RESIZE_NORTHWEST_SOUTHEAST,
		CURSOR_RESIZE_NORTH_SOUTH,
		CURSOR_RESIZE_EAST,
		CURSOR_RESIZE_WEST,
		CURSOR_RESIZE_EAST_WEST,
		
		CURSOR_RESIZE_UP,
		CURSOR_RESIZE_DOWN,
		CURSOR_RESIZE_UP_DOWN,
		CURSOR_RESIZE_LEFT,
		CURSOR_RESIZE_RIGHT,
		CURSOR_RESIZE_LEFT_RIGHT,
		
		CURSOR_ZOOM_IN,
		CURSOR_ZOOM_OUT,
		
		CURSOR_MAX
	} Cursor;
	
	NSPoint GetPosition();
	
	void SetCursor(Cursor cursor);
	void SetCursor(NSImage* image, NSPoint hotspot = NSPoint());
};

class NSCursorRegion {
public:
	NSCursorRegion(const NSRect& region, NSCursor::Cursor cursor);
	NSCursorRegion(const NSRect& region, NSImage* image, NSPoint hotspot = NSPoint());
	NSCursorRegion(const NSCursorRegion&);
	static NSCursorRegion* FromData(const uint8_t* data, uint32_t length, uint32_t* length_used=NULL);
	NSCursorRegion& operator=(const NSCursorRegion&);
	~NSCursorRegion();
	
	NSRect GetRegion() const;
	void SetRegion(const NSRect& region);
	
	NSCursor::Cursor GetCursor() const;
	void SetCursor(NSCursor::Cursor cursor);
	
	NSImage* GetImage() const;
	// Makes copy
	void SetImage(NSImage* image);
	
	NSPoint GetHotspot() const;
	void SetHotspot(const NSPoint& hotspot);
	
	bool IsActive() const;
	void SetIsActive(bool active);
	
	uint8_t* Serialize(uint32_t* length_out) const;
	
private:
	NSRect region;
	NSCursor::Cursor cursor;
	NSImage* image = NULL;
	NSPoint hotspot;
	bool active = false;
};

#endif /* NSCURSOR_H */
