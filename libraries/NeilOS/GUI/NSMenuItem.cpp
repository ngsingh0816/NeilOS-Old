//
//  NSMenuItem.cpp
//  product
//
//  Created by Neil Singh on 5/23/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSMenuItem.h"

#include "../Core/NSFont.h"
#include "../Core/Events/NSEventResource.hpp"

#include "NSView.h"

#include <string.h>

#define MENU_OFFSET_X		10
#define MENU_OFFSET_Y		5

#define CHECK_SIZE			NSSize(MENU_OFFSET_X, MENU_OFFSET_X)

#define MENU_COLOR_BACKGROUND	0
#define MENU_COLOR_HIGHLIGHTED	1
#define MENU_COLOR_BORDER		2
#define MENU_COLOR_BLACK		3
#define MENU_COLOR_WHITE		4

#define SEPARATOR_VERTICAL_WIDTH	2
#define SEPARATOR_VERTICAL_OFFSET	3
#define SEPARATOR_HORIZONTAL_HEIGHT	2
#define SEPARATOR_HORIZONTAL_OFFSET	10

#define HOVER_DURATION				0.25

using std::string;

NSMenuItem* NSMenuItem::SeparatorItem() {
	NSMenuItem* ret = new NSMenuItem;
	ret->SetIsSeparator(true);
	return ret;
}

void NSMenuItem::Init() {
	border_height = 1.5 * MENU_OFFSET_Y;
	hover_buffer = graphics_buffer_create(sizeof(float) * 4 * 4, GRAPHICS_BUFFER_DYNAMIC);
}

NSMenuItem::NSMenuItem() {
	Init();
}

NSMenuItem::NSMenuItem(const NSMenuItem& item) {
	Init();
	
	*this = item;
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
	if (hover_buffer)
		graphics_buffer_destroy(hover_buffer);
}

NSMenuItem::NSMenuItem(std::string t, string key, NSModifierFlags f) {
	Init();

	SetTitle(t);
	key_equivalent = key;
	flags = f;
}

NSMenuItem::NSMenuItem(NSImage* i, bool chr, string key, NSModifierFlags f) {
	Init();

	SetImage(i, chr);
	key_equivalent = key;
	flags = f;
}

NSMenuItem& NSMenuItem::operator=(const NSMenuItem& item) {
	if (this == &item)
		return *this;
	
	menu = NULL;
	
	if (submenu) {
		delete submenu;
		submenu = NULL;
	}
	if (item.submenu)
		submenu = new NSMenu(*item.submenu);
	
	font = item.font;
	SetTitle(item.title);
	SetImage(item.image, item.chromatic);
	SetKeyEquivalent(item.key_equivalent, item.flags);
	border_height = item.border_height;
	is_separator = item.is_separator;
	highlighted = item.highlighted;
	action = item.action;
	enabled = item.enabled;
	item_size = item.item_size;
	user_data = item.user_data;
	font_height = item.font_height;
	current_color = item.current_color;
	
	return *this;
}

