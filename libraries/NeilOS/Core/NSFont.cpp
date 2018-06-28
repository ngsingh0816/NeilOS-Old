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
#include FT_GLYPH_H

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
		uint32_t current_size;
		std::map<std::pair<uint32_t, uint32_t>, FT_Glyph> glyphs;
		std::map<uint32_t, FT_Size_Metrics> metrics;
		uint32_t ref_count;
		pthread_mutex_t lock;
	} FontFace;
	std::map<std::string, FontFace*> faces;
	pthread_mutex_t faces_lock = PTHREAD_MUTEX_INITIALIZER;
	
	inline uint32_t GetPixelSize(float size, float psf) {
		return round(size * 64 * psf);
	}
	
	void DeallocFace(std::string name) {
		pthread_mutex_lock(&faces_lock);
		auto it = faces.find(name);
		if (it != faces.end()) {
			pthread_mutex_lock(&it->second->lock);
			if ((--it->second->ref_count) == 0) {
				// Dealloc
				for (auto& g : it->second->glyphs)
					FT_Done_Glyph((FT_Glyph)g.second);
				FT_Done_Face(it->second->face);
				delete it->second;
				faces.erase(it);
			} else
				pthread_mutex_unlock(&it->second->lock);
		}
		pthread_mutex_unlock(&faces_lock);
	}
	
	FontFace* GetFace(std::string name) {
		pthread_mutex_lock(&faces_lock);
		auto it = faces.find(name);
		if (it != faces.end()) {
			it->second->ref_count++;
			pthread_mutex_unlock(&faces_lock);
			return it->second;
		}
		
		FT_Face face;
		if (FT_New_Face(ft_library, name.c_str(), 0, &face) != 0) {
			pthread_mutex_unlock(&faces_lock);
			return NULL;
		}
		
		FontFace* font_face = new FontFace;
		font_face->face = face;
		font_face->current_size = 0;
		font_face->ref_count = 1;
		font_face->lock = PTHREAD_MUTEX_INITIALIZER;
		faces[name] = font_face;
		pthread_mutex_unlock(&faces_lock);
		
		return font_face;
	}
	
	void SetSize(FontFace* face, uint32_t pixel_size) {
		if (face->current_size != pixel_size) {
			FT_Set_Char_Size(face->face, 0, pixel_size, 0, 0);
			face->current_size = pixel_size;
		}
	}
	
	FT_BitmapGlyph GetGlyph(FontFace* face, uint32_t glyph, uint32_t size) {
		auto pair = std::pair<uint32_t, uint32_t>(glyph, size);
		pthread_mutex_lock(&face->lock);
		auto it = face->glyphs.find(pair);
		if (it != face->glyphs.end()) {
			pthread_mutex_unlock(&face->lock);
			return (FT_BitmapGlyph)it->second;
		}
		
		FT_Glyph ret;
		SetSize(face, size);
		FT_Load_Char(face->face, glyph, FT_LOAD_DEFAULT);
		FT_Get_Glyph(face->face->glyph, &ret);
		if (ret->format != FT_GLYPH_FORMAT_BITMAP)
			FT_Glyph_To_Bitmap(&ret, FT_RENDER_MODE_NORMAL, NULL, true);
		face->glyphs[pair] = ret;
		pthread_mutex_unlock(&face->lock);

		return (FT_BitmapGlyph)ret;
	}
	
	FT_Size_Metrics GetMetrics(FontFace* face, uint32_t size) {
		pthread_mutex_lock(&face->lock);
		auto it = face->metrics.find(size);
		if (it != face->metrics.end()) {
			pthread_mutex_unlock(&face->lock);
			return it->second;
		}
		
		SetSize(face, size);
		FT_Size_Metrics m = face->face->size->metrics;
		face->metrics[size] = m;
		pthread_mutex_unlock(&face->lock);

		return m;
	}
};

bool NSFont::SetupDefault() {
	FT_Init_FreeType(&ft_library);
	
	ft_init = true;
	
	return true;
}

NSFont* NSFont::FromData(const uint8_t* data, uint32_t length, uint32_t* length_used) {
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
	
	psf = NSApplication::GetPixelScalingFactor();
	face = static_cast<void*>(GetFace(font_name));
}

NSFont::NSFont() {
	font_name = std::string(default_font);
	font_size = default_size;
	
	Init();
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
	
	psf = NSApplication::GetPixelScalingFactor();
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
	FontFace* font_face = static_cast<FontFace*>(face);
	return (GetMetrics(font_face, GetPixelSize(font_size, psf)).ascender / 64.0) / psf;
}

float NSFont::GetFontHeight() const {
	FontFace* font_face = static_cast<FontFace*>(face);
	return (GetMetrics(font_face, GetPixelSize(font_size, psf)).height / 64.0) / psf;
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
	
	FontFace* font_face = static_cast<FontFace*>(face);
	
	uint32_t pixel_size = GetPixelSize(font_size, psf);
	for (unsigned int z = 0; z < utf32.size(); z++) {
		FT_BitmapGlyph glyph = GetGlyph(font_face, utf32[z], pixel_size);
		width += glyph->root.advance.x >> 16;
	}
	FT_Size_Metrics metrics = GetMetrics(font_face, pixel_size);
	int height = round(metrics.height / 64.0);
	int baseline = round(metrics.ascender / 64.0);
	
	if (width == 0 || height == 0)
		return NULL;
	
	NSImage* image = new NSImage(NSSize(width, height));
	uint32_t* buffer = image->GetPixelData();
	
	int x = 0;

	float calpha = color.a / 255.0f;
	for (unsigned int z = 0; z < utf32.size(); z++) {
		// Load
		FT_BitmapGlyph glyph = GetGlyph(font_face, utf32[z], pixel_size);
		FT_Bitmap bitmap = glyph->bitmap;
		
		// Draw bitmap
		int realX = x + glyph->left;
		int realY = baseline - glyph->top;
		int bWidth = bitmap.width;
		int bHeight = bitmap.rows;
		uint32_t startX = 0, startY = 0;
		if (realX < 0) {
			startX = -realX;
			realX = 0;
		}
		if (realY < 0) {
			startY = -realY;
			realY = 0;
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
		x += glyph->root.advance.x >> 16;
	}
	
	return image;
}
