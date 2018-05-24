//
//  NSImage.cpp
//  product
//
//  Created by Neil Singh on 1/29/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSImage.h"
#include "NSLog.h"

#include <jpeglib.h>
#include <png.h>
#include <string.h>

NSImage::NSImage() {
	pixels = NULL;
	size = NSSize(0, 0);
}

NSImage::~NSImage() {
	if (pixels)
		delete[] pixels;
}

NSImage::NSImage(NSSize _size) {
	size = _size;
	uint32_t width = int(size.width + 0.5f);
	uint32_t height = int(size.height + 0.5f);
	pixels = new uint32_t[width * height];
	memset(pixels, 0, width * height * sizeof(uint32_t));
}

// Supports (24, 32) bit RGB and (1, 8) bit grayscale
NSImage::NSImage(const void* _pixels, NSSize _size, int bpp) {
	size = _size;
	uint32_t width = int(size.width + 0.5f);
	uint32_t height = int(size.height + 0.5f);
	pixels = new uint32_t[width * height];
	if (bpp == 32) {
		memcpy(pixels, _pixels, width * height * sizeof(uint32_t));
	} else if (bpp == 24) {
		const uint8_t* p = reinterpret_cast<const uint8_t*>(_pixels);
		for (uint32_t z = 0; z < width * height; z++) {
			uint32_t word = 0xFF000000 | p[z * 3] | (p[z * 3 + 1] << 8) | (p[z * 3 + 2] << 16);
			pixels[z] = word;
		}
	} else if (bpp == 8) {
		const uint8_t* p = reinterpret_cast<const uint8_t*>(_pixels);
		for (uint32_t z = 0; z < width * height; z++) {
			uint32_t word = 0xFF000000 | p[z] | (p[z] << 8) | (p[z] << 16);
			pixels[z] = word;
		}
	} else if (bpp == 1) {
		const uint8_t* p = reinterpret_cast<const uint8_t*>(_pixels);
		for (uint32_t z = 0; z < width * height; z++) {
			uint8_t b = ((p[z / 8] >> (z % 8)) & 0x1) ? 255 : 0;
			uint32_t word = 0xFF000000 | b | (b << 8) | (b << 16);
			pixels[z] = word;
		}
	} else {
		delete[] pixels;
		pixels = NULL;
		NSLog("Invalid bpp.");
	}
}

NSImage::NSImage(const std::string& filename) {
	pixels = NULL;
	size = NSSize(0, 0);
	
	FILE* file = fopen(filename.c_str(), "r");
	if (!file) {
		NSLog("File could not be opened.");
		return;
	}
	
	fseek(file, 0, SEEK_END);
	unsigned int size = ftell(file);
	rewind(file);
	
	uint8_t* data = new uint8_t[size];
	fread(data, size, 1, file);
	fclose(file);
	
	InitData(data, size);
	delete[] data;
}

NSImage::NSImage(const void* data, unsigned int length) {
	pixels = NULL;
	size = NSSize(0, 0);
	
	InitData(data, length);
}

