//
//  NSColor.cpp
//  product
//
//  Created by Neil Singh on 1/29/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSColor.h"

#include <algorithm>

#include <math.h>

#define clamp(a, b, c)	(std::max(std::min((a), (c)), (b)))

template <>
NSColor<uint8_t>::NSColor() {
	r = 0;
	g = 0;
	b = 0;
	a = 255;
}

template <>
NSColor<float>::NSColor() {
	r = 0;
	g = 0;
	b = 0;
	a = 1;
}

template <>
NSColor<uint8_t>::NSColor(uint8_t _r, uint8_t _g, uint8_t _b) {
	r = _r;
	g = _g;
	b = _b;
	a = 255;
}

template <>
NSColor<float>::NSColor(float _r, float _g, float _b) {
	r = _r;
	g = _g;
	b = _b;
	a = 1.0;
}

template <>
NSColor<uint8_t>::NSColor(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) {
	r = _r;
	g = _g;
	b = _b;
	a = _a;
}

template <>
NSColor<float>::NSColor(float _r, float _g, float _b, float _a) {
	r = _r;
	g = _g;
	b = _b;
	a = _a;
}

template <>
NSColor<uint8_t>::NSColor(uint32_t value) {
	r = ((value >> 16) & 0xFF);
	g = ((value >> 8) & 0xFF);
	b = (value & 0xFF);
	a = ((value >> 24) & 0xFF);
}

template <>
NSColor<float>::NSColor(uint32_t value) {
	r = ((value >> 16) & 0xFF) / 255.0f;
	g = ((value >> 8) & 0xFF) / 255.0f;
	b = (value & 0xFF) / 255.0f;
	a = ((value >> 24) & 0xFF) / 255.0f;
}

