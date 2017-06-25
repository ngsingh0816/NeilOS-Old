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

extern unsigned int sys_kill(int pid, unsigned int sig);
extern unsigned int sys_signal(unsigned int signum, void (*handler)());
extern unsigned int sys_sigsetmask(unsigned int signum, unsigned char masked);
extern unsigned int sys_siggetmask(unsigned int signum);

int kill(int pid, int sig) {
	return sys_kill(pid, sig);
}

int signal(int sig, void (*handler)()) {
	sys_signal(sig, handler);
}

int sigsetmask(unsigned int signum, unsigned char masked) {
	return sys_sigsetmask(signum, masked);
}

int siggetmask(unsigned int signum) {
	return sys_siggetmask(signum);
}
