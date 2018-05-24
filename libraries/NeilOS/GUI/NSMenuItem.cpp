//
//  NSMenuItem.cpp
//  product
//
//  Created by Neil Singh on 5/23/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSMenuItem.h"

#include "../Core/NSFont.h"

#include <string.h>

#define MENU_OFFSET_X		10
#define MENU_OFFSET_Y		5

#define MENU_COLOR_BACKGROUND	0
#define MENU_COLOR_HIGHLIGHTED	1
#define MENU_COLOR_BORDER		2
#define MENU_COLOR_BLACK		3
#define MENU_COLOR_WHITE		4

#define SEPARATOR_VERTICAL_WIDTH	2
#define SEPARATOR_VERTICAL_OFFSET	3
#define SEPARATOR_HORIZONTAL_HEIGHT	2
#define SEPARATOR_HORIZONTAL_OFFSET	10

using std::string;

NSMenuItem* NSMenuItem::SeparatorItem() {
	NSMenuItem* ret = new NSMenuItem;
	ret->SetIsSeparator(true);
	return ret;
}

NSMenuItem::NSMenuItem() {
}

NSMenuItem::~NSMenuItem() {
	if (submenu)
		delete submenu;
	if (image)
		delete image;
	
	if (text_buffer)
		graphics_buffer_destroy(text_buffer);
	if (img_buffer)
		graphics_buffer_destroy(img_buffer);
	if (key_buffer)
		graphics_buffer_destroy(key_buffer);
}

NSMenuItem::NSMenuItem(std::string t, string key, NSModifierFlags f) {
	SetTitle(t);
	key_equivalent = key;
	flags = f;
}

NSMenuItem::NSMenuItem(NSImage* i, string key, NSModifierFlags f) {
	SetImage(i);
	key_equivalent = key;
	flags = f;
}

NSMenu* NSMenuItem::GetSubmenu() {
	if (!submenu) {
		submenu = new NSMenu;
		submenu->SetIsContextMenu(true);
	}
	return submenu;
}

void NSMenuItem::SetSubmenu(NSMenu* menu) {
	if (submenu)
		delete submenu;
	submenu = menu;
	submenu->SetIsContextMenu(true);
	
	Update();
}

string NSMenuItem::GetTitle() const {
	return title;
}

void NSMenuItem::SetTitle(string t) {
	title = t;
	
	if (text_buffer)
		graphics_buffer_destroy(text_buffer);
	
	NSFont font;
	NSImage* text = font.GetImage(title, NSColor<float>::WhiteColor());
	if (!text) {
		text_buffer = 0;
		SetTitle(" ");
		return;
	}
	text_size = text->GetSize();
	font_height = font.GetLineHeight();
	int w = int(text_size.width + 0.5);
	int h = int(text_size.height + 0.5);
	if (w == 0 || h == 0) {
		delete text;
		text_buffer = 0;
		SetTitle(" ");
		return;
	}
	text_buffer = graphics_buffer_create(w, h, GRAPHICS_BUFFER_STATIC, GRAPHICS_FORMAT_A8R8G8B8);
	graphics_buffer_data(text_buffer, text->GetPixelData(), w, h, w * h * 4);
	delete text;
	
	Update(menu && !menu->is_context_menu);
}

NSImage* NSMenuItem::GetImage() const {
	return image;
}

void NSMenuItem::SetImage(const NSImage* i) {
	if (image)
		delete image;
	image = new NSImage(i->GetPixelData(), i->GetSize(), 32);
	
	if (img_buffer)
		graphics_buffer_destroy(img_buffer);
	
	// Make image white
	uint32_t* data = image->GetPixelData();
	for (int y = 0; y < image->GetSize().width; y++) {
		for (int x = 0; x < image->GetSize().height; x++) {
			uint32_t pos = y * int(image->GetSize().width + 0.5) + x;
			data[pos] = data[pos] | 0xFFFFFF;
		}
	}
	
	int w = int(img_size.width + 0.5);
	int h = int(img_size.height + 0.5);
	img_size = image->GetSize();
	img_buffer = graphics_buffer_create(w, h, GRAPHICS_BUFFER_STATIC, GRAPHICS_FORMAT_A8R8G8B8);
	graphics_buffer_data(img_buffer, image->GetPixelData(), w, h, w * h * 4);
	
	Update(menu && !menu->is_context_menu);
}

bool NSMenuItem::IsSeparator() const {
	return is_separator;
}

void NSMenuItem::SetIsSeparator(bool is) {
	is_separator = is;
	
	Update(true);
}

std::string NSMenuItem::GetKeyEquivalent() const {
	return key_equivalent;
}

NSModifierFlags NSMenuItem::GetKeyModifierFlags() const {
	return flags;
}

void NSMenuItem::SetKeyEquivalent(string key, NSModifierFlags f) {
	for (auto& c : key)
		c = toupper(c);
	
	key_equivalent = key;
	flags = f;
	
	if (key_buffer)
		graphics_buffer_destroy(key_buffer);
	
	if (key.length() == 0) {
		key_size = NSSize();
		key_buffer = 0;
		Update(menu && !menu->is_context_menu);
		return;
	}
	
	string t;
	if (f & NSModifierControl)
		t.append("^");
	if (f & NSModifierOption)
		t.append("◦");
	if (f & NSModifierShift)
		t.append("↑");
	if (f & NSModifierCommand)
		t.append("§");
	if (f & NSModifierFunction)
		t.append("ƒ");
	t.append(key);
	
	NSFont font;
	NSImage* text = font.GetImage(t, NSColor<float>::WhiteColor());
	key_size = text->GetSize();
	font_height = font.GetLineHeight();
	int w = int(key_size.width + 0.5);
	int h = int(key_size.height + 0.5);
	key_buffer = graphics_buffer_create(w, h, GRAPHICS_BUFFER_STATIC, GRAPHICS_FORMAT_A8R8G8B8);
	graphics_buffer_data(key_buffer, text->GetPixelData(), w, h, w * h * 4);
	delete text;
	
	Update(menu && !menu->is_context_menu);
}

