//
//  NSFont.cpp
//  product
//
//  Created by Neil Singh on 1/30/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSFont.h"
#include "NSImage.h"

#include <algorithm>
#include <codecvt>
#include <locale>

#include <ft2build.h>
#include FT_FREETYPE_H

using std::string;

namespace {
	bool ft_init = false;
	FT_Library ft_library;
	const char* font_path = "/system/fonts/";
	const char* font_extension = ".ttc";
	const char* default_font = "/system/fonts/Helvetica.ttc";
	const int default_size = 14;
};

bool NSFont::SetupDefault() {
	FT_Init_FreeType(&ft_library);
	
	ft_init = true;
	
	return true;
}

NSFont::NSFont() {
	if (!ft_init)
		SetupDefault();
	
	int error = FT_New_Face(ft_library, default_font, 0, &face);
	if (error) {
		face = NULL;
		return;
	}
	
	FT_Set_Char_Size(face, 0, default_size*64, 0, 0);
}

NSFont::~NSFont() {
	if (face)
		FT_Done_Face(face);
}

NSFont::NSFont(float size) {
	if (!ft_init)
		SetupDefault();
	
	int error = FT_New_Face(ft_library, default_font, 0, &face);
	if (error) {
		face = NULL;
		return;
	}
	
	FT_Set_Char_Size(face, 0, size*64, 0, 0);
}

NSFont::NSFont(const string& name, float size) {
	if (!ft_init)
		SetupDefault();
	
	string str = font_path + name + font_extension;
	int error = FT_New_Face(ft_library, default_font, 0, &face);
	if (error) {
		face = NULL;
		return;
	}
	
	FT_Set_Char_Size(face, 0, default_size*64, 0, 0);
}

float NSFont::GetLineHeight() const {
	return face->size->metrics.height / 64.0 - 4;		// Add pixel boundary
}

// Clamp a rectangle to (0, 0, realWidth, realHeight)
// Returns false if result is an empty rect
bool NSRectClampi(int& x, int& y, int& width, int& height, int realWidth, int realHeight) {
	if (x > realWidth || y > realHeight || x + width < 0 || y + height < 0)
		return false;
	if (x < 0) {
		width += x;
		x = 0;
	}
	if (y < 0) {
		height += y;
		y = 0;
	}
	if (x + width >= realWidth)
		width = realWidth - x;
	if (y + height >= realHeight)
		height = realHeight - y;
	return true;
}

NSImage* NSFont::GetImage(const string& str, NSColor<uint8_t> color) {
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;
	std::u32string utf32 = cvt.from_bytes(str);
	
	int width = 0;
	int height = 0;
	int ymin = INT_MAX;
	int ymax = INT_MIN;
	int baseline = 0;
	
	for (unsigned int z = 0; z < utf32.size(); z++) {
		// Load
		FT_Load_Char(face, utf32[z], FT_LOAD_RENDER);
		if (!face->glyph)
			return NULL;
		width += face->glyph->advance.x >> 6;
		//uint32_t diff_h = face->glyph->advance.y >> 6;
		//height += diff_h;
		if (ymin > -face->glyph->bitmap_top)
			ymin = -face->glyph->bitmap_top;
		if (ymax < int(face->glyph->bitmap.rows - face->glyph->bitmap_top))
			ymax = face->glyph->bitmap.rows - face->glyph->bitmap_top;
	}
	height = ymax - ymin + 2;
	baseline = -ymin;
	
	if (width == 0 || height == 0)
		return NULL;
	
	NSImage* image = new NSImage(NSSize(width, height));
	uint32_t* buffer = image->GetPixelData();
	
	int x = 0;
	int y = 0;

	float calpha = color.a / 255.0f;
	for (unsigned int z = 0; z < utf32.size(); z++) {
		// Load
		FT_Load_Char(face, utf32[z], FT_LOAD_RENDER);
		if (!face->glyph)
			break;
		FT_Bitmap bitmap = face->glyph->bitmap;
		
		// Draw bitmap
		int realY = y + baseline - face->glyph->bitmap_top;
		int realX = x + face->glyph->bitmap_left;
		int bWidth = bitmap.width;
		int bHeight = bitmap.rows;
		uint32_t startY = 0;
		uint32_t startX = 0;
		if (realY < 0) {
			startY = -realY;
			realY = 0;
		}
		if (realX < 0) {
			startX = -realX;
			realX = 0;
		}
		if (!NSRectClampi(realX, realY, bWidth, bHeight, width, height))
			continue;
		for (int by = startY; by < bHeight; by++) {
			for (int bx = startX; bx < bWidth; bx++) {
				color.a = bitmap.buffer[by * bitmap.pitch + bx] * calpha;
				buffer[(realY + by) * width + realX + bx] = color.RGBAValue();
			}
		}
		
		// Advance
		x += face->glyph->advance.x >> 6;
		y += face->glyph->advance.y >> 6;
	}
	
	return image;
}
