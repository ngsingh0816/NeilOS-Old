//
//  time.c
//  NeilOS
//
//  Created by Neil Singh on 1/10/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "time.h"
#include <drivers/rtc/rtc.h>
#include "lib.h"

#define RTC_COMMAND_PORT	0x70
#define RTC_DATA_PORT		0x71

#define REGISTER_A_CMD		0x0A
#define REGISTER_B_CMD		0x0B
#define REGISTER_C_CMD		0x0C

#define RTC_UPDATE_IN_PROGRESS		0x80

#define RTC_SECOND			0x00
#define RTC_MINUTE			0x02
#define RTC_HOUR			0x04
#define RTC_DAY				0x07
#define RTC_MONTH			0x08
#define RTC_YEAR			0x09
#define RTC_CENTURY			0x32

#define RTC_FORMAT_BIN		0x04
#define RTC_FORMAT_12		0x02

struct timeval current_time;

// Check if an update is in progress
bool rtc_update_in_progress() {
	outb(REGISTER_A_CMD, RTC_COMMAND_PORT);
	return ((inb(RTC_DATA_PORT) & RTC_UPDATE_IN_PROGRESS) != 0);
}

// Get an RTC Date register
uint8_t rtc_get_reg(uint8_t reg) {
	outb(reg, RTC_COMMAND_PORT);
	return inb(RTC_DATA_PORT);
}

// Get the rtc values as a date
date_t rtc_get_values(uint8_t* century) {
	date_t d;
	d.second = rtc_get_reg(RTC_SECOND);
	d.minute = rtc_get_reg(RTC_MINUTE);
	d.hour = rtc_get_reg(RTC_HOUR);
	d.day = rtc_get_reg(RTC_DAY);
	d.month = rtc_get_reg(RTC_MONTH);
	d.year = rtc_get_reg(RTC_YEAR);
	if (century)
		*century = rtc_get_reg(RTC_CENTURY);
	
	return d;
}

// Returns true if two dates are equal
bool dates_equal(date_t d1, date_t d2, uint8_t c1, uint8_t c2) {
	return (d1.second == d2.second && d1.minute == d2.minute && d1.hour == d2.hour &&
			d1.day == d2.day && d1.month == d2.month && d1.year == d2.year && c1 == c2);
}

// Convert a bcd value to binary
uint8_t bcd_to_binary(uint8_t val) {
	return (val & 0xF) + ((val / 0x10) * 10);
}

// Returns the current date using the RTC
date_t get_current_date_rtc() {
	date_t ret[2];
	uint8_t century[2] = { 0, 0 };
	memset(ret, 0, sizeof(date_t) * 2);
	
	// Wait for an update to not be in progress
	while (rtc_update_in_progress());
	// Get the info
	ret[0] = rtc_get_values(&century[0]);
	
	// Go until we get two consecutive RTC time values
	do {
		ret[1] = ret[0];
		century[1] = century[0];
		// Get the info
		while (rtc_update_in_progress());
		ret[0] = rtc_get_values(&century[0]);
	} while (!dates_equal(ret[0], ret[1], century[0], century[1]));
	
	uint8_t options = rtc_get_reg(REGISTER_B_CMD);
	
	// Convert BCD to binary if needed
	if (!(options & RTC_FORMAT_BIN)) {
		ret[0].second = bcd_to_binary(ret[0].second);
		ret[0].minute = bcd_to_binary(ret[0].minute);
		// Needed special case to preserve format
		ret[0].hour = bcd_to_binary(ret[0].hour & 0x7F) | (ret[0].hour & 0x80);
		ret[0].day = bcd_to_binary(ret[0].day);
		ret[0].month = bcd_to_binary(ret[0].month);
		ret[0].year = bcd_to_binary(ret[0].year);
		century[0] = bcd_to_binary(century[0]);
	}
	
	// Convert 12 hour to 24 hour clock
	if (!(options & RTC_FORMAT_12)) {
		int hour = ret[0].hour;
		int ampm = hour & 0x80;
		hour = hour & 0x7f;
		// fix 24:00 = 12am oddity
		if (hour == 12)
			hour = 0;
		if (ampm)
			hour += 12;
		ret[0].hour = hour;
	}
	
	// Calculate the full year
	ret[0].year += century[0] * 100;
	
	return ret[0];
}

const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

int sget_current_date(char* str) {
	date_t date = get_current_date();
	return sprintf(str, "%s %d, %d %d:%d:%d", months[date.month - 1], (int)date.day, (int)date.year,
			(int)date.hour, (int)date.minute, (int)date.second);
}

#define SECONDS_PER_MIN				60
#define SECONDS_PER_HOUR			3600
#define SECONDS_PER_DAY				86400
#define SECONDS_PER_MONTH			2629743
#define SECONDS_PER_YEAR			31556926