// Helpers to create images
bool NSImage::InitBMP(const void* data, unsigned int length) {
#define BMP_RGB		0
#define byte(offset)	(*(uint8_t*)((uint32_t)data + (offset)))
#define hword(offset)	(*(uint16_t*)((uint32_t)data + (offset)))
#define word(offset)	(*(uint32_t*)((uint32_t)data + (offset)))
	
	uint32_t pixel_offset = word(0x0A);
	int width = abs(int32_t(word(0x12)));
	int height = int32_t(word(0x16));
	bool inverted = true;
	if (height < 0) {
		height = -height;
		inverted = false;
	}
	uint16_t bpp = hword(0x1C);
	if (bpp != 24 && bpp != 32)
		return false;
	uint32_t pixType = word(0x1E);
	if (pixType != BMP_RGB)
		return false;
	uint32_t row_size = ((bpp * width + 31) / 32) * 4;
	
	// Loop through each row
	size.width = width;
	size.height = height;
	if (pixels)
		delete[] pixels;
	pixels = new uint32_t[width * height];
	for (int y = 0; y < height; y++) {
		uint8_t* pix = &byte(pixel_offset + row_size * y);
		uint32_t realy = inverted ? (height - 1 - y) : y;
		// Read through each pixel
		for (int x = 0; x < width; x++) {
			uint8_t b = *(pix++);
			uint8_t g = *(pix++);
			uint8_t r = *(pix++);
			uint8_t a = 0;
			if (bpp == 32)
				a = *(pix++);
			pixels[realy * width + x] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	}
	
#undef byte
#undef hword
#undef word
	
	type = NSImageTypeBMP;
	return true;
}

bool NSImage::InitPNG(const void* data, unsigned int length) {
	// Per http://pulsarengine.com/2009/01/reading-png-images-from-memory/
	if (!png_check_sig(reinterpret_cast<const uint8_t*>(data), 8))
		return false;
	
	png_structp png_ptr = NULL;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return false;
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return false;
	}
	
	FILE* file = fmemopen(const_cast<void*>(data), length, "rb");
	fseek(file, 8, SEEK_SET);
	png_init_io(png_ptr, file);
	png_set_sig_bytes(png_ptr, 8);
	
	png_read_info(png_ptr, info_ptr);
	uint32_t width = 0, height = 0;
	int bitDepth = 0;	// bits per channel
	int colorType = -1;
	uint32_t ret = png_get_IHDR(png_ptr, info_ptr, &width, &height, &bitDepth, &colorType, NULL, NULL, NULL);
	if (ret != 1) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return false;
	}
	
	if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_RGBA) {
		const uint32_t bytesPerRow = png_get_rowbytes(png_ptr, info_ptr);
		uint8_t* rowData = new uint8_t[bytesPerRow];
		if (pixels)
			delete[] pixels;
		pixels = new uint32_t[width * height];
		size = NSSize(width, height);
		for (uint32_t row = 0; row < height; row++) {
			png_read_row(png_ptr, rowData, NULL);
			uint32_t pos = 0;
			for (uint32_t col = 0; col < width; col++) {
				const uint8_t red = rowData[pos++];
				const uint8_t green = rowData[pos++];
				const uint8_t blue = rowData[pos++];
				uint8_t alpha = 255;
				if (colorType == PNG_COLOR_TYPE_RGBA)
					alpha = rowData[pos++];
				SetPixel(col, row, NSColor<uint8_t>(red, green, blue, alpha));
			}
		}
		delete[] rowData;
	}  else {
		NSLog("Unsupported PNG sample format.");
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return false;
	}
	
	type = NSImageTypePNG;
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(file);
	
	return true;
}

bool NSImage::InitJPG(const void* data, unsigned int length) {
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr_mgr;
	cinfo.err = jpeg_std_error(&jerr_mgr);
	jerr_mgr.error_exit = [](j_common_ptr cinfo){throw cinfo->err;};
	
	uint8_t* buffer = NULL;
	FILE* src = NULL;
	try {
		jpeg_create_decompress(&cinfo);
		src = fmemopen(const_cast<void*>(data), length, "rb");
		jpeg_stdio_src(&cinfo, src);
		int ret = jpeg_read_header(&cinfo, true);
		if (ret != 1)
			jpeg_destroy_decompress(&cinfo);
		jpeg_start_decompress(&cinfo);
		
		int width = cinfo.output_width;
		int height = cinfo.output_height;
		int components = cinfo.output_components;
		if (components != 3) {
			jpeg_finish_decompress(&cinfo);
			jpeg_destroy_decompress(&cinfo);
			fclose(src);
			return false;
		}
		
		size.width = width;
		size.height = height;
		if (pixels)
			delete[] pixels;
		pixels = new uint32_t[width * height];
		
		buffer = new uint8_t[width * components];
		int y = 0;
		while (cinfo.output_scanline < cinfo.output_height) {
			jpeg_read_scanlines(&cinfo, &buffer, 1);
			for (int x = 0; x < components * width; x += components) {
				uint32_t word = (buffer[x] << 16) | (buffer[x+1] << 8) | (buffer[x+2]);
				pixels[y * width + (x / components)] = 0xFF000000 | word;
			}
			y++;
		}
		delete[] buffer;
		buffer = NULL;
		
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		fclose(src);
		src = NULL;
	} catch (struct jpeg_error_mgr* err) {
		if (buffer)
			delete[] buffer;
		if (pixels)
			delete[] pixels;
		jpeg_destroy_decompress(&cinfo);
		if (src)
			fclose(src);
		return false;
	}
	
	return true;
}

bool NSImage::InitGIF(const void* data, unsigned int length) {
	// TODO
	return false;
}

bool NSImage::InitTIFF(const void* data, unsigned int length) {
	// TODO
	return false;
}

