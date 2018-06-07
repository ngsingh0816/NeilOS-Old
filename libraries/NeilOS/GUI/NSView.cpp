//
//  NSView.cpp
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSView.h"

#include "../Core/NSColor.h"
#include "../Core/NSHandler.h"
#include "../Core/NSTimer.h"
#include "NSWindow.h"

#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define ANIMATION_FRAMERATE		60.0

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
	vertex_vbo = NSView::CreateSquareBuffer();
	color_vbo = NSView::CreateColorBuffer(NSColor<float>::UILighterGrayColor(), 4);
	
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
	
	for (unsigned int z = 0; z < animations.size(); z++) {
		animations[z]->Cancel();
		delete animations[z];
	}
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
	
	if (old_frame.size != frame.size)
		UpdateVBO();
	
	if (superview) {
		superview->DrawRect(old_frame);
		superview->DrawRect(frame);
	} else
		DrawRect(GetBounds());
}

void NSView::SetFrameOrigin(NSPoint p) {
	SetFrame(NSRect(p, frame.size));
}

void NSView::SetFrameSize(NSSize size) {
	SetFrame(NSRect(frame.origin, size));
}

NSRect NSView::GetAbsoluteFrame() const {
	return GetAbsoluteRect(NSRect(NSPoint(), frame.size));
}

