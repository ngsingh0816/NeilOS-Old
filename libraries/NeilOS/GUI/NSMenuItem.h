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
#include "../Core/NSTypes.h"
#include "NSMenu.h"

#include <functional>
#include <string>

class NSMenuItem {
public:
	static NSMenuItem* FromData(uint8_t* data, uint32_t length, uint32_t* length_used=NULL);
	uint8_t* Serialize(uint32_t* length_out);

	static NSMenuItem* SeparatorItem();
	
	NSMenuItem();
	NSMenuItem(const NSMenuItem& item);
	NSMenuItem(std::string title, std::string key_equivalent="", NSModifierFlags flags=0);
	NSMenuItem(NSImage* image, bool chromatic=true, std::string key_equivalent="", NSModifierFlags flags=0);
	~NSMenuItem();
	
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
	void SetSize(NSSize size);
	
	std::string GetKeyEquivalent() const;
	NSModifierFlags GetKeyModifierFlags() const;
	void SetKeyEquivalent(std::string key, NSModifierFlags flags=0);
	
	std::function<void(NSMenuItem*)> GetAction() const;
	void SetAction(std::function<void(NSMenuItem*)> action);
	
	bool GetIsEnabled() const;
	void SetIsEnabled(bool enabled);
	
	float GetBorderHeight() const;
	void SetBorderHeight(float height);
	
	void SetUserData(void* data);
	void* GetUserData() const;
private:
	friend class NSMenu;
	
	void Draw(graphics_context_t* context, NSPoint point, NSSize size);
	void Update(bool all=false);
	void UpdateSize();
	
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
	bool highlighted = false;
	std::function<void(NSMenuItem*)> action;
	bool enabled = true;
	NSSize item_size;
	void* user_data = NULL;
	
	uint32_t text_buffer = 0;
	uint32_t img_buffer = 0;
	uint32_t key_buffer = 0;
	NSSize text_size = NSSize();
	NSSize key_size = NSSize();
	NSSize img_size = NSSize();
	float font_height = 0;
};

#endif /* NSMENUITEM_H */
