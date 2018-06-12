//
//  NSImage.h
//  product
//
//  Created by Neil Singh on 1/29/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSImage_h
#define NSImage_h

#include <string>
#include "NSColor.h"
#include "NSTypes.h"

class NSImage {
public:
	typedef enum {
		NSImageTypeBMP,
		NSImageTypePNG,
		NSImageTypeJPG,
		NSImageTypeTIFF,
		NSImageTypeGIF,
	} NSImageType;
	
	NSImage();
	~NSImage();
	NSImage(const NSSize& size);
	// Supports (24, 32) bit RGB and (1, 8) bit grayscale
	NSImage(const void* pixels, NSSize size, int bpp);
	NSImage(const std::string& filename);
	NSImage(const void* data, unsigned int length);
	
	// Accounts for pixel scaling
	NSSize GetScaledSize() const;
	NSSize GetSize() const;
	void SetScaledSize(const NSSize& size);
	void SetSize(const NSSize& size);		// Will linearly resample
	
	void SetPixel(int x, int y, NSColor<uint8_t> color);
	NSColor<uint8_t> GetPixel(int x, int y) const;
	uint32_t* GetPixelData() const;
	
	NSImageType GetType() const;
	uint8_t* RepresentationUsingType(NSImageType type, unsigned int* data_length) const;
	
private:
	bool InitData(const void* data, unsigned int length);
	bool InitBMP(const void* data, unsigned int length);
	bool InitPNG(const void* data, unsigned int length);
	bool InitJPG(const void* data, unsigned int length);
	bool InitGIF(const void* data, unsigned int length);
	bool InitTIFF(const void* data, unsigned int length);
	uint8_t* CreateBMP(unsigned int* length_out) const;
	uint8_t* CreatePNG(unsigned int* length_out) const;
	uint8_t* CreateJPG(unsigned int* length_out) const;
	uint8_t* CreateGIF(unsigned int* length_out) const;
	uint8_t* CreateTIFF(unsigned int* length_out) const;
	
	// 32-bit BGRA
	uint32_t* pixels = NULL;
	NSSize size;
	NSImageType type;
	
	float psf;
};

#endif /* NSImage_h */
