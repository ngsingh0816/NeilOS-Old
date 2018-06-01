//
//  NSFont.cpp
//  product
//
//  Created by Neil Singh on 1/30/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSFont.h"

#include "NSApplication.h"
#include "NSImage.h"

#include <algorithm>
#include <codecvt>
#include <locale>
#include <map>

#include <math.h>
#include <pthread.h>

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
	
	typedef struct {
		FT_Face face;
		float size;
		uint32_t ref_count;
		pthread_mutex_t lock;
	} FontFace;
	std::map<std::string, FontFace> faces;
	pthread_mutex_t faces_lock = PTHREAD_MUTEX_INITIALIZER;
};

bool NSFont::SetupDefault() {
	FT_Init_FreeType(&ft_library);
	
	ft_init = true;
	
	return true;
}

NSFont* NSFont::FromData(uint8_t* data, uint32_t length, uint32_t* length_used) {
#define copy(x, len) { memcpy(x, &data[pos], (len)); pos += (len); }
	if (length < sizeof(float) + sizeof(uint32_t))
		return NULL;
	
	uint32_t pos = 0;
	float size;
	copy(&size, sizeof(float));
	uint32_t len;
	copy(&len, sizeof(uint32_t));
	char buf[len + 1];
	copy(buf, len);
	buf[len] = 0;
	
	if (length_used)
		*length_used = pos;
	return new NSFont(std::string(buf), size);
#undef copy
}

uint8_t* NSFont::Serialize(uint32_t* length_out) const {
#define copy(x, len) { memcpy(&buffer[pos], x, (len)); pos += (len); }
	std::string f_name = font_name;
	if (f_name == std::string(default_font))
		f_name = std::string("");
	
	uint32_t total_length = 0;
	total_length += sizeof(float);		// size
	total_length += sizeof(uint32_t);	// font name length
	total_length += f_name.length();
	uint8_t* buffer = new uint8_t[total_length];
	
	uint32_t pos = 0;
	copy(&font_size, sizeof(float));
	uint32_t len = f_name.length();
	copy(&len, sizeof(uint32_t));
	copy(f_name.c_str(), len);
	
	if (length_out)
		*length_out = total_length;
	return buffer;
#undef copy
}

void NSFont::Init() {
	if (!ft_init)
		SetupDefault();
	
	pthread_mutex_lock(&faces_lock);
	
	auto it = faces.find(font_name);
	if (it != faces.end()) {
		it->second.ref_count++;
	} else {
		FT_Face face = NULL;
		int error = FT_New_Face(ft_library, font_name.c_str(), 0, &face);
		if (error) {
			pthread_mutex_unlock(&faces_lock);
			return;
		}
		
		FT_Set_Char_Size(face, 0, font_size * 64 * NSApplication::GetPixelScalingFactor(), 0, 0);
		
		FontFace font_face;
		font_face.face = face;
		font_face.size = font_size * NSApplication::GetPixelScalingFactor();
		font_face.ref_count = 1;
		font_face.lock = PTHREAD_MUTEX_INITIALIZER;
		faces[font_name] = font_face;
	}
	pthread_mutex_unlock(&faces_lock);
}

NSFont::NSFont() {
	font_name = std::string(default_font);
	font_size = default_size;
	
	Init();
}

void DeallocFace(std::string name) {
	pthread_mutex_lock(&faces_lock);
	auto it = faces.find(name);
	if (it != faces.end()) {
		pthread_mutex_lock(&it->second.lock);
		if ((--it->second.ref_count) == 0) {
			FT_Done_Face(it->second.face);
			faces.erase(it);
		} else
			pthread_mutex_unlock(&it->second.lock);
	}
	pthread_mutex_unlock(&faces_lock);
}

FontFace* GetFace(std::string name, float size) {
	pthread_mutex_lock(&faces_lock);
	FontFace* font_face = &faces[name];
	pthread_mutex_lock(&font_face->lock);
	pthread_mutex_unlock(&faces_lock);
	if (fabs(font_face->size - size * NSApplication::GetPixelScalingFactor()) > 0.01) {
		FT_Set_Char_Size(font_face->face, 0, size * 64 * NSApplication::GetPixelScalingFactor(), 0, 0);
		font_face->size = size * NSApplication::GetPixelScalingFactor();
	}
	return font_face;
}

NSFont::~NSFont() {
	DeallocFace(font_name);
}

NSFont::NSFont(float size) {
	font_name = std::string(default_font);
	font_size = size;
	
	Init();
}

NSFont::NSFont(const string& name, float size) {
	font_name = font_path + name + font_extension;
	if (name.length() == 0)
		font_name = std::string(default_font);
	font_size = size;
	
	Init();
}

NSFont::NSFont(const NSFont& font) {
	*this = font;
}

NSFont& NSFont::operator=(const NSFont& font) {
	if (this == &font)
		return *this;
	
	font_size = font.font_size;
	if (font.font_name == font_name) {
		return *this;
	}
	
	DeallocFace(font_name);
	font_name = font.font_name;
	
	Init();
	
	return *this;
}

float NSFont::GetLineHeight() const {
	FontFace* font_face = GetFace(font_name, font_size);
	
	// Add pixel boundary
	float ret = (font_face->face->size->metrics.height / 64.0) / NSApplication::GetPixelScalingFactor() - 4;
	
	pthread_mutex_unlock(&font_face->lock);
	return ret;
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
	
	FontFace* font_face = GetFace(font_name, font_size);
	FT_Face face = font_face->face;
	for (unsigned int z = 0; z < utf32.size(); z++) {
		// Load
		FT_Load_Char(face, utf32[z], FT_LOAD_RENDER);
		if (!face->glyph) {
			pthread_mutex_unlock(&font_face->lock);
			return NULL;
		}
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
	
	if (width == 0 || height == 0) {
		pthread_mutex_unlock(&font_face->lock);
		return NULL;
	}
	
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
	
	pthread_mutex_unlock(&font_face->lock);

	return image;
}
