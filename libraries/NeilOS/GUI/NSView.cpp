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
#include "NSScrollView.h"
#include "NSWindow.h"
#include "../NeilOS.h"
#include "NSViewTypes.hpp"

#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define ANIMATION_FRAMERATE		60.0

#define SQUARE_VERTICES			4

NSView* NSView::Create() {
	NSView* ret = new NSView();
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
	vao[0].stride = 2 * sizeof(float);
	vao[0].type = GRAPHICS_TYPE_FLOAT2;
	vao[0].usage = GRAPHICS_USAGE_POSITION;
	vao[1].bid = color_vbo;
	vao[1].stride = 4 * sizeof(float);
	vao[1].type = GRAPHICS_TYPE_FLOAT4;
	vao[1].usage = GRAPHICS_USAGE_COLOR;
}

NSView* NSView::Clone() {
	NSView* ret = new NSView;
	*ret = *this;
	return ret;
}

NSView& NSView::operator =(const NSView& v) {
	if (this == &v)
		return *this;
	
	for (auto& s : subviews)
		delete s;
	subviews.clear();
	
	SetFrame(v.frame);
	for (uint32_t z = 0; z < v.subviews.size(); z++)
		AddSubview(v.subviews[z]->Clone());
	visible = v.visible;
	resizing_mask = v.resizing_mask;
	cursor_regions = v.cursor_regions;
	action = v.action;
	
	return *this;
}

NSView* NSView::FromData(const uint8_t* data, uint32_t length, uint32_t* length_used) {
#define copy(x, l)			{ memcpy(x, &data[pos], (l)); pos += (l); }
	
	if (length < sizeof(uint32_t))
		return NULL;
	
	const uint32_t* buffer = reinterpret_cast<const uint32_t*>(data);
	uint32_t type = buffer[0];
	if (type != NSVIEW_TYPES_VIEW) {
		NSView* ret = NSViewTypes::func[type](&data[sizeof(uint32_t)], length - sizeof(uint32_t), length_used);
		if (length_used)
			*length_used += sizeof(uint32_t);
		return ret;
	}
	
	if (length < sizeof(uint32_t) * 4 + sizeof(float) * 4 + sizeof(bool))
		return NULL;
	
	uint32_t pos = sizeof(uint32_t);
	NSRect frame = NSRect::FromData(&data[pos], sizeof(float) * 4);
	pos += sizeof(float) * 4;
	bool visible;
	NSAutoResizingMask resizing_mask;
	uint32_t num_cursor_regions, num_subviews;
	copy(&visible, sizeof(bool));
	copy(&resizing_mask, sizeof(uint32_t));
	copy(&num_cursor_regions, sizeof(uint32_t));
	
	std::vector<NSCursorRegion> cursor_regions;
	cursor_regions.reserve(num_cursor_regions);
	for (unsigned int z = 0; z < num_cursor_regions; z++) {
		uint32_t lu;
		NSCursorRegion* r = NSCursorRegion::FromData(&data[pos], length - pos, &lu);
		if (!r)
			return NULL;
		pos += lu;
		cursor_regions.emplace_back(*r);
		delete r;
	}
	
	copy(&num_subviews, sizeof(uint32_t));
	std::vector<NSView*> subviews;
	subviews.reserve(num_subviews);
	for (unsigned int z = 0; z < num_subviews; z++) {
		uint32_t lu;
		NSView* s = NSView::FromData(&data[pos], length - pos, &lu);
		if (!s)
			return NULL;
		pos += lu;
		subviews.emplace_back(s);
	}
	
#undef copy
	
	NSView* view = new NSView();
	view->SetFrame(frame);
	view->SetVisible(visible);
	view->SetResizingMask(resizing_mask);
	view->cursor_regions = cursor_regions;
	view->SetSubviews(subviews);
	
	if (length_used)
		*length_used = pos;
	
	return view;
}