#define EPOCH_MONTH					1
#define EPOCH_DAY					1
#define EPOCH_YEAR					1970
#define IS_LEAP_YEAR(year)  ((((year)%4 == 0) && ((year)%100 != 0)) || ((year)%400 == 0))

// Returns the current UNIX time
time_t get_current_unix_time_rtc() {
	
	// Adjust the time to unix
	date_t date = get_current_date_rtc();
	
	uint32_t ts = 0;
	
	static int days_per_month[2][12] = {
		{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
		{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
	};
	
	static int days_per_year[2] = {
		365, 366
	};
	
	//  Add up the seconds from all prev years, up until this year.
	uint8_t years = 0;
	uint8_t leap_years = 0;
	for(uint16_t y_k = EPOCH_YEAR; y_k < date.year; y_k++)
	{
		if (IS_LEAP_YEAR(y_k))
			leap_years++;
		else
			years++;
	}
	ts += ((years*days_per_year[0]) + (leap_years*days_per_year[1])) * SECONDS_PER_DAY;
	
	//  Add up the seconds from all prev days this year, up until today.
	uint8_t year_index = (IS_LEAP_YEAR(date.year)) ? 1 : 0;
	for(uint8_t mo_k = 0; mo_k< (date.month-1); mo_k++) {
		//  days from previous months this year
		ts += days_per_month[year_index][mo_k] * SECONDS_PER_DAY;
	}
	ts += (date.day-1) * SECONDS_PER_DAY; // days from this month
	
	//  Calculate seconds elapsed just today.
	ts += date.hour * SECONDS_PER_HOUR;
	ts += date.minute * SECONDS_PER_MIN;
	ts += date.second;
	
	return (time_t){ ts };
}

time_t get_current_unix_time() {
	return (time_t){ current_time.tv_sec };
}

date_t get_current_date() {
	uint32_t val = current_time.tv_sec;
	date_t date;
	
	date.second = val % 60;
	val /= 60;
	date.minute = val % 60;
	val /= 60;
	date.hour = val % 24;
	val /= 24;
	
	uint32_t a = ((4 * val + 102032) / 146097 + 15);
	uint32_t b = (val + 2442113 + a - (a / 4));
	uint32_t c = (20 * b - 2442) / 7305;
	uint32_t d = b - 365 * c - (c / 4);
	uint32_t e = d * 1000 / 30601;
	uint32_t f = d - e * 30 - e * 601 / 1000;
	if (e <= 13) {
		c -= 4716;
		e--;
	} else {
		c -= 4715;
		e -= 13;
	}
	
	date.year = c;
	date.month = e;
	date.day = f;
	
	return date;
}

void time_load_current() {
	current_time.tv_sec = get_current_unix_time_rtc().val;
	current_time.tv_usec = 0;
}

struct timeval time_get() {
	return current_time;
}

void time_increment_ms(int ms) {
	current_time.tv_usec += US_IN_MS * ms;
	unsigned int sec = current_time.tv_usec / US_IN_SEC;
	current_time.tv_usec -= sec * US_IN_SEC;
	current_time.tv_sec += sec;
}

struct timeval time_add(struct timeval t1, struct timeval t2) {
	t1.tv_usec += t2.tv_usec;
	unsigned int sec = t1.tv_usec / US_IN_SEC;
	t1.tv_usec -= sec * US_IN_SEC;
	t1.tv_sec += sec + t2.tv_sec;
	return t1;
}

struct timeval time_subtract(struct timeval t1, struct timeval t2) {
	if (t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_usec < t2.tv_usec)) {
		t1.tv_sec = 0;
		t1.tv_usec = 0;
		return t1;
	}
	while (t1.tv_usec < t2.tv_usec) {
		t1.tv_usec += US_IN_SEC;
		t1.tv_sec--;
	}
	t1.tv_sec -= t2.tv_sec;
	t1.tv_usec -= t2.tv_usec;
	return t1;
}

bool time_less(struct timeval t1, struct timeval t2) {
	if (t1.tv_sec > t2.tv_sec)
		return false;
	else if (t1.tv_sec == t2.tv_sec)
		return (t1.tv_usec < t2.tv_usec);
	return true;
}

struct timeval timespec_to_timeval(struct timespec t) {
	struct timeval ret;
	ret.tv_sec = t.tv_sec;
	ret.tv_usec = t.tv_nsec / NANOS_IN_US;
	return ret;
}

struct timespec timeval_to_timespec(struct timeval t) {
	struct timespec ret;
	ret.tv_sec = t.tv_sec;
	ret.tv_nsec = t.tv_usec * NANOS_IN_US;
	return ret;
}
