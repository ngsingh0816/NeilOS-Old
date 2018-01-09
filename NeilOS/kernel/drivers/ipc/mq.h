//
//  mq.h
//  product
//
//  Created by Neil Singh on 1/7/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef MQ_H
#define MQ_H

#include <common/types.h>
#include <syscalls/descriptor.h>
#include <program/signal.h>

#define MQ_IOCTL_SEND		1
#define MQ_IOCTL_RECEIVE	2
#define MQ_IOCTL_GETATTR	3
#define MQ_IOCTL_SETATTR	4

#define MQ_PRIO_MAX 		32

typedef struct {
	long mq_flags;    /* message queue flags */
	long mq_maxmsg;   /* maximum number of messages */
	long mq_msgsize;  /* maximum message size */
	long mq_curmsgs;  /* number of messages currently queued */
} mq_attr_t;

// Open a message queue
file_descriptor_t* mq_open(const char* filename, uint32_t mode);

// Read a message (no priority information)
uint32_t mq_read(file_descriptor_t* f, void* buf, uint32_t bytes);

// Write a message with default priority
uint32_t mq_write(file_descriptor_t* f, const void* buf, uint32_t bytes);

// Send a message
uint32_t mq_send(file_descriptor_t* f, const void* buf, uint32_t bytes, uint32_t priority);

// Receive a message
uint32_t mq_receive(file_descriptor_t* f, void* buf, uint32_t bytes, uint32_t* priority);

// Get and set attributes
uint32_t mq_getattr (file_descriptor_t* f, mq_attr_t* attr);
uint32_t mq_setattr (file_descriptor_t* f, const mq_attr_t* attr, mq_attr_t* old);

// Ioctl
uint32_t mq_ioctl(file_descriptor_t* f, int request, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);

// Get info
uint32_t mq_stat(file_descriptor_t* f, sys_stat_type* data);

// Can read or write a message
bool mq_can_read(file_descriptor_t* f);
bool mq_can_write(file_descriptor_t* f);

// Duplicate a message queue
file_descriptor_t* mq_duplicate(file_descriptor_t* f);

// Close a message queue
uint32_t mq_close(file_descriptor_t* f);

// Unlink a message queue
uint32_t mq_unlink(const char* filename);

#endif /* MQ_H */
