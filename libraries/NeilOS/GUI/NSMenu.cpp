//
//  NSMenu.cpp
//  product
//
//  Created by Neil Singh on 5/21/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSMenu.h"

#include "NSMenuItem.h"

#include <string.h>

using std::vector;

#define MENU_OFFSET_X	10
#define MENU_OFFSET_Y	5
#define MENU_MIN_WIDTH	200

#define MENU_COLOR_BACKGROUND	0
#define MENU_COLOR_HIGHLIGHTED	1
#define MENU_COLOR_BORDER		2
#define MENU_COLOR_BLACK		3
#define MENU_COLOR_WHITE		4

void NSMenu::SetupVAO() {
	const float square[] = {
		0, 0,
		1.0f, 0,
		0, 1.0f,
		1.0f, 1.0f
	};
	square_vbo = graphics_buffer_create(sizeof(square), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(square_vbo, square, sizeof(square));
	
	const float triangle[] = {
		0, 0,
		0, 1.0f,
		1.0f, 0.5f
	};
	triangle_vbo = graphics_buffer_create(sizeof(triangle), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(triangle_vbo, triangle, sizeof(triangle));
	
	NSColor<float> c[5];
	c[0] = NSColor<float>::UILighterGrayColor();
	c[1] = NSColor<float>::UIBlueColor();
	c[2] = NSColor<float>::UIGrayColor();
	c[3] = NSColor<float>::BlackColor();
	c[4] = NSColor<float>::WhiteColor();
	
	for (unsigned int i = 0; i < sizeof(c) / sizeof(NSColor<float>); i++) {
		float colors[4 * 3];
		for (int z = 0; z < 4; z++) {
			colors[z * 3 + 0] = c[i].r;
			colors[z * 3 + 1] = c[i].g;
			colors[z * 3 + 2] = c[i].b;
		}
		color_vbo[i] = graphics_buffer_create(sizeof(colors), GRAPHICS_BUFFER_STATIC);
		graphics_buffer_data(color_vbo[i], colors, sizeof(colors));
	}
	
	memset(square_vao, 0, sizeof(graphics_vertex_array_t) * 3);
	square_vao[0].bid = square_vbo;
	square_vao[0].offset = 0;
	square_vao[0].stride = 2 * sizeof(float);
	square_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	square_vao[0].usage = GRAPHICS_USAGE_POSITION;
	square_vao[1].bid = square_vbo;
	square_vao[1].offset = 0;
	square_vao[1].stride = 2 * sizeof(float);
	square_vao[1].type = GRAPHICS_TYPE_FLOAT2;
	square_vao[1].usage = GRAPHICS_USAGE_TEXCOORD;
	square_vao[2].bid = color_vbo[0];
	square_vao[2].offset = 0;
	square_vao[2].stride = 3 * sizeof(float);
	square_vao[2].type = GRAPHICS_TYPE_FLOAT3;
	square_vao[2].usage = GRAPHICS_USAGE_COLOR;

	memset(triangle_vao, 0, sizeof(graphics_vertex_array_t) * 2);
	triangle_vao[0].bid = triangle_vbo;
	triangle_vao[0].offset = 0;
	triangle_vao[0].stride = 2 * sizeof(float);
	triangle_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	triangle_vao[0].usage = GRAPHICS_USAGE_POSITION;
	triangle_vao[1].bid = color_vbo[MENU_COLOR_BLACK];
	triangle_vao[1].offset = 0;
	triangle_vao[1].stride = 3 * sizeof(float);
	triangle_vao[1].type = GRAPHICS_TYPE_FLOAT3;
	triangle_vao[1].usage = GRAPHICS_USAGE_COLOR;
}

NSMenu::NSMenu(bool is_context) {
	SetupVAO();
	
	is_context_menu = is_context;
	updates.push_back(-1);
}

NSMenu::NSMenu(const std::vector<NSMenuItem*>& i, bool is_context) {
	SetupVAO();
	
	SetItems(i);
	is_context_menu = is_context;
	updates.push_back(-1);
}

NSMenu::~NSMenu() {
	if (square_vbo)
		graphics_buffer_destroy(square_vbo);
	if (triangle_vbo)
		graphics_buffer_destroy(triangle_vbo);
	
	for (unsigned int z = 0; z < sizeof(color_vbo) / sizeof(uint32_t); z++) {
		if (color_vbo[z])
			graphics_buffer_destroy(color_vbo[z]);
	}
}

bool NSMenu::IsContextMenu() const {
	return is_context_menu;
}

void NSMenu::SetIsContextMenu(bool c) {
	is_context_menu = c;
}

std::vector<NSMenuItem*> NSMenu::GetItems() {
	return items;
}

void NSMenu::SetItems(const std::vector<NSMenuItem*>& i) {
	items = i;
	for (auto c : items)
		c->menu = this;
	
	updates.push_back(-1);
}

void NSMenu::AddItem(NSMenuItem* item) {
	item->menu = this;
	items.push_back(item);
	
	updates.push_back(items.size() - 1);
}

void NSMenu::AddItem(NSMenuItem* item, unsigned int index) {
	if (index > items.size())
		index = items.size();
	
	item->menu = this;
	items.insert(items.begin() + index, item);
	
	updates.reserve(items.size() - index);
	for (unsigned int z = index; z < items.size(); z++)
		updates.push_back(z);
}

void NSMenu::RemoveItem(unsigned int index) {
	if (index >= items.size())
		return;
	
	delete items[index];
	items.erase(items.begin() + index);
	
	updates.push_back(-1);
}

void NSMenu::RemoveAllItems() {
	for (auto c : items)
		delete c;
	items.clear();
	
	updates.push_back(-1);
}

NSRect NSMenu::AdjustRect(NSRect rect) {
	NSSize max_size = GetSize();
	if (is_context_menu) {
		rect.size = max_size;
		if (rect.size.width < MENU_MIN_WIDTH)
			rect.size.width = MENU_MIN_WIDTH;
	} else {
		// Adjust minimum width and height
		if (rect.size.width < max_size.width)
			rect.size.width = max_size.width;
		if (rect.size.height < max_size.height)
			rect.size.height = max_size.height;
	}
	
	return rect;
}

void NSMenu::ClearSubmenu() {
	if (submenu) {
		cleared_rects.push_back(submenu->AdjustRect(submenu->frame) + NSRect(-1, -1, 2, 2));
		submenu->ClearSubmenu();
		cleared_rects.insert(cleared_rects.end(), submenu->cleared_rects.begin(), submenu->cleared_rects.end());
		submenu = NULL;
	}
}

// down is true for mouse down, false for mouse move
bool NSMenu::MouseEvent(NSEventMouse* event, bool down) {
	// Position relative to start of menu
	NSPoint p = event->GetPosition();
	if (submenu) {
		event->SetPosition(event->GetPosition() + frame.origin - submenu->frame.origin);
		if (submenu->MouseEvent(event, down))
			return true;
		event->SetPosition(p);
	}
	NSRect rect = AdjustRect(NSRect(NSPoint(), frame.size));
	if (!p.InsideRect(rect)) {
		if (down) {
			ClearSubmenu();
			mouse_down = false;
			mouse_captured = false;
		}
		if (!submenu) {
			// Unhighlight everything
			for (unsigned int z = 0; z < items.size(); z++) {
				if (items[z]->highlighted) {
					items[z]->highlighted = false;
					updates.push_back(z);
				}
			}
		}
		return false;
	}
	
	if (down) {
		mouse_down = true;
		mouse_captured = true;
	}
	
	float pos = is_context_menu ? MENU_OFFSET_Y : MENU_OFFSET_X;
	for (unsigned int z = 0; z < items.size(); z++) {
		auto& i = items[z];
		NSSize item_size = i->GetSize();
		if ((is_context_menu && (p.y >= pos && p.y < pos + item_size.height)) ||
			(!is_context_menu && (p.x >= pos && p.x < pos + item_size.width))) {
			// Unhighlight all the others
			for (unsigned int q = 0; q < items.size(); q++) {
				if (q != z && items[q]->highlighted) {
					items[q]->highlighted = false;
					updates.push_back(q);
				}
			}
			// Don't need to do anything if its already correct
			if (i->highlighted) {
				if (down && !is_context_menu) {
					i->highlighted = false;
					ClearSubmenu();
					mouse_captured = false;
				}
				return true;
			}
			
			ClearSubmenu();
			if (i->is_separator || !i->enabled)
				break;
			
			i->highlighted = true;
			submenu = i->submenu;
			if (submenu && submenu->items.size() == 0)
				submenu = NULL;
			if (submenu) {
				submenu->is_context_menu = true;
				submenu->mouse_down = mouse_down;
				submenu->mouse_captured = mouse_captured;
				submenu->SetContext(context);
				if (is_context_menu) {
					submenu->SetFrame(NSRect(frame.origin.x +
											 (submenu->AdjustRect(submenu->frame) + NSRect(-1, -1, 2, 2)).size.width,
											 frame.origin.y + pos, 0, 0));
				} else {
					submenu->SetFrame(NSRect(frame.origin.x + pos,
											 frame.origin.y + frame.size.height + 2, 0, 0));
				}
				for (auto item : submenu->items)
					item->highlighted = false;
				submenu->updates.push_back(-1);
			}
			updates.push_back(z);
			break;
		}
		if (is_context_menu)
			pos += item_size.height;
		else
			pos += item_size.width;
	}
	
	return true;
}

bool NSMenu::MouseDown(NSEventMouse* event) {
	return MouseEvent(event, true);
}

void NSMenu::Clear() {
	ClearSubmenu();
	// Unhighlight everything
	for (unsigned int z = 0; z < items.size(); z++) {
		if (items[z]->highlighted) {
			items[z]->highlighted = false;
			updates.push_back(z);
		}
	}
}

bool NSMenu::MouseUp(NSEventMouse* event) {
	NSPoint p = event->GetPosition();
	if (submenu) {
		event->SetPosition(event->GetPosition() + frame.origin - submenu->frame.origin);
		if (submenu->MouseUp(event)) {
			Clear();
			mouse_down = false;
			mouse_captured = false;
			return true;
		}
		event->SetPosition(p);
	}
	
	if (!mouse_down)
		return false;
	mouse_down = false;
	
	// Find a highlighted item, if it doesn't have a submenu, execute the action
	for (auto& i : items) {
		if (i->highlighted) {
			if (i->submenu)
				return false;
			if (i->action)
				i->action(i);
			Clear();
			mouse_captured = false;
			return true;
		}
	}
	
	return false;
}

bool NSMenu::MouseMoved(NSEventMouse* event) {
	if (is_context_menu || (!is_context_menu && mouse_captured))
		return MouseEvent(event, false);
	return false;
}

bool NSMenu::KeyDown(NSEventKey* event) {
	// Check for key equivalents
	for (auto& i : items) {
		if (i->key_equivalent == std::string(1, toupper(event->GetKey())) &&
			i->flags == event->GetModifierFlags() && i->action) {
			i->action(i);
			return true;
		}
		if (i->submenu) {
			if (i->submenu->KeyDown(event))
				return true;
		}
	}
	return false;
}

bool NSMenu::KeyUp(NSEventKey* event) {
	return false;
}

NSSize NSMenu::GetSize() const {
	if (is_context_menu) {
		NSSize max_size;
		for (auto& i : items) {
			NSSize size = i->GetSize();
			max_size.height += size.height;
			if (max_size.width < size.width)
				max_size.width = size.width;
		}
		return max_size + NSSize(2 * MENU_OFFSET_X, 2 * MENU_OFFSET_Y);
	}
	
	// TODO: cache
	NSSize max_size = NSSize(2 * MENU_OFFSET_X, 0);
	for (auto& i : items) {
		NSSize size = i->GetSize();
		max_size.width += size.width;
		if (max_size.height < size.height)
			max_size.height = size.height;
	}
	return max_size;
}

NSRect NSMenu::GetFrame() const {
	return frame;
}

void NSMenu::SetFrame(NSRect f) {
	frame = f;
}

graphics_context_t* NSMenu::GetContext() const {
	return context;
}

void NSMenu::SetContext(graphics_context_t* c) {
	context = c;
}

bool RectsIntersect(const vector<NSRect>& rects, NSRect rect) {
	NSRect irect = rect.IntegerRect();
	for (uint32_t z = 0; z < rects.size(); z++) {
		if (rects[z].OverlapsRect(irect))
			return true;
	}
	return false;
}

void NSMenu::DrawUpdates(const vector<NSRect>& rects) {
	if (!context)
		return;
	
	NSRect rect = AdjustRect(frame);
	
	// If we have dirty rects, redraw the whole menu
	// TODO: might need to just redraw specific items
	if (RectsIntersect(rects, rect + NSRect(-1, -1, 2, 2))) {
		updates.clear();
		updates.push_back(-1);
	}
	
	// Figure out which items were updated
	bool draw_all = false;
	for (unsigned int z = 0; z < updates.size(); z++) {
		if (updates[z] == (uint32_t)-1) {
			updates.clear();
			updates.reserve(items.size());
			for (unsigned int z = 0; z < items.size(); z++)
				updates.emplace_back(z);
			draw_all = true;
			break;
		}
	}
	
	if (is_context_menu) {
		if (draw_all) {
			// Draw border
			NSMatrix matrix = NSMatrix::Identity();
			matrix.Translate(rect.origin.x - 1, rect.origin.y - 1, 0);
			matrix.Scale(rect.size.width + 2, rect.size.height + 2, 1);
			graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
			square_vao[2].bid = color_vbo[2];
			graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square_vao, 3);
			
			// Draw bar
			matrix = NSMatrix::Identity();
			matrix.Translate(rect.origin.x, rect.origin.y, 0);
			matrix.Scale(rect.size.width, rect.size.height, 1);
			graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
			square_vao[2].bid = color_vbo[0];
			graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square_vao, 3);
			
			updated_rects.push_back(rect + NSRect(-1, -1, 2, 2));
		}
		
		float pos_y = MENU_OFFSET_Y;
		for (unsigned int z = 0; z < items.size(); z++) {
			auto& i = items[z];
			bool found = false;
			for (unsigned int q = 0; q < updates.size(); q++) {
				if (updates[q] == z) {
					found = true;
					break;
				}
			}
			NSSize size = NSSize(rect.size.width, i->GetSize().height);
			if (found) {
				NSPoint p = NSPoint(rect.origin.x, rect.origin.y + pos_y);
				i->Draw(context, p, size);
				if (!draw_all)
					updated_rects.push_back(NSRect(p, size));
			}
			pos_y += size.height;
		}
	} else {
		// Straight bar menu
		if (draw_all) {
			// Draw border
			NSMatrix matrix = NSMatrix::Identity();
			matrix.Translate(rect.origin.x - 1, rect.origin.y - 1, 0);
			matrix.Scale(rect.size.width + 2, rect.size.height + 2, 1);
			graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
			square_vao[2].bid = color_vbo[2];
			graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square_vao, 3);
			
			// Draw bar
			matrix = NSMatrix::Identity();
			matrix.Translate(rect.origin.x, rect.origin.y, 0);
			matrix.Scale(rect.size.width, rect.size.height, 1);
			graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
			square_vao[2].bid = color_vbo[0];
			graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square_vao, 3);
			
			updated_rects.push_back(rect + NSRect(-1, -1, 2, 2));
		}
		
		// Draw menu items
		float pos_x = MENU_OFFSET_X;
		for (unsigned int z = 0; z < items.size(); z++) {
			auto& i = items[z];
			bool found = false;
			for (unsigned int q = 0; q < updates.size(); q++) {
				if (updates[q] == z) {
					found = true;
					break;
				}
			}
			NSSize size = NSSize(i->GetSize().width, rect.size.height);
			if (found) {
				NSPoint p = NSPoint(pos_x + rect.origin.x, rect.origin.y);
				i->Draw(context, p, size);
				if (!draw_all)
					updated_rects.push_back(NSRect(p, size));
			}
			pos_x += size.width;
		}
	}
	
	updates.clear();
	if (submenu)
		submenu->DrawUpdates(rects);
}

bool NSMenu::NeedsUpdate() {
	bool sub = false;
	if (submenu)
		sub = submenu->NeedsUpdate();
	return ((updates.size() != 0) || (cleared_rects.size() != 0) || (updated_rects.size() != 0) || sub);
}

std::vector<NSRect> NSMenu::GetClearedRects() {
	std::vector<NSRect> ret = cleared_rects;
	if (submenu) {
		std::vector<NSRect> ret2 = submenu->GetClearedRects();
		ret.insert(ret.end(), ret2.begin(), ret2.end());
	}
	cleared_rects.clear();
	return ret;
}

std::vector<NSRect> NSMenu::GetUpdatedRects() {
	std::vector<NSRect> ret = updated_rects;
	if (submenu) {
		std::vector<NSRect> ret2 = submenu->GetUpdatedRects();
		ret.insert(ret.end(), ret2.begin(), ret2.end());
	}
	updated_rects.clear();
	return ret;
}
