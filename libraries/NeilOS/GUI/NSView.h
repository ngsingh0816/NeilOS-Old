//
//  NSView.h
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSVIEW_H
#define NSVIEW_H

#include "../Core/NSResponder.h"
#include "../Core/NSColor.h"
#include "../Core/NSCursor.h"
#include "../Core/NSImage.h"
#include "../Core/NSHandler.h"
#include "../Core/NSTypes.h"

#include "NSAnimation.h"

#include <graphics/graphics.h>

#include <functional>
#include <vector>

class NSWindow;

#define NSViewNotResizable		0
#define NSViewMinXMargin		(1 << 0)		// Min X is fixed
#define NSViewWidthSizable		(1 << 1)
#define NSViewMaxXMargin		(1 << 2)		// Max X is fixed
#define NSViewMinYMargin		(1 << 3)		// Min Y is fixed
#define NSViewHeightSizable		(1 << 4)
#define NSViewMaxYMargin		(1 << 5)		// Max Y is fixed
typedef uint32_t NSAutoResizingMask;

#define NSViewDefaultBorderColor		NSColor<float>::UIGrayColor()

class NSViewObserver;
class NSScrollView;

class NSView : public NSResponder {
public:
	static NSView* Create();
	static NSView* Create(const NSRect& frame);
	virtual NSView* Clone();
	static NSView* FromData(const uint8_t* data, uint32_t length, uint32_t* length_used=NULL);
	virtual uint8_t* Serialize(uint32_t* length_out) const;
	virtual ~NSView();
	
	NSView& operator =(const NSView& v);
	
	NSRect GetFrame() const;
	NSRect GetBounds() const;
	virtual void SetFrame(const NSRect& frame);
	void SetFrameOrigin(const NSPoint& p);
	void SetFrameSize(const NSSize& size);
	// Get in window coordinates
	NSRect GetAbsoluteFrame() const;
	NSRect GetAbsoluteRect(const NSRect& rect) const;
	
	void AddSubview(NSView* view);
	bool RemoveSubview(NSView* view);
	void RemoveSubviewAtIndex(unsigned int index);
	NSView* GetSubviewAtIndex(unsigned int index) const;
	void SetSubviews(const std::vector<NSView*>& views);
	unsigned int GetNumberOfSubviews() const;
	unsigned int GetIndexOfSubview(NSView* view) const;
	
	virtual NSView* GetViewAtPoint(const NSPoint& p);
	// Local coordinates
	virtual bool ContainsPoint(const NSPoint& p) const;
	
	const std::function<void(NSView*)>& GetAction() const;
	void SetAction(const std::function<void(NSView*)>& action);
	
	virtual void SetViewContainer(NSViewContainer* container);
	NSView* GetSuperview() const;
	NSViewContainer* GetViewContainer() const;
	NSScrollView* GetEnclosingScrollView() const;
	
	bool IsVisible() const;
	void SetVisible(bool is);
	
	NSAutoResizingMask GetResizingMask() const;
	void SetResizingMask(NSAutoResizingMask mask);
	
	void SetNeedsDisplay();
	void SetNeedsDisplay(const NSRect& rect);
	void SetNeedsDisplay(const std::vector<NSRect>& rects);
	void DrawRect(const NSRect& rect);
	
	virtual bool AcceptsFirstResponder() const;
	virtual void BecomeFirstResponder();
	virtual bool IsFirstResponder() const;
	virtual void ResignFirstResponder();
	
	void AddObserver(NSViewObserver* observer);
	void RemoveObserver(NSViewObserver* observer);
	
	void AddCursorRegion(const NSCursorRegion& region);
	// Removes all regions that overlap the rect
	void RemoveCursorRegions(const NSRect& region);
	void RemoveAllCursorRegions();
	uint32_t GetNumberOfCursorRegions();
	NSCursorRegion* GetCursorRegionAtIndex(uint32_t index);
	// Local view coordinates
	virtual NSCursorRegion* GetCursorRegionAtPoint(const NSPoint& p);
	
	uint32_t GetNumberOfAnimations() const;
	NSAnimation* GetAnimationAtIndex(uint32_t index);
	
	// Value is scale of 0-1 of how completed the animation is
	NSAnimation* Animate(const std::function<void(NSView*, const NSAnimation*)>& anim, NSTimeInterval duration,
						NSTimeInterval delay = 0,
						const std::function<void(NSView*, const NSAnimation*)>& completion = nullptr);
	
