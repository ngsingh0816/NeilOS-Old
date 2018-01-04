//
//  time.h
//  NeilOS
//
//  Created by Neil Singh on 1/10/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef TIME_H
#define TIME_H

#include "types.h"

// UNIX time struct
typedef struct {
	uint32_t val;
} time_t;

// Date
typedef struct {
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	int16_t year;
} date_t;

struct timeval {
	long tv_sec;
	long tv_usec;
};

// Returns the current date (GMT)
date_t get_current_date();
// Returns a string of the current date
int sget_current_date(char* str);

// Returns the current UNIX time
time_t get_current_time();

#endif
