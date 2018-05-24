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
struct FT_FaceRec_;

class NSFont {
public:
	NSFont();
	~NSFont();
	NSFont(float size);
	NSFont(const std::string& name, float size);
	
	float GetLineHeight() const;
	
	// UTF8-String
	NSImage* GetImage(const std::string& string, NSColor<uint8_t> color);
private:
	bool SetupDefault();
	
	FT_FaceRec_* face;
};

#endif /* NSFONT_H */
