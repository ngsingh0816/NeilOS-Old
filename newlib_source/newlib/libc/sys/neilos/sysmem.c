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
#include <sys/mman.h>

extern unsigned int sys_errno();

extern unsigned int	sys_brk(void* addr);
extern void*		sys_sbrk(int offset);
extern void* 		sys_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
extern int			sys_munmap(void* addr, size_t length);
extern int			sys_msync(void* addr, size_t length, int flags);

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

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
	unsigned int ret = (unsigned int)sys_mmap(addr, length, prot, flags, fd, offset);
	if (ret > (unsigned int)(-__ELASTERROR)) {
		errno = -ret;
		return (void*)-1;
	}
	return (void*)ret;
}

int munmap(void* addr, size_t length) {
	int ret = sys_munmap(addr, length);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int msync(void *addr, size_t length, int flags) {
	int ret = sys_msync(addr, length, flags);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}