NSMenuItem* NSMenuItem::FromData(uint8_t* data, uint32_t length, uint32_t* length_used) {
#define copy(x, len) { if (pos >= length) { return NULL; }; memcpy(x, &data[pos], (len)); pos += (len); }
	if (length < sizeof(uint32_t) * 2 + sizeof(bool) * 5 + sizeof(float) * 4)
		return NULL;
	
	uint32_t pos = 0;
	NSFont* font_ptr = NSFont::FromData(data, length, &pos);
	if (!font_ptr)
		return NULL;
	NSFont font = *font_ptr;
	delete font_ptr;
	
	uint32_t title_len;
	copy(&title_len, sizeof(uint32_t));
	char title_buf[title_len + 1];
	copy(title_buf, title_len);
	title_buf[title_len] = 0;
	
	uint32_t key_len;
	copy(&key_len, sizeof(uint32_t));
	char key_buf[key_len + 1];
	copy(key_buf, key_len);
	key_buf[key_len] = 0;
	NSModifierFlags flags;
	copy(&flags, sizeof(NSModifierFlags));
	
	uint32_t image_length = 0;
	copy(&image_length, sizeof(uint32_t));
	uint32_t image_name_len;
	copy(&image_name_len, sizeof(uint32_t));
	char image_buf[image_name_len + 1];
	copy(image_buf, image_name_len);
	image_buf[image_name_len] = 0;
	NSImage* image = NULL;
	if (image_length != 0 && image_name_len != 0) {
		std::string image_name = std::string(image_buf);
		uint8_t* data = new uint8_t[image_length];
		if (NSEventResource::GetSharedResource(image_name, data, image_length))
			image = new NSImage(data, image_length);
		NSEventResource::DeleteSharedResource(image_name);
		delete[] data;
	}
	
	bool has_sub;
	NSMenu* submenu = NULL;
	copy(&has_sub, sizeof(bool));
	if (has_sub) {
		uint32_t submenu_length;
		submenu = NSMenu::FromData(&data[pos], length - pos, &submenu_length);
		if (!submenu)
			return NULL;
		pos += submenu_length;
	}
	
	float border_height;
	copy(&border_height, sizeof(float));
	bool is_separator, highlighted, enabled, chromatic;
	copy(&is_separator, sizeof(bool));
	copy(&highlighted, sizeof(bool));
	copy(&enabled, sizeof(bool));
	copy(&chromatic, sizeof(bool));
	
	if (pos + sizeof(float) * 2 >= length)
		return NULL;
	NSSize item_size = NSSize::FromData(&data[pos], sizeof(float) * 2);
	pos += sizeof(float) * 2;
	float font_height;
	copy(&font_height, sizeof(float));
	
	NSMenuItem* ret = new NSMenuItem;
	ret->font = font;
	ret->SetTitle(title_buf);
	ret->SetKeyEquivalent(key_buf, flags);
	if (image) {
		ret->SetImage(image, chromatic);
		delete image;
	}
	else
		ret->chromatic = chromatic;
	ret->submenu = submenu;
	ret->border_height = border_height;
	ret->is_separator = is_separator;
	ret->highlighted = highlighted;
	ret->enabled = enabled;
	ret->item_size = item_size;
	ret->font_height = font_height;
	
	if (length_used)
		*length_used = pos;
	
	return ret;
#undef copy
}

uint8_t* NSMenuItem::Serialize(uint32_t* length_out) {
#define copy(x, len) { memcpy(&buffer[pos], x, (len)); pos += (len); }
	// TODO: include image
	uint32_t total_length = 0;
	uint32_t font_length = 0;
	uint8_t* font_buf = font.Serialize(&font_length);
	uint32_t sub_length = 0;
	uint8_t* sub_buf = NULL;
	if (submenu)
		sub_buf = submenu->Serialize(&sub_length);
	uint32_t image_length = 0;
	std::string image_name;
	if (image) {
		// Don't abandon if representing image fails
		uint8_t* data = image->RepresentationUsingType(NSImage::NSImageTypePNG, &image_length);
		if (data) {
			image_name = NSEventResource::CreateSharedResource(data, image_length);
			delete[] data;
		}
	}
	total_length += font_length;
	total_length += sizeof(uint32_t);	// title length
	total_length += title.length();
	total_length += sizeof(uint32_t);	// key equivalent length
	total_length += key_equivalent.length();
	total_length += sizeof(NSModifierFlags);
	total_length += sizeof(uint32_t);	// image_length
	total_length += sizeof(uint32_t);	// image_name length
	total_length += image_name.length();
	total_length += sizeof(bool);		// has submenu
	total_length += sub_length;
	total_length += sizeof(float);		// border_height
	total_length += sizeof(bool);		// is_separator
	total_length += sizeof(bool);		// highlighted
	total_length += sizeof(bool);		// enabled
	total_length += sizeof(bool);		// chromatic
	total_length += sizeof(float) * 2;	// item_size
	total_length += sizeof(float);		// font_height
	
	uint8_t* buffer = new uint8_t[total_length];
	uint32_t pos = 0;
	
	copy(font_buf, font_length);
	delete[] font_buf;
	uint32_t title_length = title.length(), key_length = key_equivalent.length(),
		image_name_length = image_name.length();
	copy(&title_length, sizeof(uint32_t));
	copy(title.c_str(), title_length);
	copy(&key_length, sizeof(uint32_t));
	copy(key_equivalent.c_str(), key_length);
	copy(&flags, sizeof(uint32_t));
	copy(&image_length, sizeof(uint32_t));
	copy(&image_name_length, sizeof(uint32_t));
	copy(image_name.c_str(), image_name_length);
	bool has_sub = submenu != NULL;
	copy(&has_sub, sizeof(bool));
	if (has_sub) {
		copy(sub_buf, sub_length);
		delete[] sub_buf;
	}
	copy(&border_height, sizeof(float));
	copy(&is_separator, sizeof(bool));
	copy(&highlighted, sizeof(bool));
	copy(&enabled, sizeof(bool));
	copy(&chromatic, sizeof(bool));
	uint8_t* buf = item_size.Serialize(NULL);
	copy(buf, sizeof(float) * 2);
	delete[] buf;
	copy(&font_height, sizeof(float));
	
	if (length_out)
		*length_out = total_length;
	return buffer;
#undef copy
}

