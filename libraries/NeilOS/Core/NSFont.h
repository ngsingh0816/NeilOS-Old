//
//  NSFont.h
//  product
//
//  Created by Neil Singh on 1/30/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSFONT_H
#define NSFONT_H

#include <string>

#include "NSColor.h"
#include "NSTypes.h"

class NSImage;

class NSFont {
public:
	static NSFont* FromData(const uint8_t* data, uint32_t length, uint32_t* length_used=NULL);
	uint8_t* Serialize(uint32_t* length_out) const;

	NSFont();
	~NSFont();
	NSFont(const NSFont& font);
	NSFont(float size);
	NSFont(const std::string& name, float size);
	NSFont& operator=(const NSFont& font);
	
	float GetLineHeight() const;
	float GetFontHeight() const;
	
	// UTF8-String
	NSImage* GetImage(const std::string& string, NSColor<uint8_t> color);
	
private:
	void Init();
	bool SetupDefault();
	
	void* face = NULL;
	std::string font_name;
	float font_size;
	float psf;
};

#endif /* NSFONT_H */
