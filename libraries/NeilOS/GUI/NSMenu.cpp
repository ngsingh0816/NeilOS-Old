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
	
	NSColor<float> c[5] = { background_color, highlight_color, border_color, text_color,
		text_highlight_color };
	
	for (unsigned int i = 0; i < sizeof(c) / sizeof(NSColor<float>); i++) {
		float colors[4 * 4];
		for (int z = 0; z < 4; z++) {
			colors[z * 4 + 0] = c[i].r;
			colors[z * 4 + 1] = c[i].g;
			colors[z * 4 + 2] = c[i].b;
			colors[z * 4 + 3] = c[i].a;
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
	square_vao[2].stride = 4 * sizeof(float);
	square_vao[2].type = GRAPHICS_TYPE_FLOAT4;
	square_vao[2].usage = GRAPHICS_USAGE_COLOR;

	memset(triangle_vao, 0, sizeof(graphics_vertex_array_t) * 2);
	triangle_vao[0].bid = triangle_vbo;
	triangle_vao[0].offset = 0;
	triangle_vao[0].stride = 2 * sizeof(float);
	triangle_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	triangle_vao[0].usage = GRAPHICS_USAGE_POSITION;
	triangle_vao[1].bid = color_vbo[MENU_COLOR_BLACK];
	triangle_vao[1].offset = 0;
	triangle_vao[1].stride = 4 * sizeof(float);
	triangle_vao[1].type = GRAPHICS_TYPE_FLOAT4;
	triangle_vao[1].usage = GRAPHICS_USAGE_COLOR;
}

NSMenu::NSMenu(const NSMenu& menu) {
	SetupVAO();
	
	*this = menu;
}

NSMenu& NSMenu::operator=(const NSMenu& menu) {
	if (&menu == this)
		return *this;
	
	items.resize(menu.items.size());
	for (unsigned int z = 0; z < menu.items.size(); z++) {
		items[z] = new NSMenuItem(*menu.items[z]);
		items[z]->highlighted = false;
		items[z]->menu = this;
	}
	submenu = NULL;
	is_context_menu = menu.is_context_menu;
	menu_size = menu.menu_size;
	cached_size = menu.cached_size;
	frame = menu.frame;
	orientation = menu.orientation;
	mouse_down = menu.mouse_down;
	mouse_captured = menu.mouse_captured;
	update = menu.update;
	context = menu.context;
	
	NSColor<float> colors[] = { menu.background_color, menu.highlight_color, menu.border_color,
		menu.text_color, menu.text_highlight_color };
	SetColors(colors);
	
	return *this;
}

NSMenu* NSMenu::FromData(uint8_t* data, uint32_t length, uint32_t* length_used) {
#define copy(x, len) { if (pos >= length) { return NULL; }; memcpy(x, &data[pos], (len)); pos += (len); }
	if (length < sizeof(uint32_t) * 6 + sizeof(bool) + sizeof(float) * 4 + sizeof(NSMenuOrientation))
		return NULL;
	
	uint32_t pos = 0;
	
	bool cm;
	NSRect frame;
	NSMenuOrientation orientation;
	NSColor<float> colors[5];
	NSMenu* menu = NULL;
	
	uint32_t num_items;
	copy(&num_items, sizeof(uint32_t));
	std::vector<NSMenuItem*> items;
	items.reserve(num_items);
	for (unsigned int z = 0; z < num_items; z++) {
		uint32_t len;
		NSMenuItem* item = NSMenuItem::FromData(&data[pos], length - pos, &len);
		if (!item)
			goto error;
		items.emplace_back(item);
		pos += len;
		if (pos >= length)
			goto error;
	}
	
	copy(&cm, sizeof(bool));
	if (pos + sizeof(float) * 4 >= length)
		goto error;
	frame = NSRect::FromData(&data[pos], sizeof(float) * 4);
	pos += sizeof(float) * 4;
	copy(&orientation, sizeof(NSMenuOrientation));
	
	for (int z = 0; z < 5; z++) {
		uint32_t value;
		copy(&value, sizeof(uint32_t));
		colors[z] = NSColor<float>(value);
	}
	
	if (length_used)
		*length_used = pos;
	
	menu = new NSMenu(items, cm);
	menu->frame = frame;
	menu->orientation = orientation;
	menu->SetColors(colors);
	return menu;
#undef copy
	
error:
	for (unsigned int z = 0; z < items.size(); z++)
		delete items[z];
	return NULL;
}

uint8_t* NSMenu::Serialize(uint32_t* length_out) {
#define copy(x, len) { memcpy(&buffer[pos], x, (len)); pos += (len); }
	uint32_t total_length = 0;
	
	// Items
	uint32_t num_items = items.size();
	uint8_t* item_ptr[num_items];
	uint32_t item_len[num_items];
	uint32_t item_total_size = 0;
	for (unsigned int z = 0; z < num_items; z++) {
		item_ptr[z] = items[z]->Serialize(&item_len[z]);
		item_total_size += item_len[z];
	}
	
	total_length += sizeof(uint32_t);	// items size
	total_length += item_total_size;
	total_length += sizeof(bool);		// is_context_menu
	total_length += sizeof(float) * 4;	// frame
	total_length += sizeof(NSMenuOrientation);
	total_length += sizeof(uint32_t) * 5; // colors
	
	uint8_t* buffer = new uint8_t[total_length];
	uint32_t pos = 0;
	
	copy(&num_items, sizeof(uint32_t));
	for (unsigned int z = 0; z < num_items; z++) {
		copy(item_ptr[z], item_len[z]);
		delete[] item_ptr[z];
	}
	copy(&is_context_menu, sizeof(bool));
	uint8_t* frame_buf = frame.Serialize(NULL);
	copy(frame_buf, sizeof(float) * 4);
	delete[] frame_buf;
	copy(&orientation, sizeof(NSMenuOrientation));
	
	NSColor<float> colors[] = { background_color, highlight_color, border_color, text_color, text_highlight_color };
	for (unsigned int z = 0; z < 5; z++) {
		uint32_t value = colors[z].RGBAValue();
		copy(&value, sizeof(uint32_t));
	}
	
	if (length_out)
		*length_out = total_length;
	return buffer;
#undef copy
}

NSMenu::NSMenu(bool is_context) {
	SetupVAO();
	
	is_context_menu = is_context;
}

NSMenu::NSMenu(const std::vector<NSMenuItem*>& i, bool is_context) {
	SetupVAO();
	
	SetItems(i);
	is_context_menu = is_context;
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

void NSMenu::SetColor(NSColor<float> color, unsigned int index) {
	if (!color_vbo[index])
		color_vbo[index] = graphics_buffer_create(sizeof(float) * 4 * 4, GRAPHICS_BUFFER_STATIC);
	
	float colors[4 * 4];
	for (int z = 0; z < 4; z++) {
		colors[z * 4 + 0] = color.r;
		colors[z * 4 + 1] = color.g;
		colors[z * 4 + 2] = color.b;
		colors[z * 4 + 3] = color.a;
	}
	graphics_buffer_data(color_vbo[index], colors, sizeof(colors));
}

void NSMenu::SetColors(NSColor<float> colors[5]) {
	NSColor<float>* current[] = { &background_color, &highlight_color, &border_color, &text_color,
		&text_highlight_color };
	for (int z = 0; z < 5; z++) {
		if (*current[z] != colors[z]) {
			*current[z] = colors[z];
			SetColor(colors[z], z);
		}
	}
}

NSColor<float> NSMenu::GetBackgroundColor() const {
	return background_color;
}

void NSMenu::SetBackgroundColor(NSColor<float> color) {
	background_color = color;
	SetColor(color, MENU_COLOR_BACKGROUND);
}

NSColor<float> NSMenu::GetHighlightColor() const {
	return highlight_color;
}

void NSMenu::SetHighlightColor(NSColor<float> color) {
	highlight_color = color;
	SetColor(color, MENU_COLOR_HIGHLIGHTED);
}

NSColor<float> NSMenu::GetBorderColor() const {
	return border_color;
}

void NSMenu::SetBorderColor(NSColor<float> color) {
	border_color = color;
	SetColor(color, MENU_COLOR_BORDER);
}

NSColor<float> NSMenu::GetTextColor() const {
	return text_color;
}

void NSMenu::SetTextColor(NSColor<float> color) {
	text_color = color;
	SetColor(color, MENU_COLOR_BLACK);
}

NSColor<float> NSMenu::GetHighlightedTextColor() const {
	return text_highlight_color;
}

void NSMenu::SetHighlightedTextColor(NSColor<float> color) {
	text_highlight_color = color;
	SetColor(color, MENU_COLOR_WHITE);
}

bool NSMenu::IsContextMenu() const {
	return is_context_menu;
}

void NSMenu::SetIsContextMenu(bool c) {
	is_context_menu = c;
}

NSMenuOrientation NSMenu::GetOrientation() const {
	return orientation;
}

void NSMenu::SetOrientation(NSMenuOrientation o) {
	orientation = o;
}

std::vector<NSMenuItem*> NSMenu::GetItems() {
	return items;
}

void NSMenu::SetItems(const std::vector<NSMenuItem*>& i) {
	items = i;
	for (auto& c : items) {
		c->menu = this;
		c->UpdateSize();
	}
	
	UpdateAll();
}

void NSMenu::AddItem(NSMenuItem* item) {
	item->menu = this;
	item->UpdateSize();
	items.push_back(item);
	
	UpdateItem(items.size() - 1);
}

void NSMenu::AddItem(NSMenuItem* item, unsigned int index) {
	if (index > items.size())
		index = items.size();
	
	item->menu = this;
	item->UpdateSize();
	items.insert(items.begin() + index, item);
	
	std::vector<unsigned int> updates;
	updates.reserve(items.size() - index);
	for (unsigned int z = index; z < items.size(); z++)
		updates.push_back(z);
	UpdateItems(updates);
}

void NSMenu::RemoveItem(unsigned int index) {
	if (index >= items.size())
		return;
	
	delete items[index];
	items.erase(items.begin() + index);
	
	UpdateAll();
}

void NSMenu::RemoveAllItems() {
	for (auto c : items)
		delete c;
	items.clear();
	
	UpdateAll();
}

uint32_t NSMenu::IndexOfItem(NSMenuItem* item) {
	for (unsigned int z = 0; z < items.size(); z++) {
		if (item == items[z])
			return z;
	}
	return -1;
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
	if (submenu && update) {
		std::vector<NSRect> rects = { submenu->AdjustRect(submenu->frame) + NSRect(-1, -1, 2, 2) };
		submenu->ClearSubmenu();
		submenu = NULL;
		update(rects);
	}
}

// down is true for mouse down, false for mouse move
bool NSMenu::MouseEvent(NSEventMouse* event, bool down) {
	if (down && event->GetButton() != NSMouseButtonLeft)
		return false;
	
	std::vector<unsigned int> updates;
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
			UpdateItems(updates);
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
					updates.push_back(z);
				}
				UpdateItems(updates);
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
				NSColor<float> colors[] = { background_color, highlight_color, border_color, text_color,
					text_highlight_color };
				submenu->is_context_menu = true;
				submenu->SetColors(colors);
				submenu->mouse_down = mouse_down;
				submenu->mouse_captured = mouse_captured;
				submenu->SetContext(context);
				submenu->SetUpdateFunction(update);
				if (is_context_menu) {
					submenu->SetFrame(NSRect(frame.origin.x +
											 (submenu->AdjustRect(submenu->frame) + NSRect(-1, -1, 2, 2)).size.width,
											 frame.origin.y + pos, 0, 0));
				} else {
					if (orientation == NSMenuOrientationUp) {
						NSRect fr = submenu->AdjustRect(NSRect(frame.origin.x + pos, frame.origin.y - 1, 0, 0));
						fr.origin.y -= fr.size.height;
						submenu->SetFrame(fr);
					} else {
						submenu->SetFrame(NSRect(frame.origin.x + pos,
												 frame.origin.y + AdjustRect(frame).size.height + 1, 0, 0));
					}
				}
				for (auto item : submenu->items)
					item->highlighted = false;
				submenu->UpdateAll();
			}
			updates.push_back(z);
			break;
		}
		if (is_context_menu)
			pos += item_size.height;
		else
			pos += item_size.width;
	}
	UpdateItems(updates);
	
	return true;
}

