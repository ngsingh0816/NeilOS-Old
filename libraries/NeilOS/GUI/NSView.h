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
	void SetFrame(NSRect frame);
	// Get in window coordinates
	NSRect GetAbsoluteFrame() const;
	NSRect GetAbsoluteRect(NSRect rect) const;
	
	void AddSubview(NSView* view);
	bool RemoveSubview(NSView* view);
	void RemoveSubviewAtIndex(unsigned int index);
	NSView* GetSubviewAtIndex(unsigned int index) const;
	
	NSWindow* GetWindow() const;
	NSView* GetSuperview() const;
	
	// Draws view and subviews
	void DrawRect(NSRect rect);
	// Just draws view (override this method for custom drawing)
	void Draw(NSRect rect);
	
	bool AcceptsFirstResponder() const;
	void BecomeFirstResponder();
	bool IsFirstResponder() const;
	void ResignFirstResponder();
	
	// Request screen update in local view coordinates
	void RequestUpdate(NSRect rect);
	void RequestUpdates(std::vector<NSRect> rects);
private:
	friend class NSWindow;
	
	NSView();
	
	NSRect frame;
	std::vector<NSView*> subviews;
	
	NSWindow* window = NULL;
	NSView* superview = NULL;
	
	uint32_t vertex_vbo = 0;
	uint32_t color_vbo = 0;
	NSMatrix matrix;
	graphics_vertex_array_t vao[2];
	
	bool is_first_responder = false;
};

#endif /* NSVIEW_H */
