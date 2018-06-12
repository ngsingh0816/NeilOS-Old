//
//  NSMatrix.h
//  product
//
//  Created by Neil Singh on 2/5/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSMATRIX_H
#define NSMATRIX_H

class NSMatrix {
public:
	static NSMatrix Identity();
	static NSMatrix Perspective(float fovy, float aspect, float zNear, float zFar);
	static NSMatrix Ortho2D(float left, float right, float bottom, float top);
	
	NSMatrix();		// all 0's
	NSMatrix(float a11, float a12, float a13, float a14,
		   float a21, float a22, float a23, float a24,
		   float a31, float a32, float a33, float a34,
		   float a41, float a42, float a43, float a44);
	NSMatrix(const float data[16]);
	NSMatrix(const NSMatrix& matrix);
	
	const float* GetData() const;
	void SetData(const float data[16]);
	float& operator[](unsigned int index);
	
	NSMatrix& Translate(float x, float y, float z);
	NSMatrix& Scale(float x, float y, float z);
	// Rotate around a vector (angle in degrees)
	NSMatrix& Rotate(float x, float y, float z, float angle);
	
	NSMatrix& Multiply(const NSMatrix& matrix);
	NSMatrix operator *(const NSMatrix& matrix) const;
	NSMatrix& operator *=(const NSMatrix& matrix);
private:
	float data[16];
};

#endif /* NSMATRIX_H */