bool NSImage::InitData(const void* data, unsigned int length) {
	// Figure out the type of file
	const uint8_t* d = reinterpret_cast<const uint8_t*>(data);
	if (length > 1 && d[0] == 'B' && d[1] == 'M') {
		// BMP
		if (!InitBMP(data, length)) {
			NSLog("Invalid BMP file.");
			return false;
		}
	} else if (length > 3 && d[1] == 'P' && d[2] == 'N' && d[3] == 'G') {
		// PNG
		if (!InitPNG(data, length)) {
			NSLog("Invalid PNG file.");
			return false;
		}
	} else if (length > 2 && d[0] == 0xFF && d[1] == 0xD8) {
		// JPG
		if (!InitJPG(data, length)) {
			NSLog("Invalid JPG file.");
			return false;
		}
	} else if (length > 2 && d[0] == 'G' && d[1] == 'I' && d[2] == 'F') {
		// GIF
		if (!InitGIF(data, length)) {
			NSLog("Invalid GIF file.");
			return false;
		}
	} else if (length > 1 && ((d[0] == 'M' && d[1] == 'M') || (d[0] == 'I' && d[1] == 'I'))) {
		// TIFF
		if (!InitTIFF(data, length)) {
			NSLog("Invalid TIFF file.");
			return false;
		}
	} else {
		// Unsupported
		NSLog("Unsupported file format.");
		return false;
	}
	return true;
}

NSSize NSImage::GetSize() const {
	return size;
}

// Will linearly resample
void NSImage::SetSize(NSSize _size) {
	float width_ratio = size.width / _size.width;
	float height_ratio = size.height / _size.height;
	
	int width = int(size.width + 0.5f);
	int height = int(size.height + 0.5f);
	int new_width = int(_size.width + 0.5f);
	int new_height = int(_size.height + 0.5f);
	
	uint32_t* buffer = new uint32_t[new_width * new_height];
	for (int y = 0; y < new_height; y++) {
		for (int x = 0; x < new_width; x++) {
			float xpos = width_ratio * x;
			float ypos = height_ratio * y;
			int yposi = int(ypos);
			int xposi = int(xpos);
			
			float rx = xpos - xposi;
			float lx = 1.0f - rx;
			float by = ypos - yposi;
			float ty = 1.0f - by;
			
			NSColor<float> ul = pixels[(yposi) * width + xposi];
			NSColor<float> ur = (xposi+1 < width) ? pixels[(yposi) * width + xposi+1] : ul;
			NSColor<float> bl = (yposi+1 < height) ? pixels[(yposi+1) * width + xposi] : ul;
			NSColor<float> br = (xposi+1 < width && yposi+1 < height) ?
									pixels[(yposi+1) * width + xposi+1] : ul;
			buffer[y * new_width + x] = (ul * lx * ty + ur * rx * ty + bl * lx * by + br * rx * by).RGBAValue();
		}
	}
	
	delete[] pixels;
	pixels = buffer;
	size = _size;
}

void NSImage::SetPixel(int x, int y, NSColor<uint8_t> color) {
	uint32_t word = color.b | (color.g << 8) | (color.r << 16) | (color.a << 24);
	uint32_t width = int(size.width + 0.5f);
	pixels[(y * width) + x] = word;
}

NSColor<uint8_t> NSImage::GetPixel(int x, int y) const {
	uint32_t width = int(size.width + 0.5f);
	uint32_t word = pixels[(y * width) + x];
	return NSColor<uint8_t>((word >> 16) & 0xFF, (word >> 8) & 0xFF, word & 0xFF, (word >> 24) & 0xFF);
}

uint32_t* NSImage::GetPixelData() const {
	return pixels;
}

NSImage::NSImageType NSImage::GetType() const {
	return type;
}

uint8_t* NSImage::CreateBMP(unsigned int* length_out) const {
	int width = int(size.width + 0.5f);
	int height = int(size.height + 0.5f);
	uint32_t row_size = (32 * width + 31) / 32 * 4;
	uint32_t pixel_size = row_size * height;
	uint32_t pixel_offset = 54;
	uint32_t total_size = pixel_offset + pixel_size;
	uint8_t* data = new uint8_t[total_size];
	*length_out = total_size;
	
#define BMP_RGB		0
#define byte(offset)	(*(uint8_t*)((uint32_t)data + (offset)))
#define hword(offset)	(*(uint16_t*)((uint32_t)data + (offset)))
#define word(offset)	(*(uint32_t*)((uint32_t)data + (offset)))
	
	hword(0) = 0x4D42;		// BM
	word(2) = total_size;
	word(6) = 0;			// reserved
	word(10) = 54;			// offset
	word(14) = 40;			// size of header
	word(18) = width;
	word(22) = height;
	hword(26) = 1;			// num color planes
	hword(28) = 32;		// bpp
	word(30) = 0;			// compression
	word(34) = pixel_size;
	word(38) = 1000;		// horizontal resolution
	word(42) = 1000;		// vertical resolution
	word(46) = 0;			// num colors per pallete
	word(50) = 0;			// num important colors
	
	// Loop through each row
	/*for (int y = 0; y < height; y++) {
		uint8_t* pix = &byte(pixel_offset + row_size * y);
		uint32_t realy = (height - 1 - y);
		// Read through each pixel
		for (int x = 0; x < width; x++) {
			NSColor<uint8_t> color = NSColor<uint8_t>(pixels[realy * width + x]);
			*(pix++) = color.b;
			*(pix++) = color.g;
			*(pix++) = color.r;
			*(pix++) = color.a;
		}
	}*/
	memcpy(&data[pixel_offset], pixels, width * height * sizeof(uint32_t));
	
#undef byte
#undef hword
#undef word
	
	return data;
}

