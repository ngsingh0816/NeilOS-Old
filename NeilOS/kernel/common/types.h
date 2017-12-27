/* types.h - Defines to use the familiar explicitly-sized types in this
 * OS (uint32_t, int8_t, etc.).  This is necessary because we don't want
 * to include <stdint.h> when building this OS
 * vim:ts=4 noexpandtab
 */

#ifndef _TYPES_H
#define _TYPES_H

#include "debug.h"

#define NULL	0
#define true	1
#define false	0

#define DEBUG				1

#ifndef ASM

typedef int int32_t;
typedef unsigned int uint32_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef char int8_t;
typedef unsigned char uint8_t;

typedef char bool;

// Emulate a 64 bit unsigned integer
typedef  struct __attribute__((__packed__)) {
	uint32_t high;
	uint32_t low;
} uint64_t;

// Create
uint64_t uint64_make(uint32_t high, uint32_t low);
// Add (a + b)
uint64_t uint64_add(uint64_t a, uint64_t b);
// Subtract (a - b)
uint64_t uint64_sub(uint64_t a, uint64_t b);
// Increment (a++)
uint64_t uint64_inc(uint64_t a);
// Shift Left (a << bits)
uint64_t uint64_shl(uint64_t a, uint8_t bits);
// Shift right (a >> bits)
uint64_t uint64_shr(uint64_t a, uint8_t bits);
// Not (~a)
uint64_t uint64_not(uint64_t a);
// Greater than (a > b)
bool uint64_greater(uint64_t a, uint64_t b);
// Less than (a < b)
bool uint64_less(uint64_t a, uint64_t b);
// Equal (a == b)
bool uint64_equal(uint64_t a, uint64_t b);

#endif /* ASM */

#endif /* _TYPES_H */
