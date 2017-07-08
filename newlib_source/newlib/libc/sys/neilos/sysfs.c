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
#include <utime.h>
#include <sys/time.h>
#include <sys/stat.h>

extern unsigned int sys_errno();

extern unsigned int sys_mkdir(const char* name);
extern unsigned int sys_link(const char* filename, const char* new_name);
extern unsigned int sys_unlink(const char* filename);
extern unsigned int sys_utime(const char* filename, unsigned int* times);

// Helpers
extern unsigned int sys_open(const char* filename, unsigned int mode);
extern unsigned int sys_close(int fd);
extern int fstat(int file, struct stat* st);

int mkdir(const char* name, mode_t mode) {
	int ret = sys_mkdir(name);
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

int link(char* old, char* new) {
	int ret = sys_link(old, new);
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

int unlink(char* name) {
	int ret = sys_unlink(name);
	if (ret == -1)
		errno = sys_errno();
	return ret;
}

int rmdir(const char* path) {
	int fd = sys_open(path, O_RDONLY + 1);
	if (fd == -1) {
		errno = sys_errno();
		return -1;
	}
	
	struct stat buf;
	if (fstat(fd, &buf) != 0) {
		errno = sys_errno();
		return -1;
	}
	if (!(buf.st_mode & _IFDIR)) {
		errno = ENOTDIR;
		return -1;
	}
	if (sys_close(fd) != 0) {
		errno = sys_errno();
		return -1;
	}
	
	int ret = sys_unlink(path);
	if (ret != 0)
		errno = sys_errno();
	return ret;
}

int chmod(const char* path, mode_t mode) {
	// Permissions aren't supported
	return 0;
}

int fchmod(int fd, mode_t mode) {
	// Permissions aren't supported
	return 0;
}

int chown(const char* path, uid_t owner, gid_t group) {
	// Users / Groups aren't supported
	return 0;
}

int fchown(int fd, uid_t owner, gid_t group) {
	// Users / Groups aren't supported
	return 0;
}

int lchown(const char* path, uid_t owner, gid_t group) {
	// Users / Groups aren't supported
	return 0;
}

int access(const char* path, int mode) {
	// Permissions aren't supported, so just check if the file exists
	if (mode == 0)
		return 0;
	int fd = sys_open(path, O_RDONLY + 1);
	if (fd == -1) {
		errno = sys_errno();
		return -1;
	}
	if (sys_close(fd) != 0) {
		errno = sys_errno();
		return -1;
	}
	return 0;
}

mode_t umask(mode_t mask) {
	// Permissions aren't supported to just return octal 022
	return 0x12;
}

int utime(const char* filename, const struct utimbuf* times) {
	unsigned int buf[2];
	int ret = 0;
	if (times) {
		buf[0] = times->actime;
		buf[1] = times->modtime;
		ret = sys_utime(filename, buf);
	} else
		ret = sys_utime(filename, NULL);
	if (ret != 0)
		errno = sys_errno();
	return ret;
}