uint8_t* NSImage::CreatePNG(unsigned int* length_out) const {
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop png_info = png_create_info_struct(png_ptr);
	
	char* data = NULL;
	size_t len_out = 0;
	FILE* file = open_memstream(&data, &len_out);
	png_init_io(png_ptr, file);
	
	int width = int(size.width + 0.5f);
	int height = int(size.height + 0.5f);
	png_set_IHDR(png_ptr, png_info, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, png_info);
	
	uint8_t* buffer = new uint8_t[4 * width];
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			NSColor<uint8_t> p(pixels[y * width + x]);
			buffer[x * 4 + 0] = p.r;
			buffer[x * 4 + 1] = p.g;
			buffer[x * 4 + 2] = p.b;
			buffer[x * 4 + 3] = p.a;
		}
		png_write_row(png_ptr, buffer);
	}
	delete[] buffer;
	png_write_end(png_ptr, png_info);
	
	png_free_data(png_ptr, png_info, PNG_FREE_ALL, -1);
	png_destroy_write_struct(&png_ptr, NULL);
	
	fclose(file);
	*length_out = len_out;
	uint8_t* ret = new uint8_t[len_out];
	memcpy(ret, data, len_out);
	free(data);
	
	return ret;
}

uint8_t* NSImage::CreateJPG(unsigned int* length_out) const {
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr_mgr;
	cinfo.err = jpeg_std_error(&jerr_mgr);
	jerr_mgr.error_exit = [](j_common_ptr cinfo){throw cinfo->err;};
	
	uint8_t* buffer = NULL;
	char* data = NULL;
	size_t len_out = 0;
	FILE* dest = NULL;
	try {
		jpeg_create_compress(&cinfo);
		
		dest = open_memstream(&data, &len_out);
		jpeg_stdio_dest(&cinfo, dest);
		
		cinfo.image_width = int(size.width + 0.5f);
		cinfo.image_height = int(size.height + 0.5f);
		cinfo.num_components = 3;
		cinfo.in_color_space = JCS_RGB;
		jpeg_set_defaults(&cinfo);
		jpeg_set_quality(&cinfo, 95, true);
		jpeg_start_compress(&cinfo, true);
		
		buffer = new uint8_t[3 * cinfo.image_width];
		while (cinfo.next_scanline < cinfo.image_height) {
			for (uint32_t x = 0; x < cinfo.image_width; x++) {
				NSColor<uint8_t> p(pixels[cinfo.next_scanline * cinfo.image_width + x]);
				buffer[x * 3 + 0] = p.r;
				buffer[x * 3 + 1] = p.g;
				buffer[x * 3 + 2] = p.b;
			}
			jpeg_write_scanlines(&cinfo, (uint8_t**)buffer, 1);
		}
		delete[] buffer;
		buffer = NULL;
		
		jpeg_finish_compress(&cinfo);
		fclose(dest);
		dest = NULL;
		
		jpeg_destroy_compress(&cinfo);
	} catch (struct jpeg_error_mgr* err) {
		if (data)
			free(data);
		if (buffer)
			delete[] buffer;
		if (pixels)
			delete[] pixels;
		jpeg_destroy_compress(&cinfo);
		if (dest)
			fclose(dest);
		return NULL;
	}
	
	*length_out = len_out;
	uint8_t* ret = new uint8_t[len_out];
	memcpy(ret, data, len_out);
	free(data);
	
	return ret;
}

uint8_t* NSImage::CreateGIF(unsigned int* length_out) const {
	// TODO
	return NULL;
}

uint8_t* NSImage::CreateTIFF(unsigned int* length_out) const {
	// TODO
	return NULL;
}

uint8_t* NSImage::RepresentationUsingType(NSImageType type, unsigned int* data_length) const {
	switch (type) {
		case NSImageTypeBMP:
			return CreateBMP(data_length);
		case NSImageTypePNG:
			return CreatePNG(data_length);
		case NSImageTypeJPG:
			return CreateJPG(data_length);
		case NSImageTypeGIF:
			return CreateGIF(data_length);
		case NSImageTypeTIFF:
			return CreateTIFF(data_length);
		default:
			return NULL;
	}
}
