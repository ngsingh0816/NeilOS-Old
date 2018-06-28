//
//  NSMenu.cpp
//  product
//
//  Created by Neil Singh on 5/21/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSMenu.h"

#include "NSMenuItem.h"
#include "NSView.h"

#include "../Core/Events/NSEventDefs.cpp"

#include <string.h>

using std::vector;

#define BORDER_SIZE		1

#define MENU_OFFSET_X	10
#define MENU_OFFSET_Y	5
#define MENU_MIN_WIDTH	200

#define CHECK_SIZE		NSSize(MENU_OFFSET_X, MENU_OFFSET_X)

#define MENU_COLOR_BACKGROUND	0
#define MENU_COLOR_HIGHLIGHTED	1
#define MENU_COLOR_BORDER		2
#define MENU_COLOR_BLACK		3
#define MENU_COLOR_WHITE		4

#define ROUND_BOTTOM			(1 << 0)
#define ROUND_TOP				(1 << 1)
#define ROUND_ALL				0x3

#define NUM_VERTICES_ROUNDED	22

namespace NSApplication {
	bool SendEvent(NSEvent* event);
}

namespace {
	bool init = false;
	uint32_t menu_square_vbo = 0;
	uint32_t menu_check_vbo = 0;
	uint32_t menu_triangle_vbo = 0;
	
	NSMenu* context_menu = NULL;
	unsigned int context_menu_id = 0;
	
	void Init() {
		if (init)
			return;
		
		menu_square_vbo = graphics_buffer_create(sizeof(float) * 4 * 2,
												 GRAPHICS_BUFFER_STATIC | GRAPHICS_BUFFER_VERTEXBUFFER);
		NSView::BufferSquare(menu_square_vbo);
		
		const float triangle[] = {
			0, 0,
			0, 1.0f,
			1.0f, 0.5f
		};
		menu_triangle_vbo = graphics_buffer_create(sizeof(triangle),
												   GRAPHICS_BUFFER_STATIC | GRAPHICS_BUFFER_VERTEXBUFFER);
		graphics_buffer_data(menu_triangle_vbo, triangle, sizeof(triangle));
		
		menu_check_vbo = graphics_buffer_create(sizeof(float) * NSVIEW_CHECKMARK_VERTICES * 2,
												 GRAPHICS_BUFFER_STATIC | GRAPHICS_BUFFER_VERTEXBUFFER);
		NSView::BufferCheckmark(menu_check_vbo);
		
		init = true;
	}
}

namespace NSMenuInternal {
	unsigned int GetCurrentContextMenuID() {
		return context_menu_id;
	}
	
	void DeallocContextMenu() {
		if (context_menu) {
			delete context_menu;
			context_menu = NULL;
		}
	}
}

// Screen coordinates
void NSMenu::PopupContextMenu(NSMenu* menu, const NSPoint& p) {
	if (context_menu) {
		delete context_menu;
		context_menu = NULL;
	}
	
	if (menu) {
		menu->SetIsContextMenu(true);
		context_menu = new NSMenu(*menu);
		context_menu_id++;
	}
	NSEventApplicationContextMenu* e = NSEventApplicationContextMenu::Create(getpid(), context_menu, p, context_menu_id);
	if (!e)
		return;
	NSApplication::SendEvent(e);
	delete e;
}

NSMenu* NSMenu::GetCurrentContextMenu() {
	return context_menu;
}

unsigned int NSMenu::GetVerticesForColor(unsigned int i) {
	if (i == MENU_COLOR_BACKGROUND || MENU_COLOR_BORDER)
		return NUM_VERTICES_ROUNDED;
	else if (i > MENU_COLOR_BORDER)
		return NSVIEW_CHECKMARK_VERTICES;
	return 4;
}

