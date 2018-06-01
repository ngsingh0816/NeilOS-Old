//
//  NSMenu.h
//  product
//
//  Created by Neil Singh on 5/21/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSMENU_H
#define NSMENU_H

#include "../Core/NSColor.h"
#include "../Core/NSResponder.h"
#include "../Core/NSTypes.h"

#include <graphics/graphics.h>

#include <algorithm>
#include <vector>

typedef enum {
	NSMenuOrientationDown = 0,
	NSMenuOrientationUp = 1,
} NSMenuOrientation;

class NSMenuItem;

class NSMenu {
public:
	static NSMenu* FromData(uint8_t* data, uint32_t length, uint32_t* length_used=NULL);
	uint8_t* Serialize(uint32_t* length_out);
	
	NSMenu(const NSMenu& menu);
	NSMenu(bool is_context=false);
	NSMenu(const std::vector<NSMenuItem*>& items, bool is_context=false);
	~NSMenu();
	
	NSMenu& operator=(const NSMenu& menu);
	
	bool IsContextMenu() const;
	void SetIsContextMenu(bool c);
	
	NSMenuOrientation GetOrientation() const;
	void SetOrientation(NSMenuOrientation orientation);
	
	std::vector<NSMenuItem*> GetItems();
	// Takes ownership of all items
	void SetItems(const std::vector<NSMenuItem*>& items);
	// Takes ownership
	void AddItem(NSMenuItem* item);
	void AddItem(NSMenuItem* item, unsigned int index);
	void RemoveItem(unsigned int index);
	void RemoveAllItems();
	uint32_t IndexOfItem(NSMenuItem* item);
	
	bool MouseDown(NSEventMouse* event);
	bool MouseUp(NSEventMouse* event);
	bool MouseMoved(NSEventMouse* event);
	bool KeyDown(NSEventKey* event);
	bool KeyUp(NSEventKey* event);
	
	NSSize GetSize();
	NSRect GetFrame() const;
	void SetFrame(NSRect frame);
	
	graphics_context_t* GetContext() const;
	void SetContext(graphics_context_t* context);
	
	void Draw(const std::vector<NSRect>& rects);
	
	void SetUpdateFunction(std::function<void(std::vector<NSRect>)> func);
	
	NSColor<float> GetBackgroundColor() const;
	void SetBackgroundColor(NSColor<float> color);
	NSColor<float> GetHighlightColor() const;
	void SetHighlightColor(NSColor<float> color);
	NSColor<float> GetBorderColor() const;
	void SetBorderColor(NSColor<float> color);
	NSColor<float> GetTextColor() const;
	void SetTextColor(NSColor<float> color);
	NSColor<float> GetHighlightedTextColor() const;
	void SetHighlightedTextColor(NSColor<float> color);
private:
	friend class NSMenuItem;
	
	void SetupVAO();
	NSRect AdjustRect(NSRect rect);
	bool MouseEvent(NSEventMouse* event, bool down);
	void ClearSubmenu();
	void Clear();
	void SetColor(NSColor<float> color, unsigned int index);
	void SetColors(NSColor<float> colors[5]);
	
	void UpdateAll();
	void UpdateItem(unsigned int index);
	void UpdateItems(std::vector<unsigned int> indices);
	
	std::vector<NSMenuItem*> items;
	NSMenu* submenu = NULL;
	bool is_context_menu = false;
	
	NSSize menu_size;
	bool cached_size = false;
	
	NSRect frame;
	NSMenuOrientation orientation = NSMenuOrientationDown;
	bool mouse_down = false;
	bool mouse_captured = false;
	std::function<void(std::vector<NSRect>)> update;
	
	NSColor<float> background_color = NSColor<float>::UILighterGrayColor();
	NSColor<float> highlight_color = NSColor<float>::UIBlueColor();
	NSColor<float> border_color = NSColor<float>::UIGrayColor();
	NSColor<float> text_color = NSColor<float>::BlackColor();
	NSColor<float> text_highlight_color = NSColor<float>::WhiteColor();
	
	graphics_context_t* context = NULL;
	uint32_t square_vbo = 0;
	uint32_t triangle_vbo = 0;
	graphics_vertex_array_t square_vao[3];
	graphics_vertex_array_t triangle_vao[2];
	uint32_t color_vbo[5];
};

#endif /* NSMENU_H */
