//
//  syssockets.c
//  
//
//  Created by Neil Singh on 11/23/17.
//
//

#include <sys/select.h>
#include <sys/time.h>
#include <stdio.h>

int select (int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout) {
	printf("Select used.\n");
	return 0;
}
