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
#include "../Core/NSTypes.h"

#include <graphics/graphics.h>

#include <vector>

class NSWindow;

class NSView : public NSResponder {
public:
	static NSView* Create();
	static NSView* Create(NSRect frame);
	~NSView();
	
	NSRect GetFrame() const;
	NSRect GetBounds() const;
	virtual void SetFrame(NSRect frame);
	// Get in window coordinates
	NSRect GetAbsoluteFrame() const;
	NSRect GetAbsoluteRect(NSRect rect) const;
	
	void AddSubview(NSView* view);
	bool RemoveSubview(NSView* view);
	void RemoveSubviewAtIndex(unsigned int index);
	NSView* GetSubviewAtIndex(unsigned int index) const;
	
	NSView* GetViewAtPoint(NSPoint p);
	
	NSWindow* GetWindow() const;
	NSView* GetSuperview() const;
	
	bool IsVisible() const;
	void SetVisible(bool is);
	
	void SetNeedsDisplay();
	
	virtual bool AcceptsFirstResponder() const;
	virtual void BecomeFirstResponder();
	virtual bool IsFirstResponder() const;
	virtual void ResignFirstResponder();
	
protected:
	NSView();

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
	
	NSRect frame;
	std::vector<NSView*> subviews;
	bool visible = true;
	
	NSWindow* window = NULL;
	NSView* superview = NULL;
	
	bool is_first_responder = false;
};

#endif /* NSVIEW_H */