uint8_t* NSView::Serialize(uint32_t* length_out) const {
#define copy(x, len) { memcpy(&buffer[pos], x, (len)); pos += (len); }
#define copy_rect(x) { uint8_t* temp = x.Serialize(NULL); memcpy(&buffer[pos], temp, sizeof(float)*4);\
						pos += sizeof(float) * 4; delete[] temp; }
	
	uint32_t total_length = sizeof(uint32_t);		// view type
	total_length += sizeof(float) * 4;				// frame
	total_length += sizeof(bool);					// visible
	total_length += sizeof(uint32_t);				// resizing mask
	total_length += sizeof(uint32_t);				// cursor_regions.size
	total_length += sizeof(uint32_t);				// subviews.size
	
	std::vector<std::pair<uint8_t*, uint32_t>> crs;
	crs.reserve(cursor_regions.size());
	for (auto& cr : cursor_regions) {
		uint32_t len;
		uint8_t* buf = cr.Serialize(&len);
		if (!buf) {
			for (auto& c : crs)
				delete[] c.first;
			return NULL;
		}
		total_length += len;
		crs.emplace_back(std::make_pair(buf, len));
	}
	
	std::vector<std::pair<uint8_t*, uint32_t>> svs;
	svs.reserve(subviews.size());
	for (auto& sv : subviews) {
		uint32_t len;
		uint8_t* buf = sv->Serialize(&len);
		if (!buf) {
			for (auto& c : crs)
				delete[] c.first;
			for (auto& s : svs)
				delete[] s.first;
			return NULL;
		}
		total_length += len;
		svs.emplace_back(std::make_pair(buf, len));
	}
	
	uint8_t* buffer = new uint8_t[total_length];
	uint32_t pos = 0;
	
	uint32_t view_code = NSVIEW_TYPES_VIEW;
	copy(&view_code, sizeof(uint32_t));
	copy_rect(frame);
	copy(&visible, sizeof(bool));
	copy(&resizing_mask, sizeof(uint32_t));
	uint32_t crs_size = crs.size(), svs_size = svs.size();
	copy(&crs_size, sizeof(uint32_t));
	for (auto& cr : crs) {
		copy(cr.first, cr.second);
		delete[] cr.first;
	}
	copy(&svs_size, sizeof(uint32_t));
	for (auto& sv : svs) {
		copy(sv.first, sv.second);
		delete[] sv.first;
	}
	
#undef copy
#undef copy_rect
	
	if (length_out)
		*length_out = total_length;
	return buffer;
}

NSView::~NSView() {
	if (vertex_vbo)
		graphics_buffer_destroy(vertex_vbo);
	if (color_vbo)
		graphics_buffer_destroy(color_vbo);
	
	for (uint32_t z = 0; z < subviews.size(); z++)
		delete subviews[z];
	
	while (animations.size() != 0)
		animations[0]->Cancel();
	
	if (display_handler) {
		display_handler->Cancel();
		delete display_handler;
	}
}

NSRect NSView::GetFrame() const {
	return frame;
}

NSRect NSView::GetBounds() const {
	return NSRect(NSPoint(), frame.size);
}

void NSView::SetFrame(const NSRect& f) {
	if (frame == f)
		return;
	
	NSRect old_frame = frame;
	frame = f;
	
	if (old_frame.size != frame.size) {
		UpdateVBO();
		for (unsigned int z = 0; z < subviews.size(); z++)
			subviews[z]->Resize(old_frame.size, frame.size);
	}
	
	for (auto& observer : observers)
		observer->FrameUpdated();
	
	if (superview)
		superview->SetNeedsDisplay({ old_frame, frame });
	else
		SetNeedsDisplay();
}

void NSView::Resize(NSSize old_sv_size, NSSize sv_size) {
	NSAutoResizingMask mask = resizing_mask;
	if (mask == NSViewNotResizable)
		return;
	
	bool width = (mask & NSViewWidthSizable) != 0, height = (mask & NSViewHeightSizable) != 0;
	bool x_min = (mask & NSViewMinXMargin) != 0, x_max = (mask & NSViewMaxXMargin) != 0;
	bool y_min = (mask & NSViewMinYMargin) != 0, y_max = (mask & NSViewMaxYMargin) != 0;
	
	NSRect new_frame = frame;
	NSRect old_frame = frame;
	if (width)
		new_frame.size.width = (new_frame.size.width / old_sv_size.width) * sv_size.width;
	if (mask & NSViewHeightSizable)
		new_frame.size.height = (new_frame.size.height / old_sv_size.height) * sv_size.height;
	
	if (x_min && x_max && width) {
		float dist = old_sv_size.width - (old_frame.origin.x + old_frame.size.width);
		new_frame.size.width = sv_size.width - dist - frame.origin.x;
	} else if (x_min) {
		// Do nothing (implicit)
	} else if (x_max) {
		float dist = old_sv_size.width - (old_frame.origin.x + old_frame.size.width);
		new_frame.origin.x = sv_size.width - new_frame.size.width - dist;
	}
	
	if (y_min && y_max && height) {
		float dist = old_sv_size.height - (old_frame.origin.y + old_frame.size.height);
		new_frame.size.height = sv_size.height - dist - new_frame.origin.y;
	} else if (y_min) {
		// Do nothing (implicit)
	} else if (y_max) {
		float dist = old_sv_size.height - (old_frame.origin.y + old_frame.size.height);
		new_frame.origin.y = sv_size.height - new_frame.size.height - dist;
	}
	
	SetFrame(new_frame);
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
		return rect + NSRect(frame.origin, NSSize());
	return NSRect(rect.origin + superview->GetAbsoluteFrame().origin + frame.origin, rect.size);
}

