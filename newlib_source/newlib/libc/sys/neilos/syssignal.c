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
#include <sys/signal.h>
#include <string.h>

extern unsigned int sys_errno();

extern unsigned int sys_kill(int pid, unsigned int sig);
extern unsigned int sys_sigsetmask(unsigned int signum, unsigned char masked);
extern unsigned int sys_siggetmask(unsigned int signum);
extern unsigned int sys_sigprocmask(int how, const sigset_t* set, sigset_t* oldset);
extern unsigned int sys_sigaction(int signum, const struct sigaction* act, struct sigaction* oldact);
extern unsigned int sys_sigsuspend(const sigset_t* mask);
extern unsigned int sys_alarm(int seconds);

int kill(int pid, int sig) {
	int ret = sys_kill(pid, sig);
	if (ret == -1)
		errno = sys_errno();
	return -1;
}

int killpg(int pgrp, int sig) {
	// Process groups aren't supported
	errno = ESRCH;
	return -1;
}

_sig_func_ptr signal(int sig, _sig_func_ptr handler) {
	
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = handler;
	struct sigaction old;
	old.sa_handler = NULL;
	int ret = sys_sigaction(sig, &act, &old);
	if (ret == -1)
		errno = sys_errno();
	return old.sa_handler;
}

int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact) {
	int ret = sys_sigaction(signum, act, oldact);
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

int sigprocmask(int how, const sigset_t* set, sigset_t* oldset) {
	int ret = sys_sigprocmask(how, set, oldset);
	if (ret == -1)
		errno = sys_errno();
	return -1;
}

int sigsuspend(const sigset_t* mask) {
	int ret = sys_sigsuspend(mask);
	if (ret == -1)
		errno = sys_errno();
	return -1;
}

int alarm(unsigned int seconds) {
	int ret = sys_alarm(seconds);
	if (ret == -1)
		errno = sys_errno();
	return -1;
}
