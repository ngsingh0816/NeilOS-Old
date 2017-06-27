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

typedef struct {
	unsigned int high;
	unsigned int low;
} __sys_uint64_t;

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
extern unsigned int sys_llseek(int fd, __sys_uint64_t offset, int whence);
extern unsigned int sys_truncate(int fd, __sys_uint64_t length);
extern unsigned int sys_stat(int fd, __sys_stat_type* data);
extern unsigned int sys_close(int fd);
extern unsigned int sys_isatty(int fd);
extern unsigned int sys_dup(unsigned int fd);
extern unsigned int sys_dup2(unsigned int fd, unsigned int new_fd);
extern unsigned int sys_pipe(int pipefd[2]);

int open(const char *name, int flags, ...) {
	return sys_open(name, flags);
}

int read(int file, char *ptr, int len) {
	return sys_read(file, ptr, len);
}

int write(int file, char *ptr, int len) {
	return sys_write(file, ptr, len);
}

int lseek(int file, int ptr, int dir) {
	__sys_uint64_t offset;
	offset.low = ptr;
	return sys_llseek(file, offset, dir);
}

int truncate(int fd, unsigned int length) {
	__sys_uint64_t len;
	len.low = length;
	return sys_truncate(fd, len);
}

int stat(const char *file, struct stat *st) {
	int fd = sys_open(file, O_RDONLY);
	if (fd == -1)
		return -1;
	int ret = fstat(fd, st);
	sys_close(fd);
	return ret;
}

int fstat(int file, struct stat *st) {
	if (!st)
		return -1;
	
	__sys_stat_type data;
	int ret = sys_stat(file, &data);
	
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

int close(int file) {
	return sys_close(file);
}

int isatty(int file) {
	return sys_isatty(file);
}

int dup(int fd) {
	return sys_dup(fd);
}

int dup2(int fd, int new_fd) {
	return sys_dup2(fd, new_fd);
}

int pipe(int pipefd[2]) {
	return sys_pipe(pipefd);
}
