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

#define SQUARE_VERTICES			4

NSView* NSView::Create() {
	NSView* ret = new NSView();
	ret->SetFrame(NSRect());
	return ret;
}

NSView* NSView::Create(const NSRect& f) {
	NSView* ret = new NSView();
	ret->SetFrame(f);
	return ret;
}

NSView::NSView() {
	vertex_vbo = graphics_buffer_create(sizeof(float) * SQUARE_VERTICES * 2, GRAPHICS_BUFFER_STATIC);
	BufferSquare(vertex_vbo);
	color_vbo = graphics_buffer_create(sizeof(float) * 4 * SQUARE_VERTICES, GRAPHICS_BUFFER_DYNAMIC);
	BufferColor(color_vbo, NSColor<float>::UILighterGrayColor(), SQUARE_VERTICES);
	
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

void NSView::SetFrame(const NSRect& f) {
	NSRect old_frame = frame;
	frame = f;
	
	if (old_frame.size != frame.size) {
		UpdateVBO();
		for (unsigned int z = 0; z < subviews.size(); z++)
			Resize(subviews[z], old_frame.size, frame.size);
	}
	
	for (auto& observer : observers)
		observer->FrameUpdated();
	
	if (superview) {
		superview->DrawRect(old_frame);
		superview->DrawRect(frame);
	} else
		DrawRect(GetBounds());
}

void NSView::Resize(NSView* view, NSSize old_sv_size, NSSize sv_size) {
	NSAutoResizingMask mask = view->resizing_mask;
	if (mask == NSViewNotResizable)
		return;
	
	bool width = (mask & NSViewWidthSizable) != 0, height = (mask & NSViewHeightSizable) != 0;
	bool x_min = (mask & NSViewMinXMargin) != 0, x_max = (mask & NSViewMaxXMargin) != 0;
	bool y_min = (mask & NSViewMinYMargin) != 0, y_max = (mask & NSViewMaxYMargin) != 0;
	
	NSRect frame = view->frame;
	NSRect old_frame = view->frame;
	if (width)
		frame.size.width = (frame.size.width / old_sv_size.width) * sv_size.width;
	if (mask & NSViewHeightSizable)
		frame.size.height = (frame.size.height / old_sv_size.height) * sv_size.height;
	
	if (x_min && x_max && width) {
		float dist = old_sv_size.width - (old_frame.origin.x + old_frame.size.width);
		frame.size.width = sv_size.width - dist - frame.origin.x;
	} else if (x_min) {
		// Do nothing (implicit)
	} else if (x_max) {
		float dist = old_sv_size.width - (old_frame.origin.x + old_frame.size.width);
		frame.origin.x = sv_size.width - frame.size.width - dist;
	}
	
	if (y_min && y_max && height) {
		float dist = old_sv_size.height - (old_frame.origin.y + old_frame.size.height);
		frame.size.height = sv_size.height - dist - frame.origin.y;
	} else if (y_min) {
		// Do nothing (implicit)
	} else if (y_max) {
		float dist = old_sv_size.height - (old_frame.origin.y + old_frame.size.height);
		frame.origin.y = sv_size.height - frame.size.height - dist;
	}
	
	view->SetFrame(frame);
}

void NSView::SetFrameOrigin(const NSPoint& p) {
	SetFrame(NSRect(p, frame.size));
}

void NSView::SetFrameSize(const NSSize& size) {
	SetFrame(NSRect(frame.origin, size));
}

NSRect NSView::GetAbsoluteFrame() const {
	return GetAbsoluteRect(NSRect(NSPoint(), frame.size));
}

NSRect NSView::GetAbsoluteRect(const NSRect& rect) const {
	if (!superview)
		return rect;
	return NSRect(rect.origin + superview->GetAbsoluteFrame().origin + frame.origin, rect.size);
}

void NSView::AddSubview(NSView* view) {
	view->SetWindow(window);
	view->SetContext(context);
	view->superview = this;
	view->WindowWasSet();

	subviews.push_back(view);
	
	for (auto& observer : observers)
		observer->SubviewsUpdated();
	
	DrawSubview(view, view->frame);
}

void NSView::SetWindow(NSWindow* w) {
	window = w;
	context = window ? w->GetContext() : NULL;
	for (auto& s : subviews)
		s->SetWindow(w);
}

void NSView::SetContext(graphics_context_t* c) {
	context = c;
	for (auto& s : subviews)
		s->SetContext(c);
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
			
			for (auto& observer : observers)
				observer->SubviewsUpdated();
			
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
	
	for (auto& observer : observers)
		observer->SubviewsUpdated();
	
	DrawRect(rect);
}

NSView* NSView::GetSubviewAtIndex(unsigned int index) const {
	return subviews[index];
}

NSView* NSView::GetViewAtPoint(const NSPoint& p) {
	if (!ContainsPoint(p))
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
bool NSView::ContainsPoint(const NSPoint& p) const {
	return GetBounds().ContainsPoint(p);
}

NSWindow* NSView::GetWindow() const {
	return window;
}

void NSView::WindowWasSet() {
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

NSAutoResizingMask NSView::GetResizingMask() const {
	return resizing_mask;
}

void NSView::SetResizingMask(NSAutoResizingMask mask) {
	resizing_mask = mask;
}

void NSView::DrawRect(const NSRect& rect) {
	if (!context || !visible)
		return;
	
	needs_display = false;
	
	PrepareDraw(rect);
	Draw(rect);
	FinishDraw();
	
	// Draw subviews
	for (unsigned int z = 0; z < subviews.size(); z++)
		DrawSubview(subviews[z], rect);
}

void NSView::DrawSubview(NSView* subview, NSRect rect) {
	if (subview->visible && subview->frame.OverlapsRect(rect)) {
		NSRect out = NSRect(rect.origin - subview->frame.origin, rect.size);
		if (NSRectClamp(out, subview->GetBounds(), &out))
			subview->DrawRect(out);
	}
}

void NSView::PrepareDraw(const NSRect& rect) {
	float bsf = window ? window->GetBackingScaleFactor() : 1;
	NSRect scissor = (GetAbsoluteRect(rect) * bsf).IntegerRect();
	graphics_scissor_rect_set(context, scissor.origin.x, scissor.origin.y,
							  scissor.size.width, scissor.size.height);
	
	NSMatrix m = NSMatrix::Identity();
	NSRect trans = GetAbsoluteFrame();
	m.Translate(trans.origin.x, trans.origin.y, 0);
	graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, m);
}

void NSView::FinishDraw() {
	// Reset some things
	NSMatrix m = NSMatrix::Identity();
	graphics_transform_set(context, GRAPHICS_TRANSFORM_WORLD, m);
	graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, m);
	graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
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

void NSView::Draw(const NSRect& rect) {
	if (!context)
		return;
	
	NSMatrix matrix = NSMatrix::Identity();
	matrix.Translate(frame.origin.x, frame.origin.y, 0);
	matrix.Scale(frame.size.width, frame.size.height, 1);
	graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
	graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, vao, 2);
	
	RequestUpdate(rect);
}

void NSView::SetNeedsDisplay() {
	std::vector<NSRect> rects = { GetBounds() };
	SetNeedsDisplay(rects);
}

void NSView::SetNeedsDisplay(const NSRect& rect) {
	std::vector<NSRect> rects = { rect };
	SetNeedsDisplay(rects);
}

void NSView::SetNeedsDisplay(const std::vector<NSRect>& rects) {
	if (!needs_display) {
		needs_display = true;
		NSHandler([this, rects](NSThread*) {
			NSView* view = superview ? superview : this;
			NSPoint offset = superview ? frame.origin : NSPoint();
			for (auto& r : rects)
				view->DrawRect(NSRect(offset + r.origin, r.size));
		}).Post(NSThread::MainThread(), NSHandlerDefaultPriortiy);
	}
}

void NSView::RequestUpdate(const NSRect& rect) {
	std::vector<NSRect> rects = { rect };
	RequestUpdates(rects);
}

void NSView::RequestUpdates(const std::vector<NSRect>& r) {
	std::vector<NSRect> rects = r;
	NSPoint offset = frame.origin;
	for (unsigned int z = 0; z < rects.size(); z++)
		rects[z].origin += offset;
	
	for (auto& o : observers)
		o->UpdateRequested(r);
	
	if (superview) {
		superview->RequestUpdates(rects);
		return;
	}
	
	if (window && window->content_view == this)
		window->AddUpdateRects(rects);
}

bool NSView::AcceptsFirstResponder() const {
	return true;
}

void NSView::BecomeFirstResponder() {
	is_first_responder = true;
	
	for (auto& observer : observers)
		observer->BecomeFirstResponder();
}

bool NSView::IsFirstResponder() const {
	return is_first_responder;
}

void NSView::ResignFirstResponder() {
	is_first_responder = false;
	
	for (auto& observer : observers)
		observer->ResignFirstResponder();
}

void NSView::AddObserver(NSViewObserver* observer) {
	observers.push_back(observer);
}

void NSView::RemoveObserver(NSViewObserver* observer) {
	for (unsigned int z = 0; z < observers.size(); z++) {
		if (observers[z] == observer) {
			observers.erase(observers.begin() + z);
			break;
		}
	}
}

void NSView::AddCursorRegion(const NSCursorRegion& region) {
	cursor_regions.push_back(region);
}

// Removes all regions that overlap the rect
void NSView::RemoveCursorRegions(const NSRect& region) {
	for (unsigned int z = 0; z < cursor_regions.size(); z++) {
		if (region.OverlapsRect(cursor_regions[z].GetRegion())) {
			cursor_regions.erase(cursor_regions.begin() + z);
			z--;
		}
	}
}

void NSView::RemoveAllCursorRegions() {
	cursor_regions.clear();
}

uint32_t NSView::GetNumberOfCursorRegions() {
	return cursor_regions.size();
}

NSCursorRegion* NSView::GetCursorRegionAtIndex(uint32_t index) {
	return &cursor_regions[index];
}

// Local view coordinates
NSCursorRegion* NSView::GetCursorRegionAtPoint(const NSPoint& p) {
	if (!ContainsPoint(p))
		return NULL;
	
	for (unsigned int z = subviews.size() - 1; z != (unsigned int)-1; z--) {
		if (!subviews[z]->visible)
			continue;
		NSCursorRegion* ret = subviews[z]->GetCursorRegionAtPoint(p - subviews[z]->frame.origin);
		if (ret)
			return ret;
	}
	
	for (auto& c : cursor_regions) {
		if (c.GetRegion().ContainsPoint(p))
			return &c;
	}
	
	return NULL;
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
	std::function<void(const NSAnimation*)> la = [anim, this](const NSAnimation* a) {
		if (anim)
			anim(this, a);
	};
	std::function<void(const NSAnimation*, bool)> lc = [completion, this](const NSAnimation* a, bool canceled) {
		if (!canceled && completion)
			completion(this, a);
		for (unsigned int z = 0; z < animations.size(); z++) {
			if (animations[z] == a) {
				animations.erase(animations.begin() + z);
				delete a;
				break;
			}
		}
	};
	NSAnimation* animation = new NSAnimation(la, lc, duration);
	animations.push_back(animation);
	animation->Start(delay);
	return animation;
}

void NSView::MouseDown(NSEventMouse* event) {
	NSPoint p = event->GetPosition() - GetAbsoluteFrame().origin;
	if (!visible || !ContainsPoint(p))
		return;
	
	for (auto& s : subviews) {
		NSPoint sp = p - s->frame.origin;
		if (s->visible && s->ContainsPoint(sp))
			s->MouseDown(event);
	}
}

void NSView::MouseMoved(NSEventMouse* event) {
	if (!visible)
		return;
	for (auto& s : subviews) {
		if (s->visible)
			s->MouseMoved(event);
	}
}

void NSView::MouseDragged(NSEventMouse* event) {
	NSPoint p = event->GetPosition() - GetAbsoluteFrame().origin;
	if (!visible || !ContainsPoint(p))
		return;
	
	for (auto& s : subviews) {
		NSPoint sp = p - s->frame.origin;
		if (s->visible && s->ContainsPoint(sp))
			s->MouseDragged(event);
	}
}

void NSView::MouseUp(NSEventMouse* event) {
	if (!visible)
		return;
	for (auto& s : subviews) {
		if (s->visible)
			s->MouseUp(event);
	}
}

void NSView::MouseScrolled(NSEventMouse* event) {
	NSPoint p = event->GetPosition();
	if (!visible || !ContainsPoint(p))
		return;
	
	p -= frame.origin;
	for (auto& s : subviews) {
		if (s->visible && s->ContainsPoint(p))
			s->MouseScrolled(event);
	}
}

void NSView::KeyDown(NSEventKey* event)  {
	for (auto& s : subviews)
		s->KeyDown(event);
}

void NSView::KeyUp(NSEventKey* event)  {
	for (auto& s : subviews)
		s->KeyUp(event);
}

void NSView::BufferSquare(uint32_t bid) {
	const float square[] = {
		0, 0,
		1, 0,
		0, 1,
		1, 1
	};
	graphics_buffer_data(bid, square, sizeof(square));
}

void NSView::BufferOval(uint32_t bid, uint32_t num_vertices) {
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
	
	graphics_buffer_data(bid, vertices, sizeof(vertices));
}

void NSView::BufferRoundedRect(uint32_t bid, const NSSize& size, float border_radius_x, float border_radius_y,
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
	
	graphics_buffer_data(bid, vertices, sizeof(vertices));
}

void NSView::BufferHorizontalCurvedRect(uint32_t bid, const NSSize& size, uint32_t num_vertices) {
	float radius = size.height / 2;
	
	float vertices[num_vertices * 2];
	vertices[0] = size.width / 2;
	vertices[1] = size.height / 2;
	int num = (num_vertices - 2) / 2;
	uint32_t pos = 2;
	for (int z = 0; z < num; z++) {
		float angle = (270.0f - ((float(z) / (num - 1)) * 180.0f)) / 180.0f * M_PI;
		vertices[pos++] = radius + cos(angle) * radius;
		vertices[pos++] = radius - sin(angle) * radius;
	}
	for (int z = 0; z < num; z++) {
		float angle = (90.0f - ((float(z) / (num - 1)) * 180.0f)) / 180.0f * M_PI;
		vertices[pos++] = size.width - radius + cos(angle) * radius;
		vertices[pos++] = radius - sin(angle) * radius;
	}
	vertices[pos++] = radius;
	vertices[pos++] = size.height;
	
	graphics_buffer_data(bid, vertices, sizeof(vertices));
}

void NSView::BufferVerticalCurvedRect(uint32_t bid, const NSSize& size, uint32_t num_vertices) {
	float radius = size.width / 2;
	
	float vertices[num_vertices * 2];
	vertices[0] = size.width / 2;
	vertices[1] = size.height / 2;
	int num = (num_vertices - 2) / 2;
	uint32_t pos = 2;
	for (int z = 0; z < num; z++) {
		float angle = (180.0f - ((float(z) / (num - 1)) * 180.0f)) / 180.0f * M_PI;
		vertices[pos++] = radius + cos(angle) * radius;
		vertices[pos++] = radius - sin(angle) * radius;
	}
	for (int z = 0; z < num; z++) {
		float angle = (-((float(z) / (num - 1)) * 180.0f)) / 180.0f * M_PI;
		vertices[pos++] = radius + cos(angle) * radius;
		vertices[pos++] = size.height - radius - sin(angle) * radius;
	}
	vertices[pos++] = 0;
	vertices[pos++] = radius;
	
	graphics_buffer_data(bid, vertices, sizeof(vertices));
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
	uint32_t img_vbo = graphics_buffer_create(w, h, GRAPHICS_BUFFER_STATIC | GRAPHICS_BUFFER_TEXTURE,
											  GRAPHICS_FORMAT_A8R8G8B8);
	graphics_buffer_data(img_vbo, image->GetPixelData(), w, h, w * h * 4);
	
	return img_vbo;
}

void NSView::SetupContext(graphics_context_t* context, NSSize size) {
	NSMatrix iden = NSMatrix::Identity();
	graphics_transform_set(context, GRAPHICS_TRANSFORM_WORLD, iden);
	graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, iden);
	NSMatrix proj = NSMatrix::Ortho2D(0, size.width, size.height, 0);
	graphics_transform_set(context, GRAPHICS_TRANSFORM_PROJECTION, proj);
	graphics_renderstate_seti(context, GRAPHICS_RENDERSTATE_BLENDENABLE, true);
	graphics_renderstate_seti(context, GRAPHICS_RENDERSTATE_SRCBLEND, GRAPHICS_BLENDOP_SRCALPHA);
	graphics_renderstate_seti(context, GRAPHICS_RENDERSTATE_DSTBLEND, GRAPHICS_BLENDOP_INVSRCALPHA);
	graphics_renderstate_seti(context, GRAPHICS_RENDERSTATE_BLENDEQUATION, GRAPHICS_BLENDEQ_ADD);
	graphics_renderstate_seti(context, GRAPHICS_RENDERSTATE_SCISSORTESTENABLE, true);
	//graphics_renderstate_seti(context, GRAPHICS_RENDERSTATE_MULTISAMPLEANTIALIAS, true);
	graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_MINFILTER, GRAPHICS_TEXTURE_FILTER_LINEAR);
	graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_MAGFILTER, GRAPHICS_TEXTURE_FILTER_LINEAR);
}
