//
//  NSView.cpp
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSView.h"

#include "../Core/NSColor.h"
#include "NSWindow.h"

#include <string.h>

NSView* NSView::Create() {
	NSView* ret = new NSView();
	ret->SetFrame(NSRect());
	return ret;
}

NSView* NSView::Create(NSRect f) {
	NSView* ret = new NSView();
	ret->SetFrame(f);
	return ret;
}

NSView::NSView() {
	const float square[] = {
		0, 0,
		1, 0,
		0, 1,
		1, 1
	};
	float color[12];
	NSColor<float> c = NSColor<float>::UILighterGrayColor();
	for (int z = 0; z < 4; z++) {
		color[z*3+0] = c.r;
		color[z*3+1] = c.g;
		color[z*3+2] = c.b;
	}
	vertex_vbo = graphics_buffer_create(sizeof(square), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(vertex_vbo, square, sizeof(square));
	color_vbo = graphics_buffer_create(sizeof(color), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(color_vbo, color, sizeof(color));
	
	memset(vao, 0, sizeof(graphics_vertex_array_t) * 2);
	vao[0].bid = vertex_vbo;
	vao[0].offset = 0;
	vao[0].stride = 2 * sizeof(float);
	vao[0].type = GRAPHICS_TYPE_FLOAT2;
	vao[0].usage = GRAPHICS_USAGE_POSITION;
	vao[1].bid = color_vbo;
	vao[1].offset = 0;
	vao[1].stride = 3 * sizeof(float);
	vao[1].type = GRAPHICS_TYPE_FLOAT3;
	vao[1].usage = GRAPHICS_USAGE_COLOR;
}

NSView::~NSView() {
	for (uint32_t z = 0; z < subviews.size(); z++)
		delete subviews[z];
	
	graphics_buffer_destroy(vertex_vbo);
	graphics_buffer_destroy(color_vbo);
}

NSRect NSView::GetFrame() const {
	return frame;
}

void NSView::SetFrame(NSRect f) {
	frame = f;
	
	matrix = NSMatrix::Identity();
	matrix.Translate(frame.origin.x, frame.origin.y, 0);
	matrix.Scale(frame.size.width, frame.size.height, 1);
}

NSRect NSView::GetAbsoluteFrame() const {
	return GetAbsoluteRect(frame);
}

NSRect NSView::GetAbsoluteRect(NSRect rect) const {
	if (!superview)
		return rect;
	return NSRect(rect.origin + superview->GetAbsoluteFrame().origin, rect.size);
}

void NSView::AddSubview(NSView* view) {
	view->window = window;
	view->superview = this;

	subviews.push_back(view);
	
	view->DrawRect(view->frame);
}

bool NSView::RemoveSubview(NSView* view) {
	for (uint32_t z = 0; z < subviews.size(); z++) {
		if (subviews[z] == view) {
			delete subviews[z];
			subviews.erase(subviews.begin() + z);
			return true;
		}
	}
	return false;
}

void NSView::RemoveSubviewAtIndex(unsigned int index) {
	delete subviews[index];
	subviews.erase(subviews.begin() + index);
}

NSView* NSView::GetSubviewAtIndex(unsigned int index) const {
	return subviews[index];
}

NSWindow* NSView::GetWindow() const {
	return window;
}

NSView* NSView::GetSuperview() const {
	return superview;
}

void NSView::DrawRect(NSRect rect) {
	Draw(rect);
	
	// Draw subviews
	for (unsigned int z = 0; z < subviews.size(); z++) {
		if (subviews[z]->frame.OverlapsRect(rect)) {
			NSRect out = NSRect(rect.origin - subviews[z]->frame.origin, rect.size);
			if (NSRectClamp(out, NSRect(NSPoint(), subviews[z]->frame.size), &out))
				subviews[z]->DrawRect(out);
		}
	}
}

void NSView::Draw(NSRect rect) {
	// TODO: scissor?
	
	graphics_context_t* context = window->GetContext();
	/*graphics_clear(context, 0xFFFFFFFF, 1.0f, 0, GRAPHICS_CLEAR_COLOR | GRAPHICS_CLEAR_DEPTH,
	 0, 0, frame.size.width, frame.size.height);*/
	
	graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
	graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, vao, 2);
	
	if (window)
		window->AddUpdateRect(rect);
}

void NSView::RequestUpdate(NSRect rect) {
	std::vector<NSRect> rects = { rect };
	RequestUpdates(rects);
}

void NSView::RequestUpdates(std::vector<NSRect> rects) {
	if (!window)
		return;
	
	NSPoint offset = NSPoint();
	if (superview)
		offset = superview->GetAbsoluteFrame().origin;
	for (unsigned int z = 0; z < rects.size(); z++)
		rects[z].origin += offset;
	window->AddUpdateRects(rects);
}

bool NSView::AcceptsFirstResponder() const {
	return true;
}

void NSView::BecomeFirstResponder() {
	is_first_responder = true;
}

bool NSView::IsFirstResponder() const {
	return is_first_responder;
}

void NSView::ResignFirstResponder() {
	is_first_responder = false;
}
