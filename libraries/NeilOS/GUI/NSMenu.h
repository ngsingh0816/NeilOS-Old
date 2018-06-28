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
#include "NSWindow.h"

#include <graphics/graphics.h>

#include <algorithm>
#include <vector>

typedef enum {
	NSMenuOrientationDown = 0,
	NSMenuOrientationUp = 1,
} NSMenuOrientation;

class NSMenuItem;

class NSMenu : public NSViewContainer {
public:
	// Screen coordinates
	static void PopupContextMenu(NSMenu* menu, const NSPoint& p);
	static NSMenu* GetCurrentContextMenu();
	
	static NSMenu* FromData(const uint8_t* data, uint32_t length, uint32_t* length_used=NULL);
	uint8_t* Serialize(uint32_t* length_out);
	
	NSMenu(const NSMenu& menu);
	NSMenu(bool is_context=false);
	NSMenu(const std::vector<NSMenuItem*>& items, bool is_context=false);
	virtual ~NSMenu();
	
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
	uint32_t IndexOfItem(NSMenuItem* item) const;
	NSPoint GetOffsetOfItem(NSMenuItem* item);
	
	bool MouseDown(NSEventMouse* event);
	bool MouseUp(NSEventMouse* event);
	bool MouseMoved(NSEventMouse* event);
	bool MouseScrolled(NSEventMouse* event);
	bool KeyDown(NSEventKey* event);
	bool KeyUp(NSEventKey* event);
	
	NSSize GetSize();
	NSRect GetFrame() const;
	NSRect GetFullFrame();
	virtual NSPoint GetOffset() const override;
	void SetFrame(const NSRect& frame);
	
	NSRect GetFrameBounds() const;
	void SetFrameBounds(const NSRect& bounds);
	
	float GetBorderRadius() const;
	void SetBorderRadius(float radius);
	
	virtual graphics_context_t* GetContext() override;
	void SetContext(graphics_context_t* context);
	
	void Draw(const std::vector<NSRect>& rects);
	
	void Clear();
	
	virtual void AddUpdateRects(const std::vector<NSRect>& rects) override;
	void SetUpdateFunction(const std::function<void(std::vector<NSRect>)>& func);
	
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
	friend class NSMenuViewItem;
	
	void SetupVAO();
	NSRect AdjustRect(const NSRect& rect);
	bool MouseEvent(NSEventMouse* event, bool down);
	void ClearSubmenu();
	void SetColor(NSColor<float> color, unsigned int index);
	void SetColors(NSColor<float> colors[5]);
	unsigned int GetVerticesForColor(unsigned int index);
	
	void UpdateAll();
	void UpdateItem(unsigned int index);
	void UpdateItems(std::vector<unsigned int> indices);
	
	std::vector<NSMenuItem*> items;
	NSMenu* submenu = NULL;
	bool is_context_menu = false;
	
	NSSize menu_size;
	bool cached_size = false;
	
	NSRect frame;
	NSRect frame_bounds;
	NSMenuOrientation orientation = NSMenuOrientationDown;
	bool mouse_down = false;
	bool mouse_captured = false;
	bool mouse_view = false;
	float border_radius = 5;
	unsigned int round_edge = 0;
	std::function<void(std::vector<NSRect>)> update;
	
	NSColor<float> background_color = NSColor<float>::UILighterGrayColor();
	NSColor<float> highlight_color = NSColor<float>::UILightGrayColor();
	NSColor<float> border_color = NSColor<float>::UIGrayColor();
	NSColor<float> text_color = NSColor<float>::BlackColor();
	NSColor<float> text_highlight_color = NSColor<float>::BlackColor();
	
	graphics_context_t* context = NULL;
	uint32_t square_vbo = 0;
	uint32_t triangle_vbo = 0;
	uint32_t check_vbo = 0;
	uint32_t rounded_vbo = 0;
	graphics_vertex_array_t square_vao[3];
	graphics_vertex_array_t triangle_vao[2];
	graphics_vertex_array_t rounded_vao[2];
	uint32_t color_vbo[5];
};

#endif /* NSMENU_H */