void NSMenu::SetupVAO() {
	Init();
	
	square_vbo = menu_square_vbo;
	triangle_vbo = menu_triangle_vbo;
	check_vbo = menu_check_vbo;
	rounded_vbo = graphics_buffer_create(sizeof(float) * NUM_VERTICES_ROUNDED * 2, GRAPHICS_BUFFER_STATIC);
	
	NSColor<float> c[5] = { background_color, highlight_color, border_color, text_color,
		text_highlight_color };
	
	for (unsigned int i = 0; i < sizeof(c) / sizeof(NSColor<float>); i++) {
		uint32_t num_verts = GetVerticesForColor(i);
		color_vbo[i] = graphics_buffer_create(sizeof(float) * num_verts * 4,
											  GRAPHICS_BUFFER_STATIC | GRAPHICS_BUFFER_VERTEXBUFFER);
		NSView::BufferColor(color_vbo[i], c[i], num_verts);
	}
	
	memset(square_vao, 0, sizeof(graphics_vertex_array_t) * 3);
	square_vao[0].bid = square_vbo;
	square_vao[0].stride = 2 * sizeof(float);
	square_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	square_vao[0].usage = GRAPHICS_USAGE_POSITION;
	square_vao[1].bid = square_vbo;
	square_vao[1].stride = 2 * sizeof(float);
	square_vao[1].type = GRAPHICS_TYPE_FLOAT2;
	square_vao[1].usage = GRAPHICS_USAGE_TEXCOORD;
	square_vao[2].bid = color_vbo[0];
	square_vao[2].stride = 4 * sizeof(float);
	square_vao[2].type = GRAPHICS_TYPE_FLOAT4;
	square_vao[2].usage = GRAPHICS_USAGE_COLOR;

	memset(triangle_vao, 0, sizeof(graphics_vertex_array_t) * 2);
	triangle_vao[0].bid = triangle_vbo;
	triangle_vao[0].stride = 2 * sizeof(float);
	triangle_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	triangle_vao[0].usage = GRAPHICS_USAGE_POSITION;
	triangle_vao[1].bid = color_vbo[MENU_COLOR_BLACK];
	triangle_vao[1].stride = 4 * sizeof(float);
	triangle_vao[1].type = GRAPHICS_TYPE_FLOAT4;
	triangle_vao[1].usage = GRAPHICS_USAGE_COLOR;
	
	memset(rounded_vao, 0, sizeof(graphics_vertex_array_t) * 2);
	rounded_vao[0].bid = rounded_vbo;
	rounded_vao[0].stride = 2 * sizeof(float);
	rounded_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	rounded_vao[0].usage = GRAPHICS_USAGE_POSITION;
	rounded_vao[1].bid = color_vbo[MENU_COLOR_BACKGROUND];
	rounded_vao[1].stride = 4 * sizeof(float);
	rounded_vao[1].type = GRAPHICS_TYPE_FLOAT4;
	rounded_vao[1].usage = GRAPHICS_USAGE_COLOR;
}

NSMenu::NSMenu(const NSMenu& menu) {
	SetupVAO();
	
	*this = menu;
}

NSMenu& NSMenu::operator=(const NSMenu& menu) {
	if (&menu == this)
		return *this;
	
	for (auto& i : items)
		delete i;
	items.resize(menu.items.size());
	for (unsigned int z = 0; z < menu.items.size(); z++) {
		items[z] = menu.items[z]->Clone();
		items[z]->highlighted = false;
		items[z]->SetMenu(this);
	}
	submenu = NULL;
	SetIsContextMenu(menu.is_context_menu);
	menu_size = menu.menu_size;
	cached_size = menu.cached_size;
	frame = menu.frame;
	frame_bounds = menu.frame_bounds;
	orientation = menu.orientation;
	mouse_down = menu.mouse_down;
	mouse_captured = menu.mouse_captured;
	update = menu.update;
	context = menu.context;
	border_radius = menu.border_radius;
	round_edge = menu.round_edge;
	
	NSColor<float> colors[] = { menu.background_color, menu.highlight_color, menu.border_color,
		menu.text_color, menu.text_highlight_color };
	SetColors(colors);
	
	return *this;
}

