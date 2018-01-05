//
//  systermios.c
//  
//
//  Created by Neil Singh on 11/23/17.
//
//

#include <sys/termios.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

// Various IOCTL
#define TCFLUSH			0
#define TCFLOW			1
#define TCGETATTR		2
#define TCSETATTR		3
#define TTYNAME			4

speed_t	cfgetispeed(const struct termios* termios_p) {
	return termios_p->c_ispeed;
}

speed_t	cfgetospeed(const struct termios* termios_p) {
	return termios_p->c_ospeed;
}

int	cfsetispeed(struct termios* termios_p, speed_t speed) {
	if (speed == 0)
		speed = termios_p->c_ospeed;
	termios_p->c_ispeed = speed;
	return 0;
}

int	cfsetospeed(struct termios* termios_p, speed_t speed) {
	termios_p->c_ospeed = speed;
	return 0;
}

int	tcdrain(int fd) {
	// Waits for the data to be transferred to the device, but this is instantaneous so just return
	return 0;
}

int	tcflow(int fd, int action) {
	int ret = ioctl(fd, TCFLOW, action);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int	tcflush(int fd, int queue_selector) {
	int ret = ioctl(fd, TCFLUSH, queue_selector);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int	tcgetattr(int fd, struct termios* termios_p) {
	int ret = ioctl(fd, TCGETATTR, termios_p);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int	tcsetattr(int fd, int optional_actions, const struct termios* termios_p) {
	int ret = ioctl(fd, TCSETATTR, termios_p, optional_actions);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int	tcsendbreak(int fd, int duration) {
	// Send break has no function on pseduoterminals
	return 0;
}

char* ttyname(int fd) {
	if (isatty(fd)) {
		static char buf[256];
		int ret = ioctl(fd, TTYNAME, buf);
		if (ret < 0) {
			errno = -ret;
			return NULL;
		}
		return buf;
	}
	errno = ENOTTY;
	return NULL;
}
