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

int ioctl(int fd, int request, ...);
	
#ifdef __cplusplus
}
#endif

#endif /* ioctl_h */
