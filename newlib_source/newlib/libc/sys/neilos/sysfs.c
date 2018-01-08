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
#include <sys/dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "include/mntent.h"
#include <sys/vfs.h>

extern unsigned int sys_errno();

extern unsigned int sys_mkdir(const char* name);
extern unsigned int sys_link(const char* filename, const char* new_name);
extern unsigned int sys_unlink(const char* filename, char dir, int type);
extern unsigned int sys_readdir(int fd, void* buf, int size, struct dirent* dirent);
extern unsigned int sys_utime(const char* filename, unsigned int* times);

// Helpers
extern unsigned int sys_open(const char* filename, unsigned int mode, unsigned int type);
extern unsigned int sys_close(int fd);

int mkdir(const char* name, mode_t mode) {
	int ret = sys_mkdir(name);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int link(const char* old, const char* new) {
	int ret = sys_link(old, new);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int unlink(const char* name) {
	int ret = sys_unlink(name, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int rmdir(const char* path) {
	int ret = sys_unlink(path, 1, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
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
	int fd = sys_open(path, O_RDONLY + 1, 0);
    if (fd < 0) {
        errno = -fd;
        return -1;
    }
    int ret2 = sys_close(fd);
    if (ret2 < 0) {
        errno = -ret2;
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
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

DIR *opendir(const char* filename) {
	if (!filename) {
		errno = ENOENT;
		return NULL;
	}
	
	DIR* ret = malloc(sizeof(DIR));
	if (!ret) {
		errno = ENOMEM;
		return NULL;
	}
	memset(ret, 0, sizeof(DIR));
	ret->dd_buf = malloc(256);
	if (!ret->dd_buf) {
		free(ret);
		errno = ENOMEM;
		return NULL;
	}
	ret->dd_size = 256;
		
	int fd = sys_open(filename, O_RDONLY, 0);
	if (fd < 0) {
		errno = ENOENT;
		free(ret->dd_buf);
		free(ret);
		return NULL;
	}
	
	struct stat st;
	if (fstat(fd, &st) < 0 || !(st.st_mode & _IFDIR)) {
		errno = ENOTDIR;
		free(ret->dd_buf);
		free(ret);
		return NULL;
	}
	
	ret->dd_fd = fd;
	return ret;
}

struct dirent dirent_rd;
struct dirent* readdir(DIR* dir) {
	dir->dd_len = sys_readdir(dir->dd_fd, dir->dd_buf, dir->dd_size, &dirent_rd);
	if ((int)dir->dd_len < 0) {
		errno = -(int)dir->dd_len;
		return NULL;
	} else if (dir->dd_len == 0)
		return NULL;
	memcpy(dirent_rd.d_name, dir->dd_buf, dir->dd_len);
	dirent_rd.d_name[dir->dd_len] = 0;

	return &dirent_rd;
}

void rewinddir(DIR* dir) {
	seekdir(dir, 0);
}

long telldir(DIR* dir) {
	if (!dir) {
		errno = EBADF;
		return -1;
	}
	
	return lseek(dir->dd_fd, 0, SEEK_CUR);
}

void seekdir(DIR * dir, off_t loc) {
	if (!dir)
		return;
	
	lseek(dir->dd_fd, loc, SEEK_SET);
}

int closedir(DIR * dir) {
	if (!dir) {
		errno = EBADF;
		return -1;
	}
	
	int ret = sys_close(dir->dd_fd);
    if (ret < 0) {
        errno = -ret;
        free(dir->dd_buf);
        free(dir);
        return -1;
    }
	
	free(dir->dd_buf);
	free(dir);
	return ret;
}

// Mount things
FILE* setmntent(const char* filename, const char* type) {
	return NULL;
}

struct mntent* getmntent(FILE* fp) {
	return NULL;
}

int endmntent(FILE* filep) {
	if (filep)
		fclose(filep);
	return 1;
}

int statfs(const char* path , struct statfs* buf) {
	errno = ENOSYS;
	return -1;
	/*int fd = sys_open(file, O_RDONLY + 1, 0);
    if (fd < 0) {
        errno = -fd;
        return -1;
    }
	int ret = fstatfs(fd, buf);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	
    int ret2 = sys_close(fd)
    if (ret2 < 0) {
        errno = -ret2;
        return -1;
    }
	
	return ret;*/
}

int fstatfs(int fd, struct statfs* buf) {
	errno = ENOSYS;
	return -1;
}

