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

extern unsigned int sys_times(struct tms* buf);
extern unsigned int sys_gettimeofday();

clock_t times(struct tms *buf) {
	return sys_times(buf);
}

int gettimeofday(struct timeval* p, void* z) {
	if (!p)
		return -1;
	
	p->tv_sec = sys_gettimeofday();
	
	return 0;
}
