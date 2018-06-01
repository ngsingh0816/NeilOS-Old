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
	float color[16];
	NSColor<float> c = NSColor<float>::UILighterGrayColor();
	for (int z = 0; z < 4; z++) {
		color[z*4+0] = c.r;
		color[z*4+1] = c.g;
		color[z*4+2] = c.b;
		color[z*4+3] = c.a;
	}
	vertex_vbo = graphics_buffer_create(sizeof(square), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(vertex_vbo, square, sizeof(square));
	color_vbo = graphics_buffer_create(sizeof(color), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(color_vbo, color, sizeof(color));
	
	memset(vao, 0, sizeof(graphics_vertex_array_t) * 3);
	vao[0].bid = vertex_vbo;
	vao[0].offset = 0;
	vao[0].stride = 2 * sizeof(float);
	vao[0].type = GRAPHICS_TYPE_FLOAT2;
	vao[0].usage = GRAPHICS_USAGE_POSITION;
	vao[1].bid = color_vbo;
	vao[1].offset = 0;
	vao[1].stride = 4 * sizeof(float);
	vao[1].type = GRAPHICS_TYPE_FLOAT4;
	vao[1].usage = GRAPHICS_USAGE_COLOR;
}

NSView::~NSView() {
	for (uint32_t z = 0; z < subviews.size(); z++)
		delete subviews[z];
	
	if (vertex_vbo)
		graphics_buffer_destroy(vertex_vbo);
	if (color_vbo)
		graphics_buffer_destroy(color_vbo);
}

NSRect NSView::GetFrame() const {
	return frame;
}

NSRect NSView::GetBounds() const {
	return NSRect(NSPoint(), frame.size);
}

void NSView::SetFrame(NSRect f) {
	NSRect old_frame = frame;
	frame = f;
	
	if (superview) {
		superview->DrawRect(old_frame);
		superview->DrawRect(frame);
	} else
		DrawRect(GetBounds());
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
	
	DrawSubview(view, view->frame);
}

void NSView::RemoveFromWindow(NSView* view) {
	if (view == window->down_view)
		window->down_view = NULL;
	if (view == window->content_view)
		window->content_view = NULL;
	if (view == window->first_responder)
		window->MakeFirstResponder(window->content_view);
}

bool NSView::RemoveSubview(NSView* view) {
	for (uint32_t z = 0; z < subviews.size(); z++) {
		if (subviews[z] == view) {
			NSRect rect = view->frame;
			RemoveFromWindow(view);
			delete subviews[z];
			subviews.erase(subviews.begin() + z);
			DrawRect(rect);
			return true;
		}
	}
	return false;
}

void NSView::RemoveSubviewAtIndex(unsigned int index) {
	NSRect rect = subviews[index]->frame;
	RemoveFromWindow(subviews[index]);
	delete subviews[index];
	subviews.erase(subviews.begin() + index);
	DrawRect(rect);
}

NSView* NSView::GetSubviewAtIndex(unsigned int index) const {
	return subviews[index];
}

NSView* NSView::GetViewAtPoint(NSPoint p) {
	if (!frame.ContainsPoint(p))
		return NULL;
	
	for (unsigned int z = subviews.size() - 1; z != (unsigned int)-1; z--) {
		if (!subviews[z]->visible)
			continue;
		NSView* ret = subviews[z]->GetViewAtPoint(p - subviews[z]->frame.origin);
		if (ret)
			return ret;
	}
	
	return this;
}

NSWindow* NSView::GetWindow() const {
	return window;
}

NSView* NSView::GetSuperview() const {
	return superview;
}

bool NSView::IsVisible() const {
	return visible;
}

void NSView::SetVisible(bool is) {
	if (visible == is)
		return;
	
	visible = is;
	if (!visible && superview)
		superview->DrawRect(frame);
	else
		DrawRect(GetBounds());
}

void NSView::DrawRect(NSRect rect) {
	if (!window || !visible)
		return;
	
	Draw(rect);
	// Reset some things
	NSMatrix m = NSMatrix::Identity();
	graphics_transform_set(&window->context, GRAPHICS_TRANSFORM_WORLD, m);
	graphics_transform_set(&window->context, GRAPHICS_TRANSFORM_VIEW, m);
	graphics_texturestate_seti(&window->context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
	
	// Draw subviews
	for (unsigned int z = 0; z < subviews.size(); z++)
		DrawSubview(subviews[z], rect);
}

void NSView::DrawSubview(NSView* subview, NSRect rect) {
	if (!window)
		return;
	
	// TODO: scissor subviews?
	if (subview->visible && subview->frame.OverlapsRect(rect)) {
		NSRect out = NSRect(rect.origin - subview->frame.origin, rect.size);
		if (NSRectClamp(out, subview->GetBounds(), &out)) {
			
			NSMatrix m = NSMatrix::Identity();
			NSRect trans = subview->GetAbsoluteFrame();
			m.Translate(trans.origin.x, trans.origin.y, 0);
			graphics_transform_set(&window->context, GRAPHICS_TRANSFORM_VIEW, m);
			subview->DrawRect(out);
			
			// Reset some things
			m = NSMatrix::Identity();
			graphics_transform_set(&window->context, GRAPHICS_TRANSFORM_WORLD, m);
			graphics_transform_set(&window->context, GRAPHICS_TRANSFORM_VIEW, m);
			graphics_texturestate_seti(&window->context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
		}
	}
}

void NSView::Draw(NSRect rect) {
	if (!window)
		return;
	graphics_context_t* context = window->GetContext();
	if (!context)
		return;
	
	NSMatrix matrix = NSMatrix::Identity();
	matrix.Translate(frame.origin.x, frame.origin.y, 0);
	matrix.Scale(frame.size.width, frame.size.height, 1);
	graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
	graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, vao, 2);
	
	if (window)
		window->AddUpdateRect(rect);
}

void NSView::SetNeedsDisplay() {
	if (superview)
		superview->DrawRect(frame);
	else
		DrawRect(frame);
}

void NSView::RequestUpdate(NSRect rect) {
	std::vector<NSRect> rects = { rect };
	RequestUpdates(rects);
}

void NSView::RequestUpdates(std::vector<NSRect> rects) {
	if (!window)
		return;
	
	NSPoint offset = GetAbsoluteFrame().origin;
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