NSMenu* NSMenu::FromData(const uint8_t* data, uint32_t length, uint32_t* length_used) {
#ifdef copy
#undef copy
#endif
#define copy(x, len) { if (pos >= length) { return NULL; }; memcpy(x, &data[pos], (len)); pos += (len); }
	if (length < sizeof(uint32_t) * 6 + sizeof(bool) + sizeof(float) * 4 + sizeof(NSMenuOrientation))
		return NULL;
	
	uint32_t pos = 0;
	
	bool cm;
	NSRect frame;
	float border_radius;
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
	copy(&border_radius, sizeof(float));
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
	menu->border_radius = border_radius;
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
		if (!item_ptr[z]) {
			for (unsigned int i = 0; i < z; i++)
				delete[] item_ptr[i];
			return NULL;
		}
		item_total_size += item_len[z];
	}
	
	total_length += sizeof(uint32_t);	// items size
	total_length += item_total_size;
	total_length += sizeof(bool);		// is_context_menu
	total_length += sizeof(float) * 4;	// frame
	total_length += sizeof(float);		// border radius
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
	copy(&border_radius, sizeof(float));
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
	
	SetIsContextMenu(is_context);
}

NSMenu::NSMenu(const std::vector<NSMenuItem*>& i, bool is_context) {
	SetupVAO();
	
	SetItems(i);
	is_context_menu = is_context;
}

NSMenu::~NSMenu() {
	for (unsigned int z = 0; z < sizeof(color_vbo) / sizeof(uint32_t); z++) {
		if (color_vbo[z])
			graphics_buffer_destroy(color_vbo[z]);
	}
	if (rounded_vbo)
		graphics_buffer_destroy(rounded_vbo);
	
	for (auto& i : items)
		delete i;
}

void NSMenu::SetColor(NSColor<float> color, unsigned int index) {
	NSView::BufferColor(color_vbo[index], color, GetVerticesForColor(index));
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
	mouse_captured = c;
	if (c)
		round_edge = ROUND_ALL;
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
	for (auto& i : items)
		delete i;
	
	items = i;
	for (auto& c : items) {
		c->SetMenu(this);
		c->current_color = background_color;
		c->UpdateSize();
	}
	
	UpdateAll();
}

void NSMenu::AddItem(NSMenuItem* item) {
	item->SetMenu(this);
	item->current_color = background_color;
	item->UpdateSize();
	items.push_back(item);
	
	UpdateItem(items.size() - 1);
}

void NSMenu::AddItem(NSMenuItem* item, unsigned int index) {
	if (index > items.size())
		index = items.size();
	
	item->SetMenu(this);
	item->current_color = background_color;
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

uint32_t NSMenu::IndexOfItem(NSMenuItem* item) const {
	for (unsigned int z = 0; z < items.size(); z++) {
		if (item == items[z])
			return z;
	}
	return -1;
}

NSPoint NSMenu::GetOffsetOfItem(NSMenuItem* item) {
	NSRect rect = AdjustRect(frame);
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
		pos += is_context_menu ? item_rect.size.height : item_rect.size.width;
		if (i == item)
			return item_rect.origin;
	}
	return NSPoint();
}

NSRect NSMenu::AdjustRect(const NSRect& r) {
	NSRect rect = r;
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
		for (auto& i : submenu->items)
			i->Clear();
		
		submenu->ClearSubmenu();
		
		if (update) {
			std::vector<NSRect> rects = { submenu->AdjustRect(submenu->frame) };
			update(rects);
		}
		
		submenu = NULL;
	}
}

