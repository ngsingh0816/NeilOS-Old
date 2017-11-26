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

speed_t	cfgetispeed(const struct termios* termios_p) {
	printf("cfgetisspeed() used.\n");
	return 0;
}

speed_t	cfgetospeed(const struct termios* termios_p) {
	printf("cfgetosspeed() used.\n");
	return 0;
}

int	cfsetispeed(struct termios* termios_p, speed_t speed) {
	printf("cfsetisspeed() used.\n");
	return 0;
}

int	cfsetospeed(struct termios* termios_p, speed_t speed) {
	printf("cfsetosspeed() used.\n");
	return 0;
}

int	tcdrain(int fd) {
	printf("tcdrain() used.\n");
	return 0;
}

int	tcflow(int fd, int action) {
	printf("tcflow() used.\n");
	return 0;
}

int	tcflush(int fd, int queue_selector) {
	printf("tcflush() used.\n");
	return 0;
}

int	tcgetattr(int fd, struct termios* termios_p) {
	printf("tcgetattr() used.\n");
	return 0;
}

int	tcsendbreak(int fd, int duration) {
	printf("tcsendbreak() used.\n");
	return 0;
}

int	tcsetattr(int fd, int optional_actions, const struct termios* termios_p) {
	printf("tcsetattr() used.\n");
	return 0;
}

char* ttyname(int fd) {
	if (isatty(fd))
		return "/dev/tty";
	errno = ENOTTY;
	return NULL;
}
