//
//  log.c
//  NeilOS
//
//  Created by Neil Singh on 7/7/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "log.h"
#include <drivers/filesystem/filesystem.h>
#include <common/lib.h>
#include <common/time.h>

file_descriptor_t log_file = { .ref_count = 0 };

// Log a message to /var/log/syslog
void log(int8_t* format, ...) {
	if (log_file.ref_count == 0) {
		if (!fopen("var/log/syslog", FILE_MODE_CREATE | FILE_MODE_APPEND, &log_file)) {
			// Create the directories var and var/log
			fmkdir("var");
			fmkdir("var/log");
			if (!fopen("var/log/syslog", FILE_MODE_CREATE | FILE_MODE_APPEND, &log_file)) {
	#if DEBUG
				blue_screen("Logging error.");
	#endif
				return;
			}
		}
		log_file.ref_count = 1;
	}
	
	char str[1024];
	int len = sget_current_date(str);
	fwrite(str, 1, len, &log_file);
	fwrite(" - ", 1, 3, &log_file);
	
	// Pointer to the format string
	const int8_t* buf = format;
	
	// Stack pointer for the other parameters
	int32_t* esp = (void *)&format;
	esp++;
	
	int32_t ptr = 0;
	
	while(*buf != '\0') {
		switch(*buf) {
			case '%':
			{
				int32_t alternate = 0;
				buf++;
				
			format_char_switch:
				// Conversion specifiers
				switch(*buf) {
						// Print a literal '%' characters
					case '%':
						str[ptr++] = '%';
						break;
						
						// Use alternate formatting
					case '#':
						alternate = 1;
						buf++;
						// Yes, I know gotos are bad.  This is the
						// most elegant and general way to do this,
						// IMHO.
						goto format_char_switch;
						
						// Print a number in hexadecimal form
					case 'x':
					{
						int8_t conv_buf[64];
						if(alternate == 0) {
							itoa(*((uint32_t *)esp), conv_buf, 16);
							int32_t len = strlen(conv_buf);
							memcpy(&str[ptr], conv_buf, len);
							ptr += len;
						} else {
							int32_t starting_index;
							int32_t i;
							itoa(*((uint32_t *)esp), &conv_buf[8], 16);
							i = starting_index = strlen(&conv_buf[8]);
							while(i < 8) {
								conv_buf[i] = '0';
								i++;
							}
							int32_t len = strlen(&conv_buf[starting_index]);
							memcpy(&str[ptr], &conv_buf[starting_index], len);
							ptr += len;
						}
						esp++;
					}
						break;
						
						// Print a number in unsigned int form
					case 'u':
					{
						int8_t conv_buf[36];
						itoa(*((uint32_t *)esp), conv_buf, 10);
						int32_t len = strlen(conv_buf);
						memcpy(&str[ptr], conv_buf, len);
						ptr += len;
						esp++;
					}
						break;
						
						// Print a number in signed int form
					case 'i':
					case 'd':
					{
						int8_t conv_buf[36];
						int32_t value = *((int32_t *)esp);
						if(value < 0) {
							conv_buf[0] = '-';
							itoa(-value, &conv_buf[1], 10);
						} else {
							itoa(value, conv_buf, 10);
						}
						int32_t len = strlen(conv_buf);
						memcpy(&str[ptr], conv_buf, len);
						ptr += len;
						esp++;
					}
						break;
						
						// Print a single character
					case 'c':
						str[ptr++] = (uint8_t)*((int32_t*)esp);
						esp++;
						break;
						
						// Print a NULL-terminated string
					case 's': {
						char* string = *((int8_t **)esp);
						int32_t len = strlen(string);
						memcpy(&str[ptr], string, len);
						ptr += len;
						esp++;
						break;
					}
						
					default:
						break;
				}
				
			}
				break;
				
			default:
				str[ptr++] = *buf;
				break;
		}
		buf++;
	}
	
	
	fwrite(str, 1, ptr, &log_file);
	fwrite("\n", 1, 1, &log_file);
}
