//
//  NSLog.h
//  product
//
//  Created by Neil Singh on 1/29/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSLOG_H
#define NSLOG_H

#include <stdio.h>

// TODO: make this real
#define NSLog(x, ...)	printf((x), ## __VA_ARGS__)

#endif /* NSLOG_H */
