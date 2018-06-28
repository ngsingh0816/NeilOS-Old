//
//  NSMenuItem.h
//  product
//
//  Created by Neil Singh on 5/23/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSMENUITEM_H
#define NSMENUITEM_H

#include "../Core/NSFont.h"
#include "../Core/NSImage.h"
#include "../Core/NSResponder.h"
#include "../Core/NSTypes.h"
#include "NSAnimation.h"
#include "NSMenu.h"

#include <functional>
#include <string>

#define NSMenuItemStateOff			0
#define NSMenuItemStateOn			1
typedef bool NSMenuItemState;

class NSMenuItem {
public:
	static NSMenuItem* FromData(const uint8_t* data, uint32_t length, uint32_t* length_used=NULL);
	virtual uint8_t* Serialize(uint32_t* length_out) const;

	static NSMenuItem* SeparatorItem();
	
	NSMenuItem();
	NSMenuItem(const NSMenuItem& item);
	NSMenuItem(std::string title, std::string key_equivalent="", NSModifierFlags flags=0);
	NSMenuItem(std::string title, const std::function<void(NSMenuItem*)>& action);
	NSMenuItem(NSImage* image, bool chromatic=true, std::string key_equivalent="", NSModifierFlags flags=0);
	NSMenuItem(NSImage* image, const std::function<void(NSMenuItem*)>& action, bool chromatic=true);
	virtual ~NSMenuItem();
	
	virtual NSMenuItem* Clone();
	
	NSMenuItem& operator=(const NSMenuItem& item);
	
	NSMenu* GetSubmenu();
	// Takes ownership
	void SetSubmenu(NSMenu* menu);
	NSMenu* GetSupermenu();
	
	std::string GetTitle() const;
	void SetTitle(std::string title);
	
	NSFont GetFont() const;
	void SetFont(NSFont font);
	
	NSImage* GetImage() const;
	// Creates copy
	void SetImage(const NSImage* image, bool chromatic=true);
	
	bool IsSeparator() const;
	void SetIsSeparator(bool is);
	
	NSSize GetSize() const;
	void SetSize(const NSSize& size);
	
	std::string GetKeyEquivalent() const;
	NSModifierFlags GetKeyModifierFlags() const;
	void SetKeyEquivalent(std::string key, NSModifierFlags flags=0);
	
	const std::function<void(NSMenuItem*)>& GetAction() const;
	void SetAction(const std::function<void(NSMenuItem*)>& action);
	
	bool GetIsEnabled() const;
	void SetIsEnabled(bool enabled);
	
	float GetBorderHeight() const;
	void SetBorderHeight(float height);
	
	NSMenuItemState GetState() const;
	void SetState(NSMenuItemState state);
	
	const std::function<void(NSCursorRegion* region)>& GetCursorEvent() const;
	void SetCursorEvent(const std::function<void(NSCursorRegion* region)>&);
	
	void SetUserData(void* data);
	void* GetUserData() const;
	
	virtual bool MouseDown(NSEventMouse* event);
	virtual bool MouseDragged(NSEventMouse* event);
	virtual bool MouseUp(NSEventMouse* event);
	virtual bool MouseMoved(NSEventMouse* event);
	virtual bool MouseScrolled(NSEventMouse* event);
	virtual bool KeyDown(NSEventKey* event);
	virtual bool KeyUp(NSEventKey* event);
	
protected:
	virtual void SetMenu(NSMenu* menu);
	virtual void Clear();
	virtual void UpdateSize();
	virtual void Draw(graphics_context_t* context, NSPoint point, NSSize size);
	
private:
	friend class NSMenu;
	friend class NSMenuViewItem;
	
	void Init();
	void Hover(bool over);
	void Update(bool all=false);
	
	NSMenu* submenu = NULL;
	NSMenu* menu = NULL;
	
	std::string title = "";
	NSFont font;
	NSImage* image = NULL;
	bool chromatic = true;
	float border_height;
	bool is_separator = false;
	std::string key_equivalent = "";
	NSModifierFlags flags = NSModifierNone;
	NSMenuItemState state = NSMenuItemStateOff;
	bool highlighted = false;
	std::function<void(NSMenuItem*)> action;
	std::function<void(NSCursorRegion*)> cursor_action;
	bool enabled = true;
	NSSize item_size;
	void* user_data = NULL;
	NSAnimation* hover_animation = NULL;
	NSColor<float> current_color;
	
	uint32_t text_buffer = 0;
	uint32_t img_buffer = 0;
	uint32_t key_buffer = 0;
	uint32_t hover_buffer = 0;
	NSSize text_size = NSSize();
	NSSize key_size = NSSize();
	NSSize img_size = NSSize();
	float font_height = 0;
	bool prevents_menu_clear = false;
};

#endif /* NSMENUITEM_H */
