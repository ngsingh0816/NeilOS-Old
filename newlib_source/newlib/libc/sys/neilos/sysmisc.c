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
#include <sys/resource.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <stdio.h>
#include <regex.h>
#include <pwd.h>
#include <string.h>

extern unsigned int sys_open(const char* filename, unsigned int mode);
extern unsigned int sys_close(int fd);
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
extern unsigned int sys_fpathconf(int fd, int name);

long sysconf(int name) {
	long ret = sys_sysconf(name);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

long pathconf(const char* path, int name) {
	int fd = sys_open(path, O_RDONLY + 1);
    if (fd < 0) {
        errno = -fd;
        return -1;
    }
	int ret = fpathconf(fd, name);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	
    int ret2 = sys_close(fd);
    if (ret2 < 0) {
        errno = -ret2;
        return -1;
    }
	
	return ret;
}

long fpathconf(int fd, int name) {
	long ret = sys_fpathconf(fd, name);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
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

int setreuid(uid_t ruid, uid_t euid) {
	return 0;
}

int setregid(gid_t rgid, uid_t egid) {
	return 0;
}

struct group* getgrnam(const char* name) {
	return NULL;
}

struct group* getgrgid(gid_t gid) {
	return NULL;
}

int getgroups(int size, gid_t* list) {
	return 0;
}

int setgroups(int size, const gid_t* list) {
	return 0;
}

struct group* getgrent(void) {
	return NULL;
}

void setgrent(void) {
}

void endgrent(void) {
}

char* getlogin(void) {
	return NULL;
}

struct passwd* getpwnam(const char* name) {
	return NULL;
}

struct passwd* getpwuid(uid_t uid) {
	return NULL;
}

// Resources / limits
int	getpriority(int which, int who) {
	printf("getpriority used.\n");
	return -1;
}

int	setpriority(int which, int who, int prio) {
	printf("setpriority used.\n");
	return -1;
}

int	getrlimit(int resource, struct rlimit* rlim) {
	printf("getrlimit used.\n");
	return -1;
}

int	setrlimit(int resource, const struct rlimit* rlim) {
	printf("setrlimit used.\n");
	return -1;
}

int	getrusage(int who, struct rusage* usage) {
	printf("getrusageint used.\n");
	return -1;
}

// System info
int uname(struct utsname *name) {
	strcpy(name->sysname, "NeilOS");
	strcpy(name->nodename, "");
	strcpy(name->release, "0.1.0");
	strcpy(name->version, "0.1.0");
	strcpy(name->machine, "i686");
	
	return 0;
}
