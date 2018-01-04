//
//  sysfile.c
//  
//
//  Created by Neil Singh on 6/25/17.
//
//

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/signal.h>

typedef struct {
	uint32_t dev_id;
	uint32_t inode;
	uint32_t mode;
	uint16_t num_links;
	uint32_t size;
	uint32_t block_size;
	uint32_t num_512_blocks;
	
	time_t atime;
	time_t mtime;
	time_t ctime;
} __sys_stat_type;

extern unsigned int sys_open(const char* filename, unsigned int mode);
extern unsigned int sys_read(int fd, void* buf, unsigned int nbytes);
extern unsigned int sys_write(int fd, const void* buf, unsigned int nbytes);
extern unsigned int sys_llseek(int fd, uint32_t offset_high, uint32_t offset_low, int whence);
extern unsigned int sys_truncate(int fd, uint32_t length_high, uint32_t length_low);
extern unsigned int sys_stat(int fd, __sys_stat_type* data);
extern unsigned int sys_close(int fd);
extern unsigned int sys_isatty(int fd);
extern unsigned int sys_dup(unsigned int fd);
extern unsigned int sys_dup2(unsigned int fd, unsigned int new_fd);
extern unsigned int sys_pipe(int pipefd[2]);
extern unsigned int sys_fcntl(int fd, int cmd, ...);
extern unsigned int sys_ioctl(int fd, int cmd, ...);
extern unsigned int sys_select(int nfds, fd_set* readfds, fd_set* writefds,
							   fd_set* exceptfds, struct timeval* timeout);

int open(const char* name, int mode, ...) {
	int ret = sys_open(name, mode + 1);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int _open(const char* name, int mode, ...) {
	return open(name, mode);
}

int read(int file, char* ptr, int len) {
	int ret = sys_read(file, ptr, len);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int write(int file, char* ptr, int len) {
	int ret = sys_write(file, ptr, len);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int lseek(int file, int ptr, int dir) {
	int ret = sys_llseek(file, (ptr < 0) ? -1 : 0, ptr, dir);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int ftruncate(int fd, unsigned int length) {
	int ret = sys_truncate(fd, 0, length);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int truncate(const char* path, unsigned int length) {
	int fd = sys_open(path, O_RDONLY + 1);
    if (fd < 0) {
        errno = -fd;
        return -1;
    }
	int ret = ftruncate(fd, length);
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

int stat(const char* file, struct stat* st) {
	int fd = sys_open(file, O_RDONLY | 0x1 | _FNONBLOCK);
    if (fd < 0) {
        errno = -fd;
        return -1;
    }
	int ret = fstat(fd, st);
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

int fstat(int file, struct stat *st) {
    if (!st) {
        errno = EFAULT;
		return -1;
    }
	
	__sys_stat_type data;
	memset(&data, 0, sizeof(__sys_stat_type));
	int ret = sys_stat(file, &data);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	
	memset(st, 0, sizeof(struct stat));
	st->st_dev = data.dev_id;
	st->st_ino = data.inode;
	st->st_mode = data.mode;
	st->st_nlink = data.num_links;
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_rdev = 0;
	st->st_size = data.size;
	st->st_atime = data.atime;
	st->st_ctime = data.ctime;
	st->st_mtime = data.mtime;
	st->st_blksize = data.block_size;
	st->st_blocks = data.num_512_blocks;
	
	return ret;
}

int fsync(int fd) {
	// TOOD: come back to this if we ever buffer anything
	return 0;
}

int fdatasync(int fd) {
	// TOOD: come back to this if we ever buffer anything
	return 0;
}

int lstat(const char* file, struct stat* st) {
	// Symbolic links aren't supported
	return stat(file, st);
}

int close(int file) {
	int ret = sys_close(file);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int _close(int file) {
	return close(file);
}

int isatty(int file) {
	int ret = sys_isatty(file);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int dup(int fd) {
	int ret = sys_dup(fd);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int dup2(int fd, int new_fd) {
	int ret = sys_dup2(fd, new_fd);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int pipe(int pipefd[2]) {
	int ret = sys_pipe(pipefd);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int mkfifo(const char* filename, unsigned int mode) {
	// Read = 0x1
	int fd = sys_open(filename, 0x1 | _IFIFO | _FCREAT | _FNONBLOCK);
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

int fcntl(int fd, int cmd, ...) {
	uint32_t* esp = ((uint32_t*)&cmd) + 1;
	int ret = sys_fcntl(fd, cmd, *esp, *(esp + 1));
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int _fcntl(int fd, int cmd, ...) {
	uint32_t* esp = ((uint32_t*)&cmd) + 1;
	return fcntl(fd, cmd, *esp, *(esp + 1));
}

int ioctl(int fd, int request, ...) {
	uint32_t* esp = ((uint32_t*)&request) + 1;
	int ret = sys_ioctl(fd, request, *esp, *(esp + 1));
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
	return ret;
}

int select (int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout) {
	int ret = sys_select(nfds, readfds, writefds, exceptfds, timeout);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int pselect (int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
			 const struct timespec* timeout, const sigset_t* sigmask) {
	sigset_t origmask;
	struct timeval val;
	val.tv_sec = timeout->tv_sec;
	val.tv_usec = timeout->tv_nsec / 1000;
	sigprocmask(SIG_SETMASK, sigmask, &origmask);
	int ready = select(nfds, readfds, writefds, exceptfds, &val);
	sigprocmask(SIG_SETMASK, &origmask, NULL);
	return ready;
	
}