template <>
NSColor<float> NSColor<uint8_t>::FloatRepresentation() const {
	return NSColor<float>(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

template <>
NSColor<float> NSColor<float>::FloatRepresentation() const {
	return *this;
}

template <>
NSColor<uint8_t> NSColor<uint8_t>::ByteRepresentation() const {
	return *this;
}

template <>
NSColor<uint8_t> NSColor<float>::ByteRepresentation() const {
	return NSColor<uint8_t>(clamp(r * 255.0f, 0.0f, 255.0f),
							clamp(g * 255.0f, 0.0f, 255.0f),
							clamp(b * 255.0f, 0.0f, 255.0f),
							clamp(a * 255.0f, 0.0f, 255.0f));
}

template<>
uint32_t NSColor<uint8_t>::RGBAValue() const {
	return (a << 24) | (r << 16) | (g << 8) | b;
}

template<>
uint32_t NSColor<float>::RGBAValue() const {
	NSColor<uint8_t> c = ByteRepresentation();
	return (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::AlphaColor(uint8_t a) const {
	NSColor<uint8_t> ret = *this;
	ret.a = a;
	return ret;
}

template<>
NSColor<float> NSColor<float>::AlphaColor(float a) const {
	NSColor<float> ret = *this;
	ret.a = a;
	return ret;
}

template <>
NSColor<float>::operator NSColor<float>() const {
	return *this;
}

template <>
NSColor<uint8_t>::operator NSColor<float>() const {
	return FloatRepresentation();
}

template <>
NSColor<float>::operator NSColor<uint8_t>() const {
	return ByteRepresentation();
}

template <>
NSColor<uint8_t>::operator NSColor<uint8_t>() const {
	return *this;
}

template <>
NSColor<uint8_t>::operator uint32_t() const {
	return RGBAValue();
}

template <>
NSColor<float>::operator uint32_t() const {
	return RGBAValue();
}

template <>
NSColor<uint8_t> NSColor<uint8_t>::operator +(const NSColor<uint8_t>& c) const {
	return NSColor<uint8_t>(r + c.r, g + c.g, b + c.b, a + c.a);
}

template<>
NSColor<uint8_t>& NSColor<uint8_t>::operator +=(const NSColor<uint8_t>& c) {
	r += c.r;
	g += c.g;
	b += c.b;
	a += c.a;
	
	return *this;
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::operator -(const NSColor<uint8_t>& c) const {
	return NSColor<uint8_t>(r - c.r, g - c.g, b - c.b, a - c.a);
}

template<>
NSColor<uint8_t>& NSColor<uint8_t>::operator -=(const NSColor<uint8_t>& c) {
	r -= c.r;
	g -= c.g;
	b -= c.b;
	a -= c.a;
	
	return *this;
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::operator *(float v) const {
	return NSColor<uint8_t>(r * v, g * v, b * v, a * v);
}

template<>
NSColor<uint8_t>& NSColor<uint8_t>::operator *=(float v) {
	r *= v;
	g *= v;
	b *= v;
	a *= v;
	
	return *this;
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::operator /(float v) const {
	return NSColor<uint8_t>(r / v, g / v, b / v, a / v);
}

template<>
NSColor<uint8_t>& NSColor<uint8_t>::operator /=(float v) {
	r /= v;
	g /= v;
	b /= v;
	a /= v;
	
	return *this;
}

template <>
NSColor<float> NSColor<float>::operator +(const NSColor<float>& c) const {
	return NSColor<float>(r + c.r, g + c.g, b + c.b, a + c.a);
}

template<>
NSColor<float>& NSColor<float>::operator +=(const NSColor<float>& c) {
	r += c.r;
	g += c.g;
	b += c.b;
	a += c.a;
	
	return *this;
}

template<>
NSColor<float> NSColor<float>::operator -(const NSColor<float>& c) const {
	return NSColor<float>(r - c.r, g - c.g, b - c.b, a - c.a);
}

template<>
NSColor<float>& NSColor<float>::operator -=(const NSColor<float>& c) {
	r -= c.r;
	g -= c.g;
	b -= c.b;
	a -= c.a;
	
	return *this;
}

template<>
NSColor<float> NSColor<float>::operator *(float v) const {
	return NSColor<float>(r * v, g * v, b * v, a * v);
}

template<>
NSColor<float>& NSColor<float>::operator *=(float v) {
	r *= v;
	g *= v;
	b *= v;
	a *= v;
	
	return *this;
}

template<>
NSColor<uint8_t> operator *(float v, const NSColor<uint8_t>& c) {
	return NSColor<uint8_t>(c.r * v, c.g * v, c.b * v, c.a * v);
}

template<>
NSColor<float> operator *(float v, const NSColor<float>& c) {
	return NSColor<float>(c.r * v, c.g * v, c.b * v, c.a * v);
}

template<>
NSColor<float> NSColor<float>::operator /(float v) const {
	return NSColor<float>(r / v, g / v, b / v, a / v);
}

template<>
NSColor<float>& NSColor<float>::operator /=(float v) {
	r /= v;
	g /= v;
	b /= v;
	a /= v;
	
	return *this;
}

template<>
bool NSColor<uint8_t>::operator ==(const NSColor<uint8_t>& c) const {
	return (c.r == r && c.g == g && c.b == b && c.a == a);
}

template<>
bool NSColor<float>::operator ==(const NSColor<float>& c) const {
	constexpr float esp = 1 / 255.0;
	return (fabs(c.r - r) <= esp && fabs(c.g - g) <= esp && fabs(c.b - b) <= esp && fabs(c.a - a) <= esp);
}

template<>
bool NSColor<uint8_t>::operator !=(const NSColor<uint8_t>& c) const {
	return !(*this == c);
}

template<>
bool NSColor<float>::operator !=(const NSColor<float>& c) const {
	return !(*this == c);
}

uint8_t NSColorFloatToByte(float value) {
	return clamp(int(value * 255.0f + 0.5f), 0, 255);
}

float NSColorByteToFloat(float value) {
	return value / 255.0f;
}

// Blend using "over" alpha with fg.a being equal to the alpha component
NSColor<float> NSColorBlend(const NSColor<float>& fg, const NSColor<float>& bg) {
	if (fg.a > 254.0f / 255.0f)
		return fg;
	else if (fg.a < 1.0f / 255.0f)
		return bg;
	
	float a = fg.a;
	float inv_a = 1.0f - a;
	return NSColor<float>(fg.r * a + bg.r * inv_a,
						  fg.g * a + bg.g * inv_a,
						  fg.b * a + bg.b * inv_a);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::WhiteColor() {
	return NSColor<uint8_t>(255, 255, 255);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::BlackColor() {
	return NSColor<uint8_t>(0, 0, 0);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::LightGreyColor() {
	return NSColor<uint8_t>(192, 192, 192);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::GreyColor() {
	return NSColor<uint8_t>(128, 128, 128);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::DarkGreyColor() {
	return NSColor<uint8_t>(64, 64, 64);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::TransparentColor() {
	return NSColor<uint8_t>(0, 0, 0, 0);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::RedColor() {
	return NSColor<uint8_t>(230, 25, 75);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::GreenColor() {
	return NSColor<uint8_t>(60, 180, 75);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::YellowColor() {
	return NSColor<uint8_t>(255, 225, 25);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::BlueColor() {
	return NSColor<uint8_t>(0, 130, 200);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::PurpleColor() {
	return NSColor<uint8_t>(145, 30, 180);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::CyanColor() {
	return NSColor<uint8_t>(70, 240, 240);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::MagentaColor() {
	return NSColor<uint8_t>(240, 50, 230);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::LimeColor() {
	return NSColor<uint8_t>(210, 245, 60);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::PinkColor() {
	return NSColor<uint8_t>(250, 190, 190);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::TealColor() {
	return NSColor<uint8_t>(0, 128, 128);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::LavenderColor() {
	return NSColor<uint8_t>(230, 190, 255);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::BrownColor() {
	return NSColor<uint8_t>(170, 110, 40);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::BeigeColor() {
	return NSColor<uint8_t>(255, 250, 200);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::MaroonColor() {
	return NSColor<uint8_t>(128, 0, 0);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::MintColor() {
	return NSColor<uint8_t>(170, 255, 195);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::OliveColor() {
	return NSColor<uint8_t>(128, 128, 0);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::CoralColor() {
	return NSColor<uint8_t>(255, 215, 180);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::NavyColor() {
	return NSColor<uint8_t>(0, 0, 128);
}

template<>
NSColor<float> NSColor<float>::WhiteColor() {
	return NSColor<float>(255 / 255.0f, 255 / 255.0f, 255 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::BlackColor() {
	return NSColor<float>(0 / 255.0f, 0 / 255.0f, 0 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::LightGreyColor() {
	return NSColor<float>(192 / 255.0f, 192 / 255.0f, 192 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::GreyColor() {
	return NSColor<float>(128 / 255.0f, 128 / 255.0f, 128 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::DarkGreyColor() {
	return NSColor<float>(64 / 255.0f, 64 / 255.0f, 64 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::TransparentColor() {
	return NSColor<float>(0 / 255.0f, 0 / 255.0f, 0 / 255.0f, 0 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::RedColor() {
	return NSColor<float>(230 / 255.0f, 25 / 255.0f, 75 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::GreenColor() {
	return NSColor<float>(60 / 255.0f, 180 / 255.0f, 75 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::YellowColor() {
	return NSColor<float>(255 / 255.0f, 225 / 255.0f, 25 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::BlueColor() {
	return NSColor<float>(0 / 255.0f, 130 / 255.0f, 200 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::PurpleColor() {
	return NSColor<float>(145 / 255.0f, 30 / 255.0f, 180 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::CyanColor() {
	return NSColor<float>(70 / 255.0f, 240 / 255.0f, 240 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::MagentaColor() {
	return NSColor<float>(240 / 255.0f, 50 / 255.0f, 230 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::LimeColor() {
	return NSColor<float>(210 / 255.0f, 245 / 255.0f, 60 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::PinkColor() {
	return NSColor<float>(250 / 255.0f, 190 / 255.0f, 190 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::TealColor() {
	return NSColor<float>(0 / 255.0f, 128 / 255.0f, 128 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::LavenderColor() {
	return NSColor<float>(230 / 255.0f, 190 / 255.0f, 255 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::BrownColor() {
	return NSColor<float>(170 / 255.0f, 110 / 255.0f, 40 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::BeigeColor() {
	return NSColor<float>(255 / 255.0f, 250 / 255.0f, 200 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::MaroonColor() {
	return NSColor<float>(128 / 255.0f, 0 / 255.0f, 0 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::MintColor() {
	return NSColor<float>(170 / 255.0f, 255 / 255.0f, 195 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::OliveColor() {
	return NSColor<float>(128 / 255.0f, 128 / 255.0f, 0 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::CoralColor() {
	return NSColor<float>(255 / 255.0f, 215 / 255.0f, 180 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::NavyColor() {
	return NSColor<float>(0 / 255.0f, 0 / 255.0f, 128 / 255.0f);
}

// UI Colors
template<>
NSColor<uint8_t> NSColor<uint8_t>::UITurquoiseColor() {
	return NSColor<uint8_t>(0x1A, 0xBC, 0x9C);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIDarkTurquoiseColor() {
	return NSColor<uint8_t>(0x16, 0xA0, 0x85);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIGreenColor()  {
	return NSColor<uint8_t>(0x2E, 0xCC, 0x71);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIDarkGreenColor() {
	return NSColor<uint8_t>(0x27, 0xAE, 0x60);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIBlueColor() {
	return NSColor<uint8_t>(0x34, 0x98, 0xDB);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIDarkBlueColor() {
	return NSColor<uint8_t>(0x29, 0x80, 0xB9);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIPurpleColor() {
	return NSColor<uint8_t>(0x9B, 0x59, 0xB6);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIDarkPurpleColor() {
	return NSColor<uint8_t>(0x8E, 0x44, 0xAD);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UINavyColor() {
	return NSColor<uint8_t>(0x34, 0x49, 0x5E);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIDarkNavyColor() {
	return NSColor<uint8_t>(0x2C, 0x3E, 0x50);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UILightOrangeColor() {
	return NSColor<uint8_t>(0xF1, 0xC4, 0x0F);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIOrangeColor() {
	return NSColor<uint8_t>(0xF3, 0x9C, 0x12);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIDarkOrangeColor() {
	return NSColor<uint8_t>(0xE6, 0x7E, 0x22);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIOrangeRedColor() {
	return NSColor<uint8_t>(0xD3, 0x54, 0x00);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIRedColor() {
	return NSColor<uint8_t>(0xE7, 0x4C, 0x3C);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIDarkRedColor() {
	return NSColor<uint8_t>(0xC0, 0x39, 0x2B);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UILighterGrayColor() {
	return NSColor<uint8_t>(0xEC, 0xF0, 0xF1);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UILightGrayColor() {
	return NSColor<uint8_t>(0xBD, 0xC3, 0xC7);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIGrayColor() {
	return NSColor<uint8_t>(0x95, 0xA5, 0xA6);
}

template<>
NSColor<uint8_t> NSColor<uint8_t>::UIDarkGrayColor() {
	return NSColor<uint8_t>(0x7F, 0x8C, 0x8D);
}

template<>
NSColor<float> NSColor<float>::UITurquoiseColor() {
	return NSColor<float>(0x1A / 255.0f, 0xBC / 255.0f, 0x9C / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIDarkTurquoiseColor() {
	return NSColor<float>(0x16 / 255.0f, 0xA0 / 255.0f, 0x85 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIGreenColor()  {
	return NSColor<float>(0x2E / 255.0f, 0xCC / 255.0f, 0x71 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIDarkGreenColor() {
	return NSColor<float>(0x27 / 255.0f, 0xAE / 255.0f, 0x60 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIBlueColor() {
	return NSColor<float>(0x34 / 255.0f, 0x98 / 255.0f, 0xDB / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIDarkBlueColor() {
	return NSColor<float>(0x29 / 255.0f, 0x80 / 255.0f, 0xB9 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIPurpleColor() {
	return NSColor<float>(0x9B / 255.0f, 0x59 / 255.0f, 0xB6 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIDarkPurpleColor() {
	return NSColor<float>(0x8E / 255.0f, 0x44 / 255.0f, 0xAD / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UINavyColor() {
	return NSColor<float>(0x34 / 255.0f, 0x49 / 255.0f, 0x5E / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIDarkNavyColor() {
	return NSColor<float>(0x2C / 255.0f, 0x3E / 255.0f, 0x50 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UILightOrangeColor() {
	return NSColor<float>(0xF1 / 255.0f, 0xC4 / 255.0f, 0x0F / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIOrangeColor() {
	return NSColor<float>(0xF3 / 255.0f, 0x9C / 255.0f, 0x12 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIDarkOrangeColor() {
	return NSColor<float>(0xE6 / 255.0f, 0x7E / 255.0f, 0x22 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIOrangeRedColor() {
	return NSColor<float>(0xD3 / 255.0f, 0x54 / 255.0f, 0x00 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIRedColor() {
	return NSColor<float>(0xE7 / 255.0f, 0x4C / 255.0f, 0x3C / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIDarkRedColor() {
	return NSColor<float>(0xC0 / 255.0f, 0x39 / 255.0f, 0x2B / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UILighterGrayColor() {
	return NSColor<float>(0xEC / 255.0f, 0xF0 / 255.0f, 0xF1 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UILightGrayColor() {
	return NSColor<float>(0xBD / 255.0f, 0xC3 / 255.0f, 0xC7 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIGrayColor() {
	return NSColor<float>(0x95 / 255.0f, 0xA5 / 255.0f, 0xA6 / 255.0f);
}

template<>
NSColor<float> NSColor<float>::UIDarkGrayColor() {
	return NSColor<float>(0x7F / 255.0f, 0x8C / 255.0f, 0x8D / 255.0f);
}

#undef clamp
