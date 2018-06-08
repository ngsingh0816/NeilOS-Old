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
#include "../Core/NSImage.h"
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

class NSView : public NSResponder {
public:
	static NSView* Create();
	static NSView* Create(NSRect frame);
	~NSView();
	
	NSRect GetFrame() const;
	NSRect GetBounds() const;
	virtual void SetFrame(NSRect frame);
	void SetFrameOrigin(NSPoint p);
	void SetFrameSize(NSSize size);
	// Get in window coordinates
	NSRect GetAbsoluteFrame() const;
	NSRect GetAbsoluteRect(NSRect rect) const;
	
	void AddSubview(NSView* view);
	bool RemoveSubview(NSView* view);
	void RemoveSubviewAtIndex(unsigned int index);
	NSView* GetSubviewAtIndex(unsigned int index) const;
	
	NSView* GetViewAtPoint(NSPoint p);
	// Local coordinates
	virtual bool ContainsPoint(NSPoint p) const;
	
	NSWindow* GetWindow() const;
	NSView* GetSuperview() const;
	
	bool IsVisible() const;
	void SetVisible(bool is);
	
	NSAutoResizingMask GetResizingMask() const;
	void SetResizingMask(NSAutoResizingMask mask);
	
	void SetNeedsDisplay();
	
	virtual bool AcceptsFirstResponder() const;
	virtual void BecomeFirstResponder();
	virtual bool IsFirstResponder() const;
	virtual void ResignFirstResponder();
	
	uint32_t GetNumberOfAnimations() const;
	NSAnimation* GetAnimationAtIndex(uint32_t index);
	
	// Value is scale of 0-1 of how completed the animation is
	NSAnimation* Animate(const std::function<void(NSView*, const NSAnimation*)>& anim, NSTimeInterval duration,
						NSTimeInterval delay = 0,
						const std::function<void(NSView*, const NSAnimation*)>& completion = nullptr);
	
protected:
	// Num vertices should be divisible by 4 after subtracting 2 (ex: 38)
	static uint32_t CreateSquareBuffer();
	static uint32_t CreateOvalBuffer(uint32_t num_vertices);
	static uint32_t CreateRoundedRectBuffer(NSSize size, float border_radius_x, float border_radius_y,
											uint32_t num_vertices);
	static uint32_t CreateColorBuffer(NSColor<float> color, uint32_t num_vertices);
	static void BufferColor(uint32_t bid, NSColor<float> color, uint32_t num_vertices);
	static uint32_t CreateImageBuffer(NSImage* image, NSSize* size_out);
	NSView();
	
	// Called when added to a window
	virtual void WindowWasSet();
	
	// Called when buffers need updating (frame resizing)
	virtual void UpdateVBO();
	// Called when array objects need updated (buffer recreated - e.g. changed color)
	virtual void UpdateVAO();

	// Just draws view (override this method for custom drawing)
	// Rect in local view coordinates
	virtual void Draw(NSRect rect);
	
	// Request screen update in local view coordinates
	void RequestUpdate(NSRect rect);
	void RequestUpdates(std::vector<NSRect> rects);
	
	uint32_t vertex_vbo = 0;
	uint32_t color_vbo = 0;
	graphics_vertex_array_t vao[3];
	
private:
	friend class NSWindow;
		
	// Draws view and subviews
	void DrawRect(NSRect rect);
	void DrawSubview(NSView* subview, NSRect rect);
	
	void RemoveFromWindow(NSView* view);
	
	void Resize(NSView* view, NSSize old_sv_size, NSSize sv_size);
	
	void CheckCursorRegions(NSPoint p);
	
	NSRect frame;
	std::vector<NSView*> subviews;
	bool visible = true;
	NSAutoResizingMask resizing_mask = NSViewNotResizable;
	
	NSWindow* window = NULL;
	NSView* superview = NULL;
	
	bool is_first_responder = false;
	
	std::vector<NSAnimation*> animations;
};

#endif /* NSVIEW_H */
