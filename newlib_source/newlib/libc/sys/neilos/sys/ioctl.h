//
//  ioctl.h
//  NeilOS
//
//  Created by Neil Singh on 11/23/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#ifdef __cplusplus
extern "C" {
#endif
	
#define TIOCGWINSZ			5
	
struct winsize {
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel;
	unsigned short ws_ypixel;
};

int ioctl(int fd, int request, ...);
	
#ifdef __cplusplus
}
#endif

#endif /* ioctl_h */
