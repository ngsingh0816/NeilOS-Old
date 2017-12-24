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
extern unsigned int sys_sigsetmask(unsigned int mask);
extern unsigned int sys_siggetmask();
extern unsigned int sys_sigprocmask(int how, const sigset_t* set, sigset_t* oldset);
extern unsigned int sys_sigaction(int signum, const struct sigaction* act, struct sigaction* oldact);
extern unsigned int sys_sigsuspend(const sigset_t* mask);
extern unsigned int sys_alarm(int seconds);

int kill(int pid, int sig) {
	int ret = sys_kill(pid, sig);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
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
    if (ret < 0) {
        errno = -ret;
        return SIG_ERR;
    }
	return old.sa_handler;
}

int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact) {
	int ret = sys_sigaction(signum, act, oldact);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int sigsetmask(unsigned int mask) {
	int ret = sys_sigsetmask(mask);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int siggetmask() {
	int ret = sys_siggetmask();
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int sigprocmask(int how, const sigset_t* set, sigset_t* oldset) {
	int ret = sys_sigprocmask(how, set, oldset);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int sigsuspend(const sigset_t* mask) {
	int ret = sys_sigsuspend(mask);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int alarm(unsigned int seconds) {
	int ret = sys_alarm(seconds);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}