void NSView::SetSubview(NSView* view) {
	view->superview = this;
	view->SetViewContainer(container);
}

void NSView::AddSubview(NSView* view) {
	SetSubview(view);
	
	subviews.push_back(view);
	
	for (auto& observer : observers)
		observer->SubviewsUpdated();
	
	DrawSubview(view, view->frame);
}

void NSView::SetSubviews(const std::vector<NSView*>& views) {
	for (uint32_t z = 0; z < subviews.size(); z++)
		delete subviews[z];
	subviews = views;
	for (unsigned int z = 0; z < subviews.size(); z++)
		SetSubview(subviews[z]);
	
	for (auto& observer : observers)
		observer->SubviewsUpdated();
	
	SetNeedsDisplay();
}

unsigned int NSView::GetNumberOfSubviews() const {
	return subviews.size();
}

unsigned int NSView::GetIndexOfSubview(NSView* view) const {
	for (unsigned int z = 0; z < subviews.size(); z++) {
		if (subviews[z] == view)
			return z;
	}
	return -1;
}

void NSView::SetViewContainer(NSViewContainer* c) {
	container = c;
	if (container)
		container->ViewAdded(this);
	for (auto& s : subviews)
		s->SetViewContainer(c);
}

bool NSView::RemoveSubview(NSView* view) {
	for (uint32_t z = 0; z < subviews.size(); z++) {
		if (subviews[z] == view) {
			NSRect rect = view->frame;
			delete subviews[z];
			subviews.erase(subviews.begin() + z);
			
			for (auto& observer : observers)
				observer->SubviewsUpdated();
			
			SetNeedsDisplay(rect);
			return true;
		}
	}
	return false;
}

void NSView::RemoveSubviewAtIndex(unsigned int index) {
	NSRect rect = subviews[index]->frame;
	delete subviews[index];
	subviews.erase(subviews.begin() + index);
	
	for (auto& observer : observers)
		observer->SubviewsUpdated();
	
	SetNeedsDisplay(rect);
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

NSView* NSView::GetSuperview() const {
	return superview;
}

NSViewContainer* NSView::GetViewContainer() const {
	return container;
}

NSScrollView* NSView::GetEnclosingScrollView() const {
	return dynamic_cast<NSScrollView*>(container);
}

const std::function<void(NSView*)>& NSView::GetAction() const {
	return action;
}

void NSView::SetAction(const std::function<void(NSView*)>& a) {
	action = a;
}

bool NSView::IsVisible() const {
	return visible;
}

void NSView::SetVisible(bool is) {
	if (visible == is)
		return;
	
	visible = is;
	if (!visible && superview)
		superview->SetNeedsDisplay(frame);
	else
		SetNeedsDisplay();
}

NSAutoResizingMask NSView::GetResizingMask() const {
	return resizing_mask;
}

void NSView::SetResizingMask(NSAutoResizingMask mask) {
	resizing_mask = mask;
}

void NSView::DrawRect(const NSRect& rect) {
	if (!container || !visible)
		return;
	
	graphics_context_t* context = container->GetContext();
	if (!context)
		return;
	
	NSRect r = PrepareDraw(context, rect);
	Draw(context, r);
	FinishDraw(context, r);
	
	// Draw subviews
	for (unsigned int z = 0; z < subviews.size(); z++)
		DrawSubview(subviews[z], r);
}

void NSView::DrawSubview(NSView* subview, NSRect rect) {
	if (subview->visible && subview->frame.OverlapsRect(rect)) {
		NSRect out = NSRect(rect.origin - subview->frame.origin, rect.size);
		if (NSRectClamp(out, subview->GetBounds(), &out))
			subview->DrawRect(out);
	}
}

NSRect NSView::PrepareDraw(graphics_context_t* context, const NSRect& r) {
	NSRect rect = container->PrepareDraw(this, r);
	
	NSRect total = GetAbsoluteRect(rect);
	NSPoint offset = container->GetScissorOffset(total);
	float bsf = container->GetBackingScaleFactor();
	if (superview)
		total = total.Intersection(superview->GetAbsoluteFrame());
	NSRect scissor = ((total + NSRect(offset, NSSize())) * bsf).IntegerRect();
	graphics_scissor_rect_set(context, scissor.origin.x, scissor.origin.y,
							  scissor.size.width, scissor.size.height);
	
	NSMatrix m = NSMatrix::Identity();
	NSRect trans = GetAbsoluteFrame();
	m.Translate(trans.origin.x, trans.origin.y, 0);
	graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, m);
	
	return rect;
}