NSMenu* NSMenuItem::GetSubmenu() {
	return submenu;
}

void NSMenuItem::SetSubmenu(NSMenu* menu) {
	if (submenu)
		delete submenu;
	submenu = menu;
	submenu->SetIsContextMenu(true);
	
	Update();
}

NSMenu* NSMenuItem::GetSupermenu() {
	return menu;
}

string NSMenuItem::GetTitle() const {
	return title;
}

void NSMenuItem::SetTitle(string t) {
	title = t;
	
	if (text_buffer)
		graphics_buffer_destroy(text_buffer);
	
	if (title.length() == 0) {
		text_buffer = 0;
		text_size = NSSize();
		UpdateSize();
		Update(menu && !menu->is_context_menu);
		return;
	}
	
	NSImage* text = font.GetImage(title, NSColor<float>::WhiteColor());
	if (!text) {
		text_buffer = 0;
		SetTitle(" ");
		return;
	}
	text_size = text->GetScaledSize();
	font_height = font.GetLineHeight();
	int w = int(text->GetSize().width + 0.5);
	int h = int(text->GetSize().height + 0.5);
	if (w == 0 || h == 0) {
		delete text;
		text_buffer = 0;
		SetTitle(" ");
		return;
	}
	text_buffer = graphics_buffer_create(w, h, GRAPHICS_BUFFER_STATIC, GRAPHICS_FORMAT_A8R8G8B8);
	graphics_buffer_data(text_buffer, text->GetPixelData(), w, h, w * h * 4);
	delete text;
	
	UpdateSize();
	Update(menu && !menu->is_context_menu);
}

NSFont NSMenuItem::GetFont() const {
	return font;
}

void NSMenuItem::SetFont(NSFont f) {
	font = f;
	SetTitle(title);
}

NSImage* NSMenuItem::GetImage() const {
	return image;
}

void NSMenuItem::SetImage(const NSImage* i, bool chr) {
	chromatic = chr;

	if (image)
		delete image;
	if (!i) {
		image = NULL;
		img_size = NSSize();
		if (img_buffer) {
			graphics_buffer_destroy(img_buffer);
			img_buffer = 0;
		}
		UpdateSize();
		Update(menu && !menu->is_context_menu);
		return;
	}
	image = new NSImage(i->GetPixelData(), i->GetSize(), 32);
	
	if (img_buffer)
		graphics_buffer_destroy(img_buffer);
	
	// Make image white
	if (!chromatic) {
		uint32_t* data = image->GetPixelData();
		for (int y = 0; y < image->GetSize().height; y++) {
			for (int x = 0; x < image->GetSize().width; x++) {
				uint32_t pos = y * int(image->GetSize().width + 0.5) + x;
				data[pos] = data[pos] | 0xFFFFFF;
			}
		}
	}
	
	int w = int(image->GetSize().width + 0.5);
	int h = int(image->GetSize().height + 0.5);
	img_size = image->GetScaledSize();
	img_buffer = graphics_buffer_create(w, h, GRAPHICS_BUFFER_STATIC, GRAPHICS_FORMAT_A8R8G8B8);
	graphics_buffer_data(img_buffer, image->GetPixelData(), w, h, w * h * 4);
	
	UpdateSize();
	Update(menu && !menu->is_context_menu);
}

bool NSMenuItem::IsSeparator() const {
	return is_separator;
}

void NSMenuItem::SetIsSeparator(bool is) {
	is_separator = is;
	
	UpdateSize();
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
		UpdateSize();
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
	
	NSFont default_font;
	NSImage* text = default_font.GetImage(t, NSColor<float>::WhiteColor());
	key_size = text->GetScaledSize();
	font_height = font.GetLineHeight();
	int w = int(text->GetSize().width + 0.5);
	int h = int(text->GetSize().height + 0.5);
	key_buffer = graphics_buffer_create(w, h, GRAPHICS_BUFFER_STATIC, GRAPHICS_FORMAT_A8R8G8B8);
	graphics_buffer_data(key_buffer, text->GetPixelData(), w, h, w * h * 4);
	delete text;
	
	UpdateSize();
	Update(menu && !menu->is_context_menu);
}

