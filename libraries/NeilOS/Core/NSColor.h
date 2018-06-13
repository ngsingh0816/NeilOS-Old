//
//  NSColor.h
//  product
//
//  Created by Neil Singh on 1/29/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSCOLOR_H
#define NSCOLOR_H

#include <stdint.h>

// Colors are represented by the template type (only uint8_t and float)
// for uint8_t, values are from 0 to 255
// for float, values are from 0 to 1
template <class T>
class NSColor {
public:
	static NSColor<T> WhiteColor();
	static NSColor<T> BlackColor();
	static NSColor<T> LightGreyColor();
	static NSColor<T> GreyColor();
	static NSColor<T> DarkGreyColor();
	static NSColor<T> TransparentColor();
	static NSColor<T> RedColor();
	static NSColor<T> GreenColor();
	static NSColor<T> YellowColor();
	static NSColor<T> BlueColor();
	static NSColor<T> PurpleColor();
	static NSColor<T> CyanColor();
	static NSColor<T> MagentaColor();
	static NSColor<T> LimeColor();
	static NSColor<T> PinkColor();
	static NSColor<T> TealColor();
	static NSColor<T> LavenderColor();
	static NSColor<T> BrownColor();
	static NSColor<T> BeigeColor();
	static NSColor<T> MaroonColor();
	static NSColor<T> MintColor();
	static NSColor<T> OliveColor();
	static NSColor<T> CoralColor();
	static NSColor<T> NavyColor();
	// UI Colors
	static NSColor<T> UITurquoiseColor();
	static NSColor<T> UIDarkTurquoiseColor();
	static NSColor<T> UIGreenColor();
	static NSColor<T> UIDarkGreenColor();
	static NSColor<T> UIBlueColor();
	static NSColor<T> UIDarkBlueColor();
	static NSColor<T> UIPurpleColor();
	static NSColor<T> UIDarkPurpleColor();
	static NSColor<T> UINavyColor();
	static NSColor<T> UIDarkNavyColor();
	static NSColor<T> UILightOrangeColor();
	static NSColor<T> UIOrangeColor();
	static NSColor<T> UIDarkOrangeColor();
	static NSColor<T> UIOrangeRedColor();
	static NSColor<T> UIRedColor();
	static NSColor<T> UIDarkRedColor();
	static NSColor<T> UILighterGrayColor();
	static NSColor<T> UILightGrayColor();
	static NSColor<T> UIGrayColor();
	static NSColor<T> UIDarkGrayColor();
	
	NSColor();
	NSColor(T r, T g, T b);
	NSColor(T r, T g, T b, T a);
	NSColor(uint32_t rgbaValue);
	
	NSColor<float> FloatRepresentation() const;
	NSColor<uint8_t> ByteRepresentation() const;
	uint32_t RGBAValue() const;
	
	NSColor<T> AlphaColor(T a) const;
	
	operator NSColor<float>() const;
	operator NSColor<uint8_t>() const;
	explicit operator uint32_t() const;
		
	NSColor<T> operator +(const NSColor<T>& c) const;
	NSColor<T>& operator +=(const NSColor<T>& c);
	NSColor<T> operator -(const NSColor<T>& c) const;
	NSColor<T>& operator -=(const NSColor<T>& c);
	NSColor<T> operator *(float v) const;
	NSColor<T>& operator *=(float v);
	NSColor<T> operator /(float v) const;
	NSColor<T>& operator /=(float v);
	
	bool operator ==(const NSColor<T>& c) const;
	bool operator !=(const NSColor<T>& c) const;
	
	T r, g, b, a;
};

template <class T>
NSColor<T> operator *(float v, const NSColor<T>& c);

uint8_t NSColorFloatToByte(float value);
float NSColorByteToFloat(uint8_t value);

// Blend using "over" alpha with fg.a being equal to the alpha component
NSColor<float> NSColorBlend(const NSColor<float>& fg, const NSColor<float>& bg);

#endif /* NSCOLOR_H */
