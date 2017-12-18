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
#include <unistd.h>

extern unsigned int sys_errno();
extern unsigned int sys_sleep(unsigned int seconds);

unsigned int sleep(unsigned int seconds) {
	int ret = sys_sleep(seconds);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int usleep(useconds_t micros) {
	// TODO: actually implement this
	return sleep(micros / 1000000);
}