NSSize NSMenuItem::GetSize() const {
	return item_size;
}

void NSMenuItem::SetSize(const NSSize& size) {
	item_size = size;
	Update(menu);
}

void NSMenuItem::UpdateSize() {
	if (!menu) {
		item_size = NSSize();
		return;
	}
	if (is_separator) {
		if (menu->is_context_menu)
			item_size = NSSize(2 * SEPARATOR_HORIZONTAL_OFFSET + 30, 15);
		else
			item_size = NSSize(2 * MENU_OFFSET_X + 2 * SEPARATOR_VERTICAL_WIDTH, 15);
		return;
	}
	float x_pad = menu->is_context_menu ? MENU_OFFSET_X : 2 * MENU_OFFSET_X;
	if (submenu && menu->is_context_menu)
		x_pad += font_height;	// for triangle
	else if (!submenu && menu->is_context_menu)
		x_pad += key_size.width;
	if (image && title.length() != 0)
		x_pad += MENU_OFFSET_X;
	
	float height = font_height;
	if (height < img_size.height)
		height = img_size.height;
	item_size = NSSize(img_size.width + text_size.width, height) + NSSize(x_pad, 2 * border_height);
}

std::function<void(NSMenuItem*)> NSMenuItem::GetAction() const {
	return action;
}

