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

extern void*		sys_brk(void* addr);
extern void*		sys_sbrk(int offset);

caddr_t brk(void* b) {
	return sys_brk(b);
}

caddr_t sbrk(int incr) {
	return sys_sbrk(incr);
}
