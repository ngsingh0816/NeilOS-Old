//
//  syssignal.c
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

extern unsigned int sys_kill(int pid, unsigned int sig);
extern unsigned int sys_signal(unsigned int signum, void (*handler)());
extern unsigned int sys_sigsetmask(unsigned int signum, unsigned char masked);
extern unsigned int sys_siggetmask(unsigned int signum);

int kill(int pid, int sig) {
	int ret = sys_kill(pid, sig);
	if (ret == -1)
		errno = sys_errno();
	return -1;
}

int signal(int sig, void (*handler)()) {
	int ret = sys_signal(sig, handler);
	if (ret == -1)
		errno = sys_errno();
	return -1;
}

int sigsetmask(unsigned int signum, unsigned char masked) {
	int ret = sys_sigsetmask(signum, masked);
	if (ret == -1)
		errno = sys_errno();
	return -1;
}

int siggetmask(unsigned int signum) {
	int ret = sys_siggetmask(signum);
	if (ret == -1)
		errno = sys_errno();
	return -1;
}
