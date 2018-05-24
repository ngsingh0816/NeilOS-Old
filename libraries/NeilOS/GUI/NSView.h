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
	
	NSRect GetFrame();
	void SetFrame(NSRect frame);
	
	void AddSubview(NSView* view);
	bool RemoveSubview(NSView* view);
	void RemoveSubviewAtIndex(unsigned int index);
	NSView* GetSubviewAtIndex(unsigned int index);
	
	NSWindow* GetWindow();
	NSView* GetSuperview();
	
	void DrawRect(NSRect rect);
	
	bool AcceptsFirstResponder() const;
	void BecomeFirstResponder();
	bool IsFirstResponder() const;
	void ResignFirstResponder();
private:
	friend class NSWindow;
	
	NSView();
	
	NSRect frame;
	std::vector<NSView*> subviews;
	
	NSWindow* window;
	NSView* superview;
	
	uint32_t vertex_vbo;
	uint32_t color_vbo;
	NSMatrix matrix;
	graphics_vertex_array_t vao[2];
	
	bool is_first_responder;
};

#endif /* NSVIEW_H */