void NSView::FinishDraw(graphics_context_t* context, const NSRect& rect) {
	// Reset some things
	NSMatrix m = NSMatrix::Identity();
	graphics_transform_set(context, GRAPHICS_TRANSFORM_WORLD, m);
	graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, m);
	graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
	
	if (container)
		container->FinishDraw(this, rect);
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

void NSView::Draw(graphics_context_t* context, const NSRect& rect) {
	if (!container)
		return;
	
	NSMatrix matrix = NSMatrix::Identity();
	matrix.Scale(frame.size.width, frame.size.height, 1);
	graphics_transform_set(context, GRAPHICS_TRANSFORM_WORLD, matrix);
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
	if (!visible || !container || !requests_updates)
		return;
	
	dirty_rects.insert(dirty_rects.end(), rects.begin(), rects.end());
	if (!display_handler) {
		display_handler = new NSHandler([this, rects](NSThread*) {
			delete display_handler;
			display_handler = NULL;
			
			NSView* view = superview ? superview : this;
			NSPoint offset = superview ? frame.origin : NSPoint();
			
			std::vector<NSRect> rs = dirty_rects;
			dirty_rects.clear();
			rs = NSRectConsolidate(rs);
			for (const auto& r : rs)
				view->DrawRect(NSRect(offset + r.origin, r.size));
		});
		display_handler->Post(NSThread::MainThread(), NSHandlerDefaultPriortiy);
	}
}

void NSView::RequestUpdate(const NSRect& rect) {
	std::vector<NSRect> rects = { rect };
	RequestUpdates(rects);
}

void NSView::RequestUpdates(const std::vector<NSRect>& r) {
	if (!requests_updates)
		return;
	
	std::vector<NSRect> rects = r;
	NSPoint offset = GetAbsoluteFrame().origin;
	for (unsigned int z = 0; z < rects.size(); z++)
		rects[z].origin += offset;
	
	for (auto& o : observers)
		o->UpdateRequested(r);
	
	if (container)
		container->AddUpdateRects(rects);
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
	if (!is_first_responder)
		return;
	
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
	
	mouse_is_down = true;
	for (auto& s : subviews) {
		if (s->visible)
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
	if (!visible || !mouse_is_down)
		return;
	
	for (auto& s : subviews) {
		if (s->visible)
			s->MouseDragged(event);
	}
}

void NSView::MouseUp(NSEventMouse* event) {
	mouse_is_down = false;
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
	
	for (auto& s : subviews) {
		if (s->visible)
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
	static const float square[] = {
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

void NSView::BufferRoundedRect(uint32_t bid, const NSSize& size, float borders[4], int mask, uint32_t num_vertices) {
	int c = 0;
	for (int z = 0; z < 4; z++) {
		if (mask & (1 << z))
			c++;
	}
	float vertices[num_vertices * 2];
	vertices[0] = size.width / 2;
	vertices[1] = size.height / 2;
	uint32_t pos = 2;
	int num = (num_vertices - 2 - (4 - c)) / c;
	for (int z = 0; z < 4; z++) {
		float x_pos = (z == 0 || z == 3) ? 0 : size.width;
		float y_pos = (z >= 2) ? size.height : 0;
		if (mask & (1 << z)) {
			float start_angle = 180 - (z * 90);
			float start_x = (z == 0 || z == 3) ? borders[z] : -borders[z];
			float start_y = (z >= 2) ? -borders[z] : borders[z];
			for (int q = 0; q < num; q++) {
				float angle = (start_angle - (float(q) / (num - 1)) * 90.0f) / 180.0f * M_PI;
				float x = cosf(angle) * borders[z] + x_pos + start_x;
				float y = -sinf(angle) * borders[z] + y_pos + start_y;
				vertices[pos++] = x;
				vertices[pos++] = y;
			}
		} else {
			vertices[pos++] = x_pos;
			vertices[pos++] = y_pos;
		}
	}
	vertices[pos++] = 0;
	vertices[pos++] = (mask & 0x1) ? borders[0] : 0;
	
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

// num_verticies = 6, triangle_strip
void NSView::BufferCheckmark(uint32_t bid) {
	const float vertices[NSVIEW_CHECKMARK_VERTICES * 2] = {
		0.125, 0.5,
		0, 0.625,
		0.375, 0.75,
		0.375, 1,
		0.875, 0,
		1, 0.125
	};
	graphics_buffer_data(bid, vertices, sizeof(vertices));
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
	// No need to filter as the window server filters the whole window when drawing it
	graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_MINFILTER, GRAPHICS_TEXTURE_FILTER_NONE);
	graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_MAGFILTER, GRAPHICS_TEXTURE_FILTER_NONE);
}
