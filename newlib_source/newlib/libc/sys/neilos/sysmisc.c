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
#include <unistd.h>
#include <regex.h>

extern unsigned int sys_errno();

// We must stub out the following functions so that dynamic linking does
// not give errors about undefined functions (because newlib itself references them)
int getentropy(void* buffer, size_t length) {
	return -1;
}

int regcomp(regex_t* preg, const char* regex, int cflags) {
	return -1;
}

int regexec(const regex_t* preg, const char* string, size_t nmatch,
			regmatch_t pmatch[], int eflags) {
	return -1;
}

void regfree(regex_t* preg) {
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
	return -1;
}

// Syscalls
extern unsigned int sys_sysconf(int name);

long sysconf(int name) {
	long ret = sys_sysconf(name);
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

// Groups / Users / Permissions
// These are all not supported, so do nothing for them

pid_t tcgetpgrp(int fd) {
	return 0;
}

int tcsetpgrp(int fd, pid_t pgrp) {
	return 0;
}

pid_t getpgrp(void) {
	return 0;
}

pid_t setpgrp(void) {
	return 0;
}

pid_t getpgid(pid_t pid) {
	return 0;
}

int setpgid(pid_t pid, pid_t pgid) {
	return 0;
}

uid_t getuid(void) {
	return 0;
}

uid_t geteuid(void) {
	return 0;
}

gid_t getgid(void) {
	return 0;
}

gid_t getegid(void) {
	return 0;
}

int getgroups(int size, gid_t* list) {
	return 0;
}

int setgroups(int size, const gid_t* list) {
	return 0;
}
