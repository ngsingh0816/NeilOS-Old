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

extern unsigned int sys_errno();
extern unsigned int sys_sleep(unsigned int seconds);

int sleep(int seconds) {
	int ret = sys_sleep(seconds);
	if (ret == -1)
		errno = sys_errno();
	return ret;
}