NSRect NSView::GetAbsoluteRect(NSRect rect) const {
	if (!superview)
		return rect;
	return NSRect(rect.origin + superview->GetAbsoluteFrame().origin + frame.origin, rect.size);
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
	if (!GetBounds().ContainsPoint(p))
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

// Local coordinates
bool NSView::ContainsPoint(NSPoint p) const {
	return GetBounds().ContainsPoint(p);
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
	
	NSRect scissor = (GetAbsoluteRect(rect) * window->GetBackingScaleFactor()).IntegerRect();
	graphics_scissor_rect_set(&window->context, scissor.origin.x, scissor.origin.y,
							  scissor.size.width, scissor.size.height);
	
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

// Called when buffers need updating (frame resizing)
void NSView::UpdateVBO() {
	UpdateVAO();
}

// Called when array objects need updated (buffer recreated - e.g. changed color)
void NSView::UpdateVAO() {
	vao[0].bid = vertex_vbo;
	vao[1].bid = color_vbo;
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

uint32_t NSView::GetNumberOfAnimations() const {
	return animations.size();
}

NSAnimation* NSView::GetAnimationAtIndex(uint32_t index) {
	return animations[index];
}

// Value is scale of 0-1 of how completeed the animation is
NSAnimation* NSView::Animate(const std::function<void(NSView*, const NSAnimation* value)>& anim, NSTimeInterval duration,
							NSTimeInterval delay, const std::function<void(NSView*, const NSAnimation*)>& completion) {
	auto la = [anim, this](const NSAnimation* a) {
		anim(this, a);
	};
	auto lc = [completion, this](const NSAnimation* a, bool canceled) {
		if (!canceled && completion)
			completion(this, a);
		for (unsigned int z = 0; z < animations.size(); z++) {
			if (animations[z] == a) {
				animations.erase(animations.begin() + z);
				delete animations[z];
				break;
			}
		}
	};
	NSAnimation* animation = new NSAnimation(la, lc, duration);
	animations.push_back(animation);
	animation->Start(delay);
	return animation;
}

uint32_t NSView::CreateSquareBuffer() {
	const float square[] = {
		0, 0,
		1, 0,
		0, 1,
		1, 1
	};
	uint32_t ret = graphics_buffer_create(sizeof(square), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(ret, square, sizeof(square));
	
	return ret;
}

uint32_t NSView::CreateOvalBuffer(uint32_t num_vertices) {
	float vertices[num_vertices * 2];
	NSSize size = NSSize(1, 1);
	vertices[0] = size.width / 2;
	vertices[1] = size.height / 2;
	for (uint32_t z = 0; z < num_vertices - 1; z++) {
		float angle = (float(z) / (num_vertices - 2)) * 2 * M_PI;
		if (z == num_vertices - 2)
			angle = 0;
		float x = cosf(angle) * size.width / 2 + size.width / 2;
		float y = sinf(angle) * size.height / 2 + size.height / 2;
		vertices[z*2 + 2] = x;
		vertices[z*2 + 3] = y;
	}
	
	uint32_t ret = graphics_buffer_create(sizeof(vertices), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(ret, vertices, sizeof(vertices));
	
	return ret;
}

uint32_t NSView::CreateRoundedRectBuffer(NSSize size, float border_radius_x, float border_radius_y,
										 uint32_t num_vertices) {
	float vertices[num_vertices * 2];
	vertices[0] = size.width / 2;
	vertices[1] = size.height / 2;
	int num = (num_vertices - 2) / 4;
	uint32_t pos = 2;
	for (int z = 0; z < num; z++) {
		float angle = (180.0f - (float(z) / (num - 1)) * 90.0f) / 180.0f * M_PI;
		float x = cosf(angle) * border_radius_x + border_radius_x;
		float y = -sinf(angle) * border_radius_y + border_radius_y;
		vertices[pos++] = x;
		vertices[pos++] = y;
	}
	for (int z = 0; z < num; z++) {
		float angle = (90.0f - (float(z) / (num - 1)) * 90.0f) / 180.0f * M_PI;
		float x = cosf(angle) * border_radius_x + size.width - border_radius_x;
		float y = -sinf(angle) * border_radius_y + border_radius_y;
		vertices[pos++] = x;
		vertices[pos++] = y;
	}
	for (int z = 0; z < num; z++) {
		float angle = (-(float(z) / (num - 1)) * 90.0f) / 180.0f * M_PI;
		float x = cosf(angle) * border_radius_x + size.width - border_radius_x;
		float y = -sinf(angle) * border_radius_y + size.height - border_radius_y;
		vertices[pos++] = x;
		vertices[pos++] = y;
	}
	for (int z = 0; z < num; z++) {
		float angle = (-90.0f -(float(z) / (num - 1)) * 90.0f) / 180.0f * M_PI;
		float x = cosf(angle) * border_radius_x + border_radius_x;
		float y = -sinf(angle) * border_radius_y + size.height - border_radius_y;
		vertices[pos++] = x;
		vertices[pos++] = y;
	}
	vertices[pos++] = 0;
	vertices[pos++] = border_radius_y;
	
	uint32_t ret = graphics_buffer_create(sizeof(vertices), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(ret, vertices, sizeof(vertices));
	
	return ret;
}

uint32_t NSView::CreateColorBuffer(NSColor<float> c, uint32_t num_vertices) {
	uint32_t ret = graphics_buffer_create(num_vertices * sizeof(float) * 4, GRAPHICS_BUFFER_STATIC);
	BufferColor(ret, c, num_vertices);
	return ret;
}

void NSView::BufferColor(uint32_t bid, NSColor<float> c, uint32_t num_vertices) {
	float color[num_vertices * 4];
	for (uint32_t z = 0; z < num_vertices; z++) {
		color[z*4+0] = c.r;
		color[z*4+1] = c.g;
		color[z*4+2] = c.b;
		color[z*4+3] = c.a;
	}
	graphics_buffer_data(bid, color, sizeof(color));
}

uint32_t NSView::CreateImageBuffer(NSImage* image, NSSize* size_out) {
	if (!image) {
		if (size_out)
			*size_out = NSSize();
		return 0;
	}
	if (size_out)
		*size_out = image->GetScaledSize();
	int w = int(image->GetSize().width + 0.5);
	int h = int(image->GetSize().height + 0.5);
	if (w == 0 || h == 0) {
		if (size_out)
			*size_out = NSSize();
		return 0;
	}
	uint32_t img_vbo = graphics_buffer_create(w, h, GRAPHICS_BUFFER_STATIC, GRAPHICS_FORMAT_A8R8G8B8);
	graphics_buffer_data(img_vbo, image->GetPixelData(), w, h, w * h * 4);
	
	return img_vbo;
}
