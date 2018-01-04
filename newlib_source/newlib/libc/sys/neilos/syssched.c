//
//  syssched.c
//  
//
//  Created by Neil Singh on 8/31/17.
//
//

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <sched.h>

extern unsigned int sys_nanosleep(struct timespec* req, struct timespec* rem);
extern unsigned int sys_sched_yield();

unsigned int nanosleep(struct timespec* req, struct timespec* rem) {
	if (req->tv_sec < 0 || req->tv_nsec < 0 || req->tv_nsec > 999999999) {
		errno = EINVAL;
		return -1;
	}
	int ret = sys_nanosleep(req, rem);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int sched_yield() {
	int ret = sys_sched_yield();
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}
