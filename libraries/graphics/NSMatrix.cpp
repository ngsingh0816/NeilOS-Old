//
//  NSMatrix.cpp
//  product
//
//  Created by Neil Singh on 2/5/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSMatrix.h"
#include <string.h>
#include <math.h>

NSMatrix NSMatrix::Identity() {
	NSMatrix m;
	m[0] = 1.0f;
	m[5] = 1.0f;
	m[10] = 1.0f;
	m[15] = 1.0f;
	return m;
}

NSMatrix NSMatrix::Perspective(float fovy, float aspect, float zNear, float zFar) {
	const float h = 1.0f / tanf(fovy / 360.0f * M_PI);
	const float neg_depth = zNear - zFar;
	
	NSMatrix m;
	m[0] = h / aspect;
	m[5] = h;
	m[10] = -(zFar + zNear) / neg_depth;
	m[11] = 1.0f;
	m[14] = (zNear * zFar) / neg_depth;
	m[15] = 0.0f;
	
	return m;
}

NSMatrix NSMatrix::Ortho2D(float left, float right, float bottom, float top) {
	constexpr float n = -1.0f;
	constexpr float f = 1.0f;
	NSMatrix m;
	
	m[0] = 2.0f / (right - left);
	m[5] = 2.0f / (top - bottom);
	m[10] = 2.0f / (f - n);
	m[12] = -(right + left) / (right - left);
	m[13] = -(top + bottom) / (top - bottom);
	m[14] = -(f + n) / (f - n);
	m[15] = 1.0f;
	
	return m;
}

NSMatrix::NSMatrix() {
	memset(data, 0, sizeof(float) * 16);
}

NSMatrix::NSMatrix(float a11, float a12, float a13, float a14,
				   float a21, float a22, float a23, float a24,
				   float a31, float a32, float a33, float a34,
				   float a41, float a42, float a43, float a44) {
	data[0] = a11;
	data[1] = a12;
	data[2] = a13;
	data[3] = a14;
	
	data[4] = a21;
	data[5] = a22;
	data[6] = a23;
	data[7] = a24;

	data[8] =  a31;
	data[9] =  a32;
	data[10] = a33;
	data[11] = a34;

	data[12] = a41;
	data[13] = a42;
	data[14] = a43;
	data[15] = a44;

}

NSMatrix::NSMatrix(const float _data[16]) {
	memcpy(data, _data, sizeof(float) * 16);
}

NSMatrix::NSMatrix(const NSMatrix& matrix) {
	memcpy(data, matrix.data, sizeof(float) * 16);
}

float* NSMatrix::GetData() {
	return data;
}

float& NSMatrix::operator[](unsigned int index) {
	return data[index];
}

void NSMatrix::SetData(const float _data[16]) {
	memcpy(data, _data, sizeof(float) * 16);
}

NSMatrix& NSMatrix::Translate(float x, float y, float z) {
	NSMatrix m;
	m[0] = 1.0f;
	m[5] = 1.0f;
	m[10] = 1.0f;
	m[12] = x;
	m[13] = y;
	m[14] = z;
	m[15] = 1.0f;
	*this = m * *this;
	return *this;
}

NSMatrix& NSMatrix::Scale(float x, float y, float z) {
	NSMatrix m;
	m[0] = x;
	m[5] = y;
	m[10] = z;
	m[15] = 1.0f;
	*this = m * *this;
	return *this;
}

// Rotate around a vector (angle in degrees)
NSMatrix& NSMatrix::Rotate(float x, float y, float z, float angle) {

	float mag = sqrtf((x*x) + (y*y) + (z*z));
	if (mag == 0.0)
		return *this;
	else if (mag != 1.0)
	{
		x /= mag;
		y /= mag;
		z /= mag;
	}
	
	float degAngle = angle / 180 * M_PI;
	float c = cos(degAngle);
	float ic = 1.0f - c;
	float s = sin(degAngle);
	
	NSMatrix m;
	m[0] = (x*x)*ic + c;
	m[1] = (y*x)*ic + (z*s);
	m[2] = (x*z)*ic - (y*s);
	m[4] = (x*y)*ic-(z*s);
	m[5] = (y*y)*ic+c;
	m[6] = (y*z)*ic+(x*s);
	m[8] = (x*z)*ic+(y*s);
	m[9] = (y*z)*ic-(x*s);
	m[10] = (z*z)*ic+c;
	m[15] = 1.0f;
	
	*this = m * *this;
	return *this;
}

NSMatrix& NSMatrix::Multiply(const NSMatrix& matrix) {
	*this = *this * matrix;
	return *this;
}

NSMatrix NSMatrix::operator *(const NSMatrix& matrix) const {
	NSMatrix res;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			float sum = 0.0f;
			for (int k = 0; k < 4; k++)
				sum += data[i * 4 + k] * matrix.data[k * 4 + j];
			res[i * 4 + j] = sum;
		}
	}
	
	return res;
}

NSMatrix& NSMatrix::operator *=(const NSMatrix& matrix) {
	return Multiply(matrix);
}
