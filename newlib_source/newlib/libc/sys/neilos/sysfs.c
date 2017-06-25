//
//  sysfs.c
//  
//
//  Created by Neil Singh on 6/25/17.
//
//

#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/errno.h>

extern unsigned int sys_mkdir(const char* name);
extern unsigned int sys_link(const char* filename, const char* new_name);
extern unsigned int sys_unlink(const char* filename);

int mkdir(const char* name, mode_t mode) {
	return sys_mkdir(name);
}

int link(char *old, char *new) {
	return sys_link(old, new);
}

int unlink(char *name) {
	return sys_unlink(name);
}