// down is true for mouse down, false for mouse move
bool NSMenu::MouseEvent(NSEventMouse* event, bool down) {
	// Send event to items
	NSEventMouse event_copy = *event;
	event_copy.SetPosition(event_copy.GetPosition() + frame.origin);
	auto func = down ? &NSMenuItem::MouseDown : (mouse_down ? &NSMenuItem::MouseDragged : &NSMenuItem::MouseMoved);
	NSView* backup_first_responder = first_responder;
	if (down)
		first_responder = NULL;
	for (auto& i : items) {
		bool ret = (i->*func)(&event_copy);
		if (down && ret)
			mouse_view = true;
	}
	if (down && first_responder != backup_first_responder && backup_first_responder != NULL) {
		first_responder = backup_first_responder;
		MakeFirstResponder(NULL);
	}
	
	if (mouse_view) {
		if (down && event->GetButton() == NSMouseButtonLeft) {
			mouse_down = true;
			mouse_captured = true;
		}
		return true;
	}
	
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
					if (is_context_menu)
						items[z]->Hover(false);
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
	
	float pos = (is_context_menu ? MENU_OFFSET_Y : MENU_OFFSET_X) + BORDER_SIZE;
	for (unsigned int z = 0; z < items.size(); z++) {
		auto& i = items[z];
		NSSize item_size = i->GetSize();
		if ((is_context_menu && (p.y >= pos && p.y < pos + item_size.height)) ||
			(!is_context_menu && (p.x >= pos && p.x < pos + item_size.width))) {
			// Unhighlight all the others
			for (unsigned int q = 0; q < items.size(); q++) {
				if (q != z && items[q]->highlighted) {
					items[q]->highlighted = false;
					if (is_context_menu)
						items[q]->Hover(false);
					updates.push_back(q);
				}
			}
			
			// Don't need to do anything if its already correct
			if (i->highlighted) {
				if (down && !is_context_menu) {
					i->highlighted = false;
					if (is_context_menu)
						i->Hover(false);
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
			if (is_context_menu)
				i->Hover(true);
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
				submenu->SetFrameBounds(frame_bounds);
				if (is_context_menu) {
					submenu->round_edge = ROUND_ALL;
					submenu->SetFrame(NSRect(frame.origin.x + rect.size.width
											 - BORDER_SIZE, frame.origin.y + pos - BORDER_SIZE - MENU_OFFSET_Y, 0, 0));
				} else {
					if (orientation == NSMenuOrientationUp) {
						submenu->round_edge = ROUND_TOP;
						NSRect fr = submenu->AdjustRect(NSRect(frame.origin.x + pos, frame.origin.y + BORDER_SIZE, 0, 0));
						fr.origin.y -= fr.size.height;
						submenu->SetFrame(fr);
					} else {
						submenu->round_edge = ROUND_BOTTOM;
						submenu->SetFrame(NSRect(frame.origin.x + pos,
												 frame.origin.y + rect.size.height - BORDER_SIZE, 0, 0));
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
		items[z]->Clear();
	}
	UpdateItems(updates);
}

bool NSMenu::MouseScrolled(NSEventMouse* event) {
	NSEventMouse event_copy = *event;
	for (auto& i : items)
		i->MouseScrolled(&event_copy);
	
	if (submenu) {
		if (submenu->MouseScrolled(event))
			return true;
	}
	
	NSRect rect = AdjustRect(frame);
	return rect.ContainsPoint(event->GetPosition());
}

bool NSMenu::MouseUp(NSEventMouse* event) {
	NSEventMouse event_copy = *event;
	event_copy.SetPosition(event_copy.GetPosition() + frame.origin);
	for (auto& i : items)
		i->MouseUp(&event_copy);
	if (mouse_view) {
		mouse_view = false;
		mouse_down = false;
		return false;
	}
	
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
			if (i->prevents_menu_clear || i->submenu)
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
	NSEventKey event_copy = *event;
	for (auto& i : items)
		i->KeyDown(&event_copy);
	
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
	NSEventKey event_copy = *event;
	for (auto& i : items)
		i->KeyUp(&event_copy);
	
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
		menu_size = max_size + NSSize(2 * MENU_OFFSET_X, 2 * MENU_OFFSET_Y) + NSSize(BORDER_SIZE * 2, BORDER_SIZE * 2);
	} else {
		NSSize max_size = NSSize(2 * MENU_OFFSET_X, 0);
		for (auto& i : items) {
			NSSize size = i->GetSize();
			max_size.width += size.width;
			if (max_size.height < size.height)
				max_size.height = size.height;
		}
		menu_size = max_size + NSSize(BORDER_SIZE * 2, BORDER_SIZE * 2);
	}
	
	NSRect rect = AdjustRect(frame);
	
	// Buffer rounded rect
	float borders[] = { border_radius, border_radius, 0, 0 };
	NSView::BufferRoundedRect(rounded_vbo, NSSize(rect.size.width, MENU_OFFSET_Y + BORDER_SIZE), borders,
							  NSVIEW_ROUNDED_UPPER_LEFT | NSVIEW_ROUNDED_UPPER_RIGHT, NUM_VERTICES_ROUNDED);
	
	return menu_size;
}

NSRect NSMenu::GetFrame() const {
	return frame;
}

NSRect NSMenu::GetFullFrame() {
	return AdjustRect(frame);
}

NSPoint NSMenu::GetOffset() const {
	return frame.origin;
}

void NSMenu::SetFrame(const NSRect& f) {
	NSRect old_frame = frame;
	frame = f;
	
	NSRect real_frame = AdjustRect(frame);
	if (is_context_menu && frame_bounds.size != NSSize(0, 0) && !frame_bounds.ContainsRect(real_frame)) {
		if (frame.origin.x < frame_bounds.origin.x)
			frame.origin.x = frame_bounds.origin.x;
		if (frame.origin.y < frame_bounds.origin.y)
			frame.origin.y = frame_bounds.origin.y;
		if (frame.origin.x + real_frame.size.width > frame_bounds.origin.x + frame_bounds.size.width)
			frame.origin.x = frame_bounds.origin.x + frame_bounds.size.width - real_frame.size.width;
		if (frame.origin.y + real_frame.size.height > frame_bounds.origin.y + frame_bounds.size.height)
			frame.origin.y = frame_bounds.origin.y + frame_bounds.size.height - real_frame.size.height;
	}
	
	if (old_frame != frame) {
		UpdateAll();
		if (update) {
			std::vector<NSRect> rects = { AdjustRect(old_frame) };
			update(rects);
		}
	}
}

NSRect NSMenu::GetFrameBounds() const {
	return frame_bounds;
}

void NSMenu::SetFrameBounds(const NSRect& bounds) {
	frame_bounds = bounds;
	SetFrame(frame);
}

float NSMenu::GetBorderRadius() const {
	return border_radius;
}

void NSMenu::SetBorderRadius(float radius) {
	border_radius = radius;
	cached_size = false;
	GetSize();
}

graphics_context_t* NSMenu::GetContext() {
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
	
	NSRect border_rect = AdjustRect(frame);
	NSRect rect = border_rect + NSRect(BORDER_SIZE, BORDER_SIZE, -2 * BORDER_SIZE, -2 * BORDER_SIZE);
	
	// Draw background if needed
	if (RectsIntersect(rects, border_rect)) {
		NSRect old_border = border_rect;
		NSRect old_rect = rect;
		
		if (is_context_menu) {
			if (round_edge & ROUND_TOP) {
				border_rect.origin.y += MENU_OFFSET_Y + BORDER_SIZE;
				border_rect.size.height -= MENU_OFFSET_Y + BORDER_SIZE;
				rect.origin.y += MENU_OFFSET_Y;
				rect.size.height -= MENU_OFFSET_Y;
			}
			if (round_edge & ROUND_BOTTOM) {
				border_rect.size.height -= MENU_OFFSET_Y + BORDER_SIZE;
				rect.size.height -= MENU_OFFSET_Y;
			}
		}
		
		// Draw border
		NSMatrix matrix = NSMatrix::Identity();
		matrix.Translate(border_rect.origin.x, border_rect.origin.y, 0);
		matrix.Scale(border_rect.size.width, border_rect.size.height, 1);
		graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
		square_vao[2].bid = color_vbo[MENU_COLOR_BORDER];
		graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square_vao, 3);
		
		if (is_context_menu) {
			if (round_edge & ROUND_TOP) {
				matrix = NSMatrix::Identity();
				matrix.Translate(border_rect.origin.x, border_rect.origin.y - (MENU_OFFSET_Y + BORDER_SIZE), 0);
				graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
				rounded_vao[1].bid = color_vbo[MENU_COLOR_BORDER];
				graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLEFAN, NUM_VERTICES_ROUNDED - 2, rounded_vao, 2);
			}
			if (round_edge & ROUND_BOTTOM) {
				matrix = NSMatrix::Identity();
				matrix.Translate(border_rect.origin.x, border_rect.origin.y + border_rect.size.height, 0);
				// Account for rotation
				matrix.Translate(border_rect.size.width, MENU_OFFSET_Y + BORDER_SIZE, 0);
				matrix.Rotate(0, 0, 1, 180);
				graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
				rounded_vao[1].bid = color_vbo[MENU_COLOR_BORDER];
				graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLEFAN, NUM_VERTICES_ROUNDED - 2, rounded_vao, 2);
			}
		}
		
		// Draw bar
		matrix = NSMatrix::Identity();
		matrix.Translate(rect.origin.x, rect.origin.y, 0);
		matrix.Scale(rect.size.width, rect.size.height, 1);
		graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
		square_vao[2].bid = color_vbo[MENU_COLOR_BACKGROUND];
		graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square_vao, 3);
		
		if (is_context_menu) {
			if (round_edge & ROUND_TOP) {
				matrix = NSMatrix::Identity();
				matrix.Translate(old_rect.origin.x, old_rect.origin.y, 0);
				matrix.Scale(1 - (2 * BORDER_SIZE / old_border.size.width),
							 1 - (2 * BORDER_SIZE / old_border.size.height), 1);
				graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
				rounded_vao[1].bid = color_vbo[MENU_COLOR_BACKGROUND];
				graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLEFAN, NUM_VERTICES_ROUNDED - 2, rounded_vao, 2);
			}
			if (round_edge & ROUND_BOTTOM) {
				matrix = NSMatrix::Identity();
				matrix.Translate(rect.origin.x, rect.origin.y + rect.size.height, 0);
				// Account for rotation (and scaling)
				matrix.Translate(border_rect.size.width - 2 * BORDER_SIZE, MENU_OFFSET_Y, 0);
				matrix.Rotate(0, 0, 1, 180);
				matrix.Scale(1 - (2 * BORDER_SIZE / old_border.size.width),
							 1 - (2 * BORDER_SIZE / old_border.size.height), 1);
				graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
				rounded_vao[1].bid = color_vbo[MENU_COLOR_BACKGROUND];
				graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLEFAN, NUM_VERTICES_ROUNDED - 2, rounded_vao, 2);
			}
		}
		
		border_rect = old_border;
		rect = old_rect;
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

void NSMenu::AddUpdateRects(const std::vector<NSRect>& rects) {
	if (update)
		update(rects);
}

void NSMenu::SetUpdateFunction(const std::function<void(std::vector<NSRect>)>& func) {
	update = func;
	if (submenu)
		submenu->SetUpdateFunction(func);
}

void NSMenu::UpdateAll() {
	if (!update)
		return;
	
	cached_size = false;
	std::vector<NSRect> rects = { AdjustRect(frame) };
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