bool NSMenu::MouseDown(NSEventMouse* event) {
	return MouseEvent(event, true);
}

void NSMenu::Clear() {
	ClearSubmenu();
	// Unhighlight everything
	std::vector<unsigned int> updates;
	for (unsigned int z = 0; z < items.size(); z++) {
		if (items[z]->highlighted) {
			items[z]->highlighted = false;
			updates.push_back(z);
		}
	}
	UpdateItems(updates);
}

bool NSMenu::MouseUp(NSEventMouse* event) {
	if (event->GetButton() != NSMouseButtonLeft)
		return false;
	
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
			Clear();
			return true;
		}
		if (i->submenu) {
			if (i->submenu->KeyDown(event)) {
				Clear();
				return true;
			}
		}
	}
	return false;
}

bool NSMenu::KeyUp(NSEventKey* event) {
	return false;
}

NSSize NSMenu::GetSize() {
	if (cached_size)
		return menu_size;
	
	cached_size = true;
	if (is_context_menu) {
		NSSize max_size;
		for (auto& i : items) {
			NSSize size = i->GetSize();
			max_size.height += size.height;
			if (max_size.width < size.width)
				max_size.width = size.width;
		}
		menu_size = max_size + NSSize(2 * MENU_OFFSET_X, 2 * MENU_OFFSET_Y);
		return menu_size;
	}
	
	NSSize max_size = NSSize(2 * MENU_OFFSET_X, 0);
	for (auto& i : items) {
		NSSize size = i->GetSize();
		max_size.width += size.width;
		if (max_size.height < size.height)
			max_size.height = size.height;
	}
	menu_size = max_size;
	return menu_size;
}

