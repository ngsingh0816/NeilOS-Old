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

#define NANOS_IN_SEC	(1000 * 1000 * 1000)
#define NANOS_IN_MS		(1000 * 1000)
#define NANOS_IN_US		1000
#define US_IN_SEC		(1000 * 1000)
#define US_IN_MS		1000
#define MS_IN_SEC		1000

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
	uint32_t tv_sec;
	uint32_t tv_usec;
};

struct timespec {
	uint32_t tv_sec;
	uint32_t tv_nsec;
};

// Returns the current date (GMT)
date_t get_current_date();
// Returns a string of the current date
int sget_current_date(char* str);

// Returns the current UNIX time
time_t get_current_unix_time();

void time_load_current();
struct timeval time_get();
void time_increment_ms(int ms);
struct timeval time_add(struct timeval t1, struct timeval t2);
struct timeval time_subtract(struct timeval t1, struct timeval t2);
bool time_less(struct timeval t1, struct timeval t2);
struct timeval timespec_to_timeval(struct timespec t);
struct timespec timeval_to_timespec(struct timeval t);

#endif
