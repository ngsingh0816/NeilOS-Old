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

extern unsigned int sys_fork();
extern unsigned int sys_execve(char* filename, char** argv, char** envp);
extern unsigned int sys_getpid();
extern unsigned int sys_waitpid(unsigned int pid);
extern unsigned int sys_wait();
extern unsigned int sys_exit(int status);

int fork() {
	return sys_fork();
}

int execve(char *name, char **argv, char **env) {
	return sys_execve(name, argv, env);
}

int getpid() {
	return sys_getpid();
}

int waitpid(unsigned int pid) {
	return sys_waitpid(pid);
}

int wait(int *status) {
	return sys_wait();
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