NSRect NSMenu::GetFrame() const {
	return frame;
}

void NSMenu::SetFrame(NSRect f) {
	NSRect old_frame = frame;
	frame = f;
	
	UpdateAll();
	if (update) {
		std::vector<NSRect> rects = { AdjustRect(old_frame) + NSRect(-1, -1, 2, 2) };
		update(rects);
	}
}

graphics_context_t* NSMenu::GetContext() const {
	return context;
}

void NSMenu::SetContext(graphics_context_t* c) {
	context = c;
}

bool RectsIntersect(const vector<NSRect>& rects, NSRect rect) {
	for (uint32_t z = 0; z < rects.size(); z++) {
		if (rects[z].OverlapsRect(rect))
			return true;
	}
	return false;
}

void NSMenu::Draw(const vector<NSRect>& rects) {
	if (!context)
		return;
	
	NSRect rect = AdjustRect(frame);
	NSRect border_rect = rect + NSRect(-1, -1, 2, 2);
	
	// Redraw background if needed
	bool draw_background = false;
	if (RectsIntersect(rects, border_rect))
		draw_background = true;
	
	if (draw_background) {
		// Draw border
		NSMatrix matrix = NSMatrix::Identity();
		matrix.Translate(border_rect.origin.x, border_rect.origin.y, 0);
		matrix.Scale(border_rect.size.width, border_rect.size.height, 1);
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
	}
	
	// Draw menu items
	if (is_context_menu) {
		float pos_y = MENU_OFFSET_Y;
		for (auto i : items) {
			NSRect item_rect = NSRect(NSPoint(rect.origin.x, rect.origin.y + pos_y),
									  NSSize(rect.size.width, i->GetSize().height));
			if (RectsIntersect(rects, item_rect))
				i->Draw(context, item_rect.origin, item_rect.size);
			pos_y += item_rect.size.height;
		}
	} else {
		// Straight bar menu
		float pos_x = MENU_OFFSET_X;
		for (auto i : items) {
			NSRect item_rect = NSRect(NSPoint(pos_x + rect.origin.x, rect.origin.y),
									  NSSize(i->GetSize().width, rect.size.height));
			if (RectsIntersect(rects, item_rect))
				i->Draw(context, item_rect.origin, item_rect.size);
			pos_x += item_rect.size.width;
		}
	}
	
	if (submenu)
		submenu->Draw(rects);
}

