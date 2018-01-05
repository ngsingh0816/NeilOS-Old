//
//  systime.c
//  
//
//  Created by Neil Singh on 6/25/17.
//
//

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>

extern unsigned int sys_errno();

extern unsigned int sys_times(struct tms* buf);
extern unsigned int sys_gettimeofday(struct timeval* t);

clock_t times(struct tms *buf) {
	int ret = sys_times(buf);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int gettimeofday(struct timeval* p, void* z) {
	if (!p) {
		errno = EINVAL;
		return -1;
	}
	
	int ret = sys_gettimeofday(p);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int settimeofday(const struct timeval* tv, const struct timezone* tz) {
	errno = EPERM;
	return -1;
}

int clock_getres(clockid_t clk_id, struct timespec* res) {
	if (res) {
		res->tv_sec = 0;
		res->tv_nsec = 1000 * 1000;
	}
	return 0;
}

int clock_gettime(clockid_t clk_id, struct timespec* tp) {
	if (!tp)
		return 0;
	
	struct timeval p;
	int ret = sys_gettimeofday(&p);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	
	tp->tv_sec = p.tv_sec;
	tp->tv_nsec = p.tv_usec * 1000;
	
	return ret;
}

int clock_settime(clockid_t clk_id, const struct timespec* tp) {
	errno = EPERM;
	return -1;
}