NSSize NSMenuItem::GetSize() const {
	if (!menu)
		return NSSize();
	if (is_separator) {
		if (menu->is_context_menu)
			return NSSize(2 * SEPARATOR_HORIZONTAL_OFFSET + 30, 15);
		else
			return NSSize(2 * MENU_OFFSET_X + 2 * SEPARATOR_VERTICAL_WIDTH, 15);
	}
	float x_pad = menu->is_context_menu ? 0 : 2 * MENU_OFFSET_X;
	if (submenu && menu->is_context_menu)
		x_pad += font_height;	// for triangle
	else if (!submenu && menu->is_context_menu)
		x_pad += key_size.width;
	return NSSize(text_size.width, font_height) + NSSize(x_pad, font_height * 1.5);
}

std::function<void(NSMenuItem*)> NSMenuItem::GetAction() const {
	return action;
}

void NSMenuItem::SetAction(std::function<void(NSMenuItem*)> a) {
	action = a;
}

bool NSMenuItem::GetIsEnabled() const {
	return enabled;
}

void NSMenuItem::SetIsEnabled(bool e) {
	if (enabled == e)
		return;
	
	enabled = e;
	Update();
}

void NSMenuItem::Update(bool all) {
	if (!menu)
		return;
	
	for (unsigned int z = 0; z < menu->items.size(); z++) {
		if (menu->items[z] == this) {
			if (all) {
				menu->updates.push_back(-1);
				return;
			}
			menu->updates.push_back(z);
		}
	}
}

void NSMenuItem::Draw(graphics_context_t* context, NSPoint point, NSSize size) {
	if (!menu)
		return;
	
	NSRect text_frame;
	if (menu->is_context_menu) {
		text_frame = NSRect(NSPoint(point.x + MENU_OFFSET_X,
									point.y + (size.height - font_height) / 2), text_size);
	} else {
		text_frame = NSRect(NSPoint(point.x + (size.width - text_size.width) / 2,
									point.y + (size.height - font_height) / 2), text_size);
	}
	NSRect frame = NSRect(point, size);
	
	NSMatrix matrix = NSMatrix::Identity();
	matrix.Translate(frame.origin.x, frame.origin.y, 0);
	matrix.Scale(frame.size.width, frame.size.height, 1);
	graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
	graphics_vertex_array_t	square[3];
	memcpy(square, menu->square_vao, sizeof(square));
	square[2].bid = menu->color_vbo[highlighted];
	graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square, 3);
	
	if (is_separator) {
		if (menu->is_context_menu) {
			frame.origin.x = frame.origin.x + SEPARATOR_HORIZONTAL_OFFSET;
			frame.size.width -= SEPARATOR_HORIZONTAL_OFFSET * 2;
			frame.origin.y = frame.origin.y + (frame.size.height - SEPARATOR_HORIZONTAL_HEIGHT) / 2;
			frame.size.height = SEPARATOR_HORIZONTAL_HEIGHT;
		} else {
			frame.origin.x = frame.origin.x + (frame.size.width - SEPARATOR_VERTICAL_WIDTH) / 2;
			frame.size.width = SEPARATOR_VERTICAL_WIDTH;
			frame.origin.y += SEPARATOR_VERTICAL_OFFSET;
			frame.size.height = size.height - SEPARATOR_VERTICAL_OFFSET * 2;
		}
		matrix = NSMatrix::Identity();
		matrix.Translate(frame.origin.x, frame.origin.y, 0);
		matrix.Scale(frame.size.width, frame.size.height, 1);
		graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
		square[2].bid = menu->color_vbo[MENU_COLOR_BORDER];
		graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square, 3);
		return;
	}
	
	// Draw text
	matrix = NSMatrix::Identity();
	matrix.Translate(text_frame.origin.x, text_frame.origin.y, 0);
	matrix.Scale(text_frame.size.width, text_frame.size.height, 1);
	graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, text_buffer);
	graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
	square[2].bid = menu->color_vbo[enabled ? (MENU_COLOR_BLACK + highlighted) : MENU_COLOR_BORDER];
	graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square, 3);
	
	graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
	if (submenu && menu->is_context_menu) {
		// Draw triangle
		matrix = NSMatrix::Identity();
		matrix.Translate(frame.origin.x + frame.size.width - MENU_OFFSET_X - font_height,
						 frame.origin.y + (frame.size.height - font_height) / 2, 0);
		matrix.Scale(font_height, font_height, 1);
		graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
		graphics_vertex_array_t	triangle[2];
		memcpy(triangle, menu->triangle_vao, sizeof(triangle));
		triangle[1].bid = menu->color_vbo[enabled ? (MENU_COLOR_BLACK + highlighted) : MENU_COLOR_BORDER];
		graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLELIST, 1, triangle, 2);
	} else if (!submenu && menu->is_context_menu && key_equivalent.length() != 0) {
		// Draw key equivalent
		matrix = NSMatrix::Identity();
		matrix.Translate(frame.origin.x + frame.size.width - MENU_OFFSET_X - key_size.width,
						 frame.origin.y + (frame.size.height - font_height) / 2, 0);
		matrix.Scale(key_size.width, key_size.height, 1);
		graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, key_buffer);
		graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
		square[2].bid = menu->color_vbo[enabled ? (MENU_COLOR_BLACK + highlighted) : MENU_COLOR_BORDER];
		graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square, 3);
		
		graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
	}
}
