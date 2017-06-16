#ifndef RTC_H
#define RTC_H

#include <common/types.h>
#include <syscalls/descriptor.h>

// Init the rtc
void rtc_init();
file_descriptor_t* rtc_open(const char* filename, uint32_t mode);
// Set the rate of the RTC
bool rtc_set_freq(uint32_t hz);
// Set the virtual rate of the RTC
uint32_t rtc_write(int32_t fd, const void* buf, uint32_t nbytes);
// Blocks until an interrupt occurs
uint32_t rtc_read(int32_t fd, void* buf, uint32_t bytes);
file_descriptor_t* rtc_duplicate(file_descriptor_t* f);
uint32_t rtc_close(file_descriptor_t* fd);

#endif
