//
//  NSMenu.h
//  product
//
//  Created by Neil Singh on 5/21/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSMENU_H
#define NSMENU_H

#include "../Core/NSResponder.h"
#include "../Core/NSTypes.h"

#include <graphics/graphics.h>

#include <vector>

class NSMenuItem;

class NSMenu {
public:
	NSMenu(bool is_context=false);
	NSMenu(const std::vector<NSMenuItem*>& items, bool is_context=false);
	~NSMenu();
	
	bool IsContextMenu() const;
	void SetIsContextMenu(bool c);
	
	std::vector<NSMenuItem*> GetItems();
	// Takes ownership of all items
	void SetItems(const std::vector<NSMenuItem*>& items);
	// Takes ownership
	void AddItem(NSMenuItem* item);
	void AddItem(NSMenuItem* item, unsigned int index);
	void RemoveItem(unsigned int index);
	void RemoveAllItems();
	
	bool MouseDown(NSEventMouse* event);
	bool MouseUp(NSEventMouse* event);
	bool MouseMoved(NSEventMouse* event);
	bool KeyDown(NSEventKey* event);
	bool KeyUp(NSEventKey* event);
	
	NSSize GetSize() const;
	NSRect GetFrame() const;
	void SetFrame(NSRect frame);
	
	graphics_context_t* GetContext() const;
	void SetContext(graphics_context_t* context);
	
	void DrawUpdates(const std::vector<NSRect>& rects);
	bool NeedsUpdate();
	std::vector<NSRect> GetClearedRects();
	std::vector<NSRect> GetUpdatedRects();
private:
	friend class NSMenuItem;
	
	void SetupVAO();
	NSRect AdjustRect(NSRect rect);
	bool MouseEvent(NSEventMouse* event, bool down);
	void ClearSubmenu();
	void Clear();
	
	std::vector<NSMenuItem*> items;
	NSMenu* submenu = NULL;
	bool is_context_menu = false;
	
	NSRect frame;
	bool mouse_down = false;
	bool mouse_captured = false;
	std::vector<uint32_t> updates;
	std::vector<NSRect> cleared_rects;
	std::vector<NSRect> updated_rects;
	
	graphics_context_t* context = NULL;
	uint32_t square_vbo = 0;
	uint32_t triangle_vbo = 0;
	graphics_vertex_array_t square_vao[3];
	graphics_vertex_array_t triangle_vao[2];
	uint32_t color_vbo[5];
};

#endif /* NSMENU_H */
