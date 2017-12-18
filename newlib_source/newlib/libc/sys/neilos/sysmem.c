//
//  sysmem.c
//  
//
//  Created by Neil Singh on 6/25/17.
//
//

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/errno.h>

extern unsigned int sys_errno();

extern unsigned int	sys_brk(void* addr);
extern void*		sys_sbrk(int offset);

unsigned int brk(void* b) {
	int ret = sys_brk(b);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

caddr_t sbrk(int incr) {
	void* ret = sys_sbrk(incr);
    if ((int)ret == -1) {
        errno = ENOMEM;
        return NULL;
    }
	return ret;
}