void NSMenu::SetUpdateFunction(std::function<void(std::vector<NSRect>)> func) {
	update = func;
	if (submenu)
		submenu->SetUpdateFunction(func);
}

void NSMenu::UpdateAll() {
	if (!update)
		return;
	
	cached_size = false;
	std::vector<NSRect> rects = { AdjustRect(frame) + NSRect(-1, -1, 2, 2) };
	update(rects);
}

void NSMenu::UpdateItem(unsigned int index) {
	std::vector<unsigned int> indices = { index };
	UpdateItems(indices);
}

void NSMenu::UpdateItems(std::vector<unsigned int> indices) {
	if (!update || indices.size() == 0)
		return;
	
	cached_size = false;
	NSRect rect = AdjustRect(frame);
	std::vector<NSRect> rects;
	float pos = is_context_menu ? MENU_OFFSET_Y : MENU_OFFSET_X;
	for (unsigned int z = 0; z < items.size(); z++) {
		auto i = items[z];
		NSRect item_rect;
		if (is_context_menu) {
			item_rect = NSRect(NSPoint(rect.origin.x, rect.origin.y + pos),
								  NSSize(rect.size.width, i->GetSize().height));
		} else {
			item_rect = NSRect(NSPoint(pos + rect.origin.x, rect.origin.y),
							   NSSize(i->GetSize().width, rect.size.height));
		}
		bool found = false;
		for (unsigned int q = 0; q < indices.size(); q++) {
			if (indices[q] == z) {
				found = true;
				break;
			}
		}
		if (found)
			rects.push_back(item_rect);
		pos += is_context_menu ? item_rect.size.height : item_rect.size.width;
	}
	
	update(rects);
}