void NSMenuItem::SetAction(const std::function<void(NSMenuItem*)>& a) {
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

float NSMenuItem::GetBorderHeight() const {
	return border_height;
}

void NSMenuItem::SetBorderHeight(float height) {
	border_height = height;
	UpdateSize();
	Update();
}

NSMenuItemState NSMenuItem::GetState() const {
	return state;
}

void NSMenuItem::SetState(NSMenuItemState s) {
	state = s;
	Update();
}

void NSMenuItem::SetUserData(void* data) {
	user_data = data;
}

void* NSMenuItem::GetUserData() const {
	return user_data;
}

void NSMenuItem::Hover(bool over) {
	if (!menu)
		return;
	
	if (hover_animation)
		hover_animation->Cancel();
	NSColor<float> start = current_color;
	NSColor<float> end = over ? menu->highlight_color : menu->background_color;
	hover_animation = new NSAnimation([start, end, this](const NSAnimation* a) {
		current_color = start + (end - start) * a->GetValue();
		NSView::BufferColor(hover_buffer, current_color, 4);
		Update();
	}, [end, this](const NSAnimation* a, bool canceled) {
		hover_animation = NULL;
		if (!canceled) {
			current_color = end;
			Update();
		}
		delete a;
	}, HOVER_DURATION);
	hover_animation->Start();
}

void NSMenuItem::Update(bool all) {
	if (!menu)
		return;
	
	if (all) {
		menu->UpdateAll();
		return;
	}
	
	std::vector<unsigned int> updates;
	for (unsigned int z = 0; z < menu->items.size(); z++) {
		if (menu->items[z] == this)
			updates.push_back(z);
	}
	menu->UpdateItems(updates);
}

void NSMenuItem::Draw(graphics_context_t* context, NSPoint point, NSSize size) {
	if (!menu)
		return;
	
	NSRect image_frame;
	NSSize combined_size = img_size + text_size;
	if (image && title.length() != 0)
		combined_size.width += MENU_OFFSET_X;
	if (menu->is_context_menu) {
		image_frame = NSRect(NSPoint(point.x + 2 * MENU_OFFSET_X,
									point.y + (size.height - img_size.height) / 2), img_size);
	} else {
		image_frame = NSRect(NSPoint(point.x + (size.width - combined_size.width) / 2,
									point.y + (size.height - img_size.height) / 2), img_size);
	}
	
	NSRect text_frame;
	NSPoint real_point = point;
	if (image) {
		if (menu->is_context_menu)
			real_point = NSPoint(image_frame.origin.x + image_frame.size.width, point.y);
		else
			real_point.x += (text_size.width - combined_size.width) / 2 + image_frame.size.width + MENU_OFFSET_X;
	}
	if (menu->is_context_menu) {
		text_frame = NSRect(NSPoint(real_point.x + 2 * MENU_OFFSET_X,
									real_point.y + (size.height - font_height) / 2), text_size);
	} else {
		text_frame = NSRect(NSPoint(real_point.x + (size.width - text_size.width) / 2,
									real_point.y + (size.height - font_height) / 2), text_size);
	}
	NSRect frame = NSRect(point, size);
	
	NSMatrix matrix = NSMatrix::Identity();
	matrix.Translate(frame.origin.x, frame.origin.y, 0);
	matrix.Scale(frame.size.width, frame.size.height, 1);
	graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
	if (hover_animation)
		menu->square_vao[2].bid = hover_buffer;
	else
		menu->square_vao[2].bid = menu->color_vbo[highlighted];
	graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, menu->square_vao, 3);
	
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
		menu->square_vao[2].bid = menu->color_vbo[MENU_COLOR_BORDER];
		graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, menu->square_vao, 3);
		return;
	}
	
	if (image) {
		// Draw image
		matrix = NSMatrix::Identity();
		matrix.Translate(image_frame.origin.x, image_frame.origin.y, 0);
		matrix.Scale(image_frame.size.width, image_frame.size.height, 1);
		graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, img_buffer);
		graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
		if (chromatic)
			menu->square_vao[2].bid = menu->color_vbo[MENU_COLOR_WHITE];
		else
			menu->square_vao[2].bid = menu->color_vbo[enabled ? (MENU_COLOR_BLACK + highlighted) : MENU_COLOR_BORDER];
		graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, menu->square_vao, 3);
	}
	if (title.length() != 0) {
		// Draw text
		matrix = NSMatrix::Identity();
		matrix.Translate(text_frame.origin.x, text_frame.origin.y, 0);
		matrix.Scale(text_frame.size.width, text_frame.size.height, 1);
		graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, text_buffer);
		graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
		menu->square_vao[2].bid = menu->color_vbo[enabled ? (MENU_COLOR_BLACK + highlighted) : MENU_COLOR_BORDER];
		graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, menu->square_vao, 3);
	}
	
	graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
	if (submenu && menu->is_context_menu) {
		// Draw triangle
		matrix = NSMatrix::Identity();
		matrix.Translate(frame.origin.x + frame.size.width - MENU_OFFSET_X - font_height,
						 frame.origin.y + (frame.size.height - font_height) / 2, 0);
		matrix.Scale(font_height, font_height, 1);
		graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
		menu->triangle_vao[1].bid = menu->color_vbo[enabled ? (MENU_COLOR_BLACK + highlighted) : MENU_COLOR_BORDER];
		graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLELIST, 1, menu->triangle_vao, 2);
	} else if (!submenu && menu->is_context_menu && key_equivalent.length() != 0) {
		// Draw key equivalent
		matrix = NSMatrix::Identity();
		matrix.Translate(frame.origin.x + frame.size.width - MENU_OFFSET_X - key_size.width,
						 frame.origin.y + (frame.size.height - font_height) / 2, 0);
		matrix.Scale(key_size.width, key_size.height, 1);
		graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, key_buffer);
		graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
		menu->square_vao[2].bid = menu->color_vbo[enabled ? (MENU_COLOR_BLACK + highlighted) : MENU_COLOR_BORDER];
		graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, menu->square_vao, 3);
		
		graphics_texturestate_seti(context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
	}
	
	// Draw state
	if (menu->is_context_menu && state == NSMenuItemStateOn) {
		NSRect check_rect = NSRect(NSPoint(frame.origin.x + MENU_OFFSET_X / 2,
								   frame.origin.y + (frame.size.height - CHECK_SIZE.height) / 2),
								   CHECK_SIZE);
		matrix = NSMatrix::Identity();
		matrix.Translate(check_rect.origin.x, check_rect.origin.y, 0);
		matrix.Scale(check_rect.size.width, check_rect.size.height, 1);
		graphics_transform_set(context, GRAPHICS_TRANSFORM_VIEW, matrix);
		menu->square_vao[0].bid = menu->check_vbo;
		menu->square_vao[2].bid = menu->color_vbo[enabled ? (MENU_COLOR_BLACK + highlighted) : MENU_COLOR_BORDER];
		graphics_draw(context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, NSVIEW_CHECKMARK_VERTICES - 2, menu->square_vao, 3);
		menu->square_vao[0].bid = menu->square_vbo;
	}
}
