//
//  types.c
//  NeilOS
//
//  Created by Neil Singh on 12/25/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#include "types.h"

// Create a 64bit integer
uint64_t uint64_make(uint32_t high, uint32_t low) {
	return (uint64_t){ .high = high, .low = low };
}

// Add two 64bit integers
uint64_t uint64_add(uint64_t a, uint64_t b) {
	a.low += b.low;
	a.high += b.high;
	// Overflow
	if (a.low < b.low)
		a.high++;
	
	return a;
}

// Subtract (a - b)
uint64_t uint64_sub(uint64_t a, uint64_t b) {
	// Classic two's complement addition (a - b = a + (~b + 1))
	return uint64_inc(uint64_add(a, uint64_not(b)));
}

// Increment a 64bit integer
uint64_t uint64_inc(uint64_t a) {
	a.low++;
	if (a.low == 0)
		a.high++;
	
	return a;
}

// Shift Left
uint64_t uint64_shl(uint64_t a, uint8_t bits) {
	if (bits >= 32) {
		a.high = a.low << (bits - 32);
		a.low = 0;
		
		return a;
	}
	
	a.high <<= bits;
	a.high |= (a.low >> (32 - bits));
	a.low <<= bits;
	
	return a;
}

// Shift right
uint64_t uint64_shr(uint64_t a, uint8_t bits) {
	if (bits >= 32) {
		a.low = a.high >> (bits - 32);
		a.high = 0;
		
		return a;
	}
	
	a.low >>= bits;
	a.low |= (a.high & ((1 << bits) - 1)) << (32 - bits);
	a.high >>= bits;
	
	return a;
}

// Not
uint64_t uint64_not(uint64_t a) {
	a.high = ~a.high;
	a.low = ~a.low;
	
	return a;
}

// Greater than (a > b)
bool uint64_greater(uint64_t a, uint64_t b) {
	if (a.high == b.high)
		return a.low > b.low;
	
	return a.high > b.high;
}

// Less than (a < b)
bool uint64_less(uint64_t a, uint64_t b) {
	if (a.high == b.high)
		return a.low < b.low;
	
	return a.high < b.high;
}

// Equal (a == b)
bool uint64_equal(uint64_t a, uint64_t b) {
	return (a.high == b.high && a.low == b.low);
}
