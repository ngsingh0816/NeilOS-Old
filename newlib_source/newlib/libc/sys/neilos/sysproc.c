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
#include <sys/resource.h>
#include <sys/vfs.h>
#include <stdlib.h>
#include <string.h>

extern char** environ;

extern unsigned int sys_errno();

extern unsigned int sys_fork();
extern unsigned int sys_execve(const char* filename, char* const argv[], char** envp);
extern unsigned int sys_getpid();
extern unsigned int sys_getppid();
extern unsigned int sys_waitpid(unsigned int pid, int* status, int options);
extern unsigned int sys_exit(int status);
extern unsigned int sys_getwd(char* buf);
extern unsigned int sys_chdir(const char* path);

int fork() {
	int ret = sys_fork();
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

int execve(const char *name, char **argv, char **env) {
	int ret = sys_execve(name, argv, env);
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

int execv(const char* name, char **argv) {
	return execve(name, argv, environ);
}

int execl(const char* path, const char* arg, ...) {
	int num = 1;
	char** esp = (char**)&arg;
	while (*esp != NULL) {
		esp++;
		num++;
	}
	char** list = malloc(sizeof(char*) * num);
	if (!list) {
		errno = ENOMEM;
		return -1;
	}
	esp = (char**)&arg;
	for (int z = 0; z < num; z++)
		list[z] = *(esp++);
	
	int ret = execv(path, list);
	free(list);
	
	return ret;
}

int execvp(const char* name, char* const argv[]) {
	char* pathc = getenv("PATH");
	if (!pathc) {
		errno = ENOENT;
		return -1;
	}
	int psize = strlen(pathc);
	char* path = malloc(psize + 1);
	if (!path) {
		errno = ENOMEM;
		return -1;
	}
	memcpy(path, pathc, psize + 1);
	
	int ret = sys_execve(name, argv, environ);
	if (ret != -1)
		return ret;
	char* str = NULL;
	uint32_t name_len = strlen(name);
	while ((str = strsep(&path, ":")) != NULL) {
		uint32_t len = strlen(str);
		char* buffer = malloc(len + name_len + 2);
		if (!buffer) {
			free(path);
			return ENOMEM;
		}
		memcpy(buffer, str, len);
		buffer[len] = '/';
		memcpy(&buffer[len + 1], name, name_len);
		buffer[len + name_len + 1] = 0;
		ret = sys_execve(buffer, argv, environ);
		free(buffer);
		if (ret != -1) {
			free(path);
			return ret;
		}
	}
	
	free(path);
	return ENOENT;
}

int execlp(const char* path, const char* arg, ...) {
	int num = 1;
	char** esp = (char**)&arg;
	while (*esp != NULL) {
		esp++;
		num++;
	}
	char** list = malloc(sizeof(char*) * num);
	if (!list) {
		errno = ENOMEM;
		return -1;
	}
	esp = (char**)&arg;
	for (int z = 0; z < num; z++)
		list[z] = *(esp++);
	
	int ret = execvp(path, list);
	free(list);
	
	return ret;
}

int getpid() {
	int ret = sys_getpid();
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

int getppid() {
	int ret = sys_getppid();
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

int waitpid(unsigned int pid, int* status, int options) {
	int ret = sys_waitpid(pid, status, options);
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

int wait(int* status) {
	return waitpid(-1, status, 0);
}

pid_t wait3(int* status, int options, struct rusage* rusage) {
	// TODO: implement the rusage part (most likely for "time command")
	return waitpid(-1, status, options);
}

pid_t wait4(pid_t pid, int* status, int options, struct rusage* rusage) {
	// TODO: implement the rusage part (most likely for "time command")
	return waitpid(pid, status, options);
}

extern void (*__fini_array_start []) (void) __attribute__((weak));
extern void (*__fini_array_end []) (void) __attribute__((weak));
extern void _fini();
extern void __cxa_finalize(void* d);

void _exit(int status) {
	// Perform deconstructors
	__cxa_finalize(NULL);
	unsigned int count = __fini_array_end - __fini_array_start;
	for (unsigned int z = 0; z < count; z++)
		__fini_array_start[z]();
	_fini();
	
	sys_exit(status);
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

char* getcwd(char* buf, size_t size) {
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
}

int chdir(const char* path) {
	int ret = sys_chdir(path);
	if (ret == -1)
		errno = sys_errno();
	return ret;
}
