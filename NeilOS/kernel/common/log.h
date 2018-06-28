//
//  log.h
//  NeilOS
//
//  Created by Neil Singh on 7/7/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef LOG_H
#define LOG_H

#include <common/types.h>

#if DEBUG
// Change this to 1 to enable debug logging
#define DEBUG_LOG	0
#else
#define DEBUG_LOG	0
#endif

// Change this to enable / disable output logging
#define OUTPUT_LOG		1

#if DEBUG_LOG
#define LOG_DEBUG(...) 	log(__VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif

#if OUTPUT_LOG
#define LOG_OUTPUT_DEBUG(...) 	log_output(__VA_ARGS__)
#else
#define LOG_OUTPUT_DEBUG(...)
#endif

#define LOG_DEBUG_INFO() \
	LOG_DEBUG("%s:%d - %s", __FILE__, __LINE__, __FUNCTION__)
#define LOG_DEBUG_INFO_STR(x, ...) \
	LOG_DEBUG("%s:%d - %s - " x, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

// Log a message to /var/log/syslog
void log(int8_t* fmt, ...);

// Log a message to /var/log/syslog without formatting
void quick_log(int8_t* fmt, ...);

// Log a message to /var/log/output.log
void log_output(int8_t* fmt, ...);

#endif /* LOG_H */