	virtual void MouseDown(NSEventMouse* event) override;
	virtual void MouseMoved(NSEventMouse* event) override;
	virtual void MouseDragged(NSEventMouse* event) override;
	virtual void MouseUp(NSEventMouse* event) override;
	virtual void MouseScrolled(NSEventMouse* event) override;
	virtual void KeyDown(NSEventKey* event) override;
	virtual void KeyUp(NSEventKey* event) override;
	
	// Drawing
	static void SetupContext(graphics_context_t* context, NSSize size);
	static void BufferSquare(uint32_t bid);
	static void BufferOval(uint32_t bid, uint32_t num_vertices);
	// Num vertices should be divisible by 4 after subtracting 2 (ex: 38)
	static void BufferRoundedRect(uint32_t bid, const NSSize& size, float border_radius_x, float border_radius_y,
								  uint32_t num_vertices);
#define NSVIEW_ROUNDED_UPPER_LEFT	(1 << 0)
#define NSVIEW_ROUNDED_UPPER_RIGHT	(1 << 1)
#define NSVIEW_ROUNDED_BOTTOM_LEFT	(1 << 2)
#define NSVIEW_ROUNDED_BOTTOM_RIGHT	(1 << 3)
	// num vertices = c * x + 2 + (4 - c) where c = # of corners, and x is an integer
	static void BufferRoundedRect(uint32_t bid, const NSSize& size, float borders[4], int mask, uint32_t num_vertices);
	// Num verticies should be divisible by 2 after subtracting 2 (ex: 18)
	static void BufferHorizontalCurvedRect(uint32_t bid, const NSSize& size, uint32_t num_vertices);
	static void BufferVerticalCurvedRect(uint32_t bid, const NSSize& size, uint32_t num_vertices);
	static void BufferColor(uint32_t bid, NSColor<float> color, uint32_t num_vertices);
	static uint32_t CreateImageBuffer(NSImage* image, NSSize* size_out);
#define NSVIEW_CHECKMARK_VERTICES		6
	static void BufferCheckmark(uint32_t bid);		// triangle_strip
	
protected:
	NSView();
	
	// Called when buffers need updating (frame resizing)
	virtual void UpdateVBO();
	// Called when array objects need updated (buffer recreated - e.g. changed color)
	virtual void UpdateVAO();

	// Just draws view (override this method for custom drawing)
	// Rect in local view coordinates
	virtual void Draw(graphics_context_t* context, const NSRect& rect);
	
	// Request screen update in local view coordinates
	void RequestUpdate(const NSRect& rect);
	void RequestUpdates(const std::vector<NSRect>& rects);
	
	uint32_t vertex_vbo = 0;
	uint32_t color_vbo = 0;
	graphics_vertex_array_t vao[3];
	bool requests_updates = true;
private:
	friend class NSMenuViewItem;
	
	void SetSubview(NSView* view);
	
	NSRect PrepareDraw(graphics_context_t* context, const NSRect& rect);
	void FinishDraw(graphics_context_t* context, const NSRect& rect);
	
	void DrawSubview(NSView* subview, NSRect rect);
	
	void Resize(NSSize old_sv_size, NSSize sv_size);
	
	void CheckCursorRegions(const NSPoint& p);
	
	NSRect frame;
	std::vector<NSView*> subviews;
	bool visible = true;
	NSAutoResizingMask resizing_mask = NSViewNotResizable;
	std::vector<NSViewObserver*> observers;
	std::vector<NSCursorRegion> cursor_regions;
	
	NSViewContainer* container = NULL;
	NSView* superview = NULL;
	std::function<void(NSView*)> action;
	
	bool mouse_is_down = false;
	bool is_first_responder = false;
	std::vector<NSRect> dirty_rects;
	NSHandler* display_handler = NULL;
	
	std::vector<NSAnimation*> animations;
};

class NSViewObserver {
public:
	virtual void FrameUpdated() {};
	virtual void SubviewsUpdated() {};
	virtual void BecomeFirstResponder() {};
	virtual void ResignFirstResponder() {};
	virtual void UpdateRequested(const std::vector<NSRect>& rects) {};
};

#endif /* NSVIEW_H */
