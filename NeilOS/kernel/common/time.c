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
date_t get_current_date() {
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
		ret[0].year = bcd_to_binary(ret[0].year);
		century[0] = bcd_to_binary(century[0]);
	}
	
	// Convert 12 hour to 24 hour clock
	if (options & RTC_FORMAT_12)
		ret[0].hour = ((ret[0].hour & 0x7F) + 12) % 24;
	
	// Calculate the full year
	ret[0].year += century[0] * 100;
	
	return ret[0];
}

#define SECONDS_PER_MIN				60
#define SECONDS_PER_HOUR			3600
#define SECONDS_PER_DAY				86400
#define SECONDS_PER_MONTH			2629743
#define SECONDS_PER_YEAR			31556926

#define EPOCH_MONTH					1
#define EPOCH_DAY					1
#define EPOCH_YEAR					1970

// Returns the current UNIX time
time_t get_current_time() {
	uint32_t val = 0;
	
	// Adjust the time to unix
	date_t date = get_current_date();
	date.month -= EPOCH_MONTH;
	date.day -= EPOCH_DAY;
	date.year -= EPOCH_YEAR;
	
	// Convert to unix time
	val = date.second +
		date.minute * SECONDS_PER_MIN +
		date.hour * SECONDS_PER_HOUR +
		date.day * SECONDS_PER_DAY +
		date.month * SECONDS_PER_MONTH +
		date.year * SECONDS_PER_YEAR;
	
	return (time_t){ val };
}
