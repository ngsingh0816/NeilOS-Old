//
//  sysproc.c
//  
//
//  Created by Neil Singh on 6/25/17.
//
//

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <stdlib.h>

extern unsigned int sys_errno();

extern unsigned int sys_fork();
extern unsigned int sys_execve(char* filename, char** argv, char** envp);
extern unsigned int sys_getpid();
extern unsigned int sys_waitpid(unsigned int pid);
extern unsigned int sys_wait();
extern unsigned int sys_exit(int status);
extern unsigned int sys_getwd(char* buf);

int fork() {
	int ret = sys_fork();
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

int execve(char *name, char **argv, char **env) {
	int ret = sys_execve(name, argv, env);
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

int getpid() {
	int ret = sys_getpid();
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

int waitpid(unsigned int pid) {
	int ret = sys_waitpid(pid);
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

int wait(int *status) {
	int ret = sys_wait();
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

extern void (*__fini_array_start []) (void) __attribute__((weak));
extern void (*__fini_array_end []) (void) __attribute__((weak));
extern void _fini();
extern void __cxa_finalize(void* d);

void _exit() {
	// Perform deconstructors
	__cxa_finalize(NULL);
	unsigned int count = __fini_array_end - __fini_array_start;
	for (unsigned int z = 0; z < count; z++)
		__fini_array_start[z]();
	_fini();
	
	sys_exit(0);
}

char* getwd(char* buf) {
	if (!buf)
		return NULL;
	
	if (sys_getwd(buf) == -1) {
		errno = sys_errno();
		return NULL;
	}
	return buf;
}

/*char* getcwd(char* buf, size_t size) {
	if (!buf || size == 0) {
		buf = malloc(4096);	// PATH_MAX on linux
		if (!buf)
			return NULL;
	}
	
	if (sys_getwd(buf) == -1) {
		errno = sys_errno();
		return NULL;
	}
	return buf;
}*/


