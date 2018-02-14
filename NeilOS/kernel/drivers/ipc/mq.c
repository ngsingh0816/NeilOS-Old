//
//  mq.c
//  product
//
//  Created by Neil Singh on 1/7/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "mq.h"
#include <syscalls/interrupt.h>

#define DEFAULT_PRIORITY		(MQ_PRIO_MAX / 2)
#define MAX_MESSAGE_LENGTH		4096
#define MAX_NUM_MESSAGES		2048

typedef struct mq_message {
	void* buffer;
	uint32_t length;
	uint32_t priority;
	
	struct mq_message* next;
	struct mq_message* prev;
} mq_message_t;

// Message queue list
typedef struct mq_list {
	char* name;
	mq_attr_t attr;
	// Message list is a linked list of messages sorted by priority. This way, when reading a message,
	// one just has to pop off the first message in the list. When inserting a message, one just has to
	// insert the message at the tail end of its priority section in the list. Because the tail pointers are
	// cached, both read and write will be in O(1) time.
	mq_message_t* messages;
	// Tails of each priority in the message list
	mq_message_t* priority_tails[MQ_PRIO_MAX];
	
	uint32_t num_open;
	bool dealloc_on_close;
	semaphore_t lock;
	
	struct mq_list* next;
	struct mq_list* prev;
} mq_list_t;

mq_list_t* mq_list = NULL;
semaphore_t mq_list_lock = MUTEX_UNLOCKED;

mq_list_t* mq_get(const char* filename, bool allocate, bool increment_open) {
	int name_len = strlen(filename);
	// Find it in this list
	down(&mq_list_lock);
	mq_list_t* t = mq_list;
	if (t)
		down(&t->lock);
	up(&mq_list_lock);
	while (t) {
		if (strncmp(t->name, filename, name_len + 1) == 0) {
			if (increment_open)
				t->num_open++;
			up(&t->lock);
			return t;
		}
		
		mq_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	
	// Nothing was found, allocate if desired
	if (!allocate)
		return NULL;
	
	t = kmalloc(sizeof(mq_list_t));
	if (!t)
		return NULL;
	memset(t, 0, sizeof(mq_list_t));
	t->lock = MUTEX_LOCKED;
	if (increment_open)
		t->num_open = 1;
	t->name = kmalloc(name_len);
	if (!t->name) {
		kfree(t);
		return NULL;
	}
	memcpy(t->name, filename, name_len + 1);
	t->attr.mq_maxmsg = MAX_NUM_MESSAGES;
	t->attr.mq_msgsize = MAX_MESSAGE_LENGTH;
	
	// Link this in
	down(&mq_list_lock);
	if (mq_list) {
		down(&mq_list->lock);
		mq_list->prev = t;
		up(&mq_list->lock);
	}
	t->next = mq_list;
	mq_list = t;
	up(&mq_list_lock);
	up(&t->lock);
	
	return t;
}

// Open a message queue
file_descriptor_t* mq_open(const char* filename, uint32_t mode) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memset(d, 0, sizeof(file_descriptor_t));
	// Mark in use
	d->lock = MUTEX_UNLOCKED;
	d->type = SHARED_MEMORY_FILE_TYPE;
	d->mode = (mode & ~FILE_MODE_CREATE & ~FILE_MODE_EXCLUSIVE);
	int namelen = strlen(filename);
	d->filename = kmalloc(namelen + 1);
	if (!d->filename) {
		kfree(d);
		return NULL;
	}
	memcpy(d->filename, filename, namelen + 1);
	
	// Assign the functions
	d->read = mq_read;
	d->write = mq_write;
	d->stat = mq_stat;
	d->can_read = mq_can_read;
	d->can_write = mq_can_write;
	d->ioctl = mq_ioctl;
	d->duplicate = mq_duplicate;
	d->close = mq_close;
	
	mq_list_t* info = NULL;
	if (mode & FILE_MODE_CREATE) {
		if (mode & FILE_MODE_EXCLUSIVE) {
			if (mq_get(d->filename, false, false)) {
				kfree(d->filename);
				kfree(d);
				return NULL;
			}
		}
		info = mq_get(d->filename, true, true);
	} else
		info = mq_get(d->filename, false, true);
	if (!info) {
		kfree(d->filename);
		kfree(d);
		return NULL;
	}
	d->info = info;
	
	return d;
}

// Read a message (no priority information)
uint32_t mq_read(file_descriptor_t* f, void* buf, uint32_t bytes) {
	return mq_receive(f, buf, bytes, NULL);
}

// Write a message with default priority
uint32_t mq_write(file_descriptor_t* f, const void* buf, uint32_t bytes) {
	uint32_t ret = mq_send(f, buf, bytes, DEFAULT_PRIORITY);
	if (ret != 0)
		return ret;
	return bytes;
}

// Send a message
uint32_t mq_send(file_descriptor_t* f, const void* buf, uint32_t bytes, uint32_t priority) {
	if (priority >= MQ_PRIO_MAX || !buf || bytes == 0)
		return -EINVAL;
	
	// Create the message
	mq_message_t* msg = kmalloc(sizeof(mq_message_t));
	if (!msg)
		return -ENOMEM;
	memset(msg, 0, sizeof(mq_message_t));
	msg->buffer = kmalloc(bytes);
	if (!msg->buffer) {
		kfree(msg);
		return -ENOMEM;
	}
	memcpy(msg->buffer, buf, bytes);
	msg->length = bytes;
	msg->priority = priority;
	
	// Block until there is room
	mq_list_t* info = f->info;
	down(&info->lock);
	while (!(f->mode & FILE_MODE_NONBLOCKING) && !(current_pcb && current_pcb->should_terminate)) {
		if (signal_occurring(current_pcb)) {
			kfree(msg->buffer);
			kfree(msg);
			return -EINTR;
		}
		if (info->attr.mq_curmsgs < MAX_NUM_MESSAGES)
			break;
		up(&info->lock);
		schedule();
		down(&info->lock);
	}
	if (info->attr.mq_curmsgs >= MAX_NUM_MESSAGES) {
		up(&info->lock);
		kfree(msg->buffer);
		kfree(msg);
		return -EAGAIN;
	}
	
	// Find the nearest tail from the beginning
	mq_message_t* tail = NULL;
	for (uint32_t z = priority; z < MQ_PRIO_MAX; z++) {
		if (info->priority_tails[priority]) {
			tail = info->priority_tails[priority];
			break;
		}
	}
	if (!tail) {
		// No messages, just link it into the start
		msg->next = info->messages;
		if (info->messages)
			info->messages->prev = msg;
		info->messages = msg;
		info->priority_tails[priority] = msg;
	} else {
		// Insert this message
		msg->next = tail->next;
		msg->prev = tail;
		if (tail->next)
			tail->next->prev = msg;
		tail->next = msg;
		info->priority_tails[priority] = msg;
	}
	info->attr.mq_curmsgs++;
	up(&info->lock);
	
	return 0;
}

// Receive a message
uint32_t mq_receive(file_descriptor_t* f, void* buf, uint32_t bytes, uint32_t* priority) {
	if (!buf)
		return -EINVAL;
	
	mq_list_t* info = f->info;
	down(&info->lock);
	// Block until there is a message
	while (!(f->mode & FILE_MODE_NONBLOCKING) && !(current_pcb && current_pcb->should_terminate)) {
		if (signal_occurring(current_pcb))
			return -EINTR;
		if (info->attr.mq_curmsgs != 0)
			break;
		up(&info->lock);
		schedule();
		down(&info->lock);
	}
	if (info->attr.mq_curmsgs == 0) {
		up(&info->lock);
		return -EAGAIN;
	}
	
	// Copy the first message
	mq_message_t* msg = info->messages;
	uint32_t min = bytes;
	if (min < msg->length)
		min = msg->length;
	memcpy(buf, info->messages->buffer, min);
	if (priority)
		*priority = msg->priority;
	
	// Unlink the message
	info->messages = info->messages->next;
	if (info->messages)
		info->messages->prev = NULL;
	info->attr.mq_curmsgs--;
	
	// Update tail pointer
	if (info->priority_tails[msg->priority] == msg)
		info->priority_tails[msg->priority] = NULL;
	up(&info->lock);
	
	// Dealloc the message
	kfree(msg->buffer);
	kfree(msg);
	
	return min;
}

// Get and set attributes
uint32_t mq_getattr (file_descriptor_t* f, mq_attr_t* attr) {
	if (!attr)
		return -EINVAL;
	mq_list_t* info = (mq_list_t*)f->info;
	memcpy(attr, &info->attr, sizeof(mq_attr_t));
	return 0;
}

uint32_t mq_setattr (file_descriptor_t* f, const mq_attr_t* attr, mq_attr_t* old) {
	if (!attr)
		return -EINVAL;
	mq_list_t* info = (mq_list_t*)f->info;
	if (old)
		memcpy(old, &info->attr, sizeof(mq_attr_t));
	down(&info->lock);
	info->attr.mq_flags = attr->mq_flags;
	up(&info->lock);
	
	if (attr->mq_flags & FILE_MODE_NONBLOCKING)
		f->mode |= FILE_MODE_NONBLOCKING;
	else
		f->mode &= ~FILE_MODE_NONBLOCKING;
	
	return 0;
}

// Ioctl
uint32_t mq_ioctl(file_descriptor_t* f, int request, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
	switch (request) {
		case MQ_IOCTL_SEND:
			return mq_send(f, (const void*)arg1, arg2, arg3);
		case MQ_IOCTL_RECEIVE:
			return mq_receive(f, (void*)arg1, arg2, (uint32_t*)arg3);
		case MQ_IOCTL_GETATTR:
			return mq_getattr(f, (mq_attr_t*)arg1);
		case MQ_IOCTL_SETATTR:
			return mq_setattr(f, (const mq_attr_t*)arg1, (mq_attr_t*)arg2);
	}
	
	return -EINVAL;
}

// Get info
uint32_t mq_stat(file_descriptor_t* f, sys_stat_type* data) {
	mq_list_t* info = (mq_list_t*)f->info;
	data->dev_id = f->type;
	data->size = info->attr.mq_curmsgs;
	data->block_size = MAX_MESSAGE_LENGTH;
	data->mode = f->mode;
	return 0;
}

// Can read or write a message
bool mq_can_read(file_descriptor_t* f) {
	mq_list_t* info = (mq_list_t*)f->info;
	down(&info->lock);
	uint32_t ret = (info->attr.mq_curmsgs > 0);
	up(&info->lock);
	return ret;
}

bool mq_can_write(file_descriptor_t* f) {
	mq_list_t* info = (mq_list_t*)f->info;
	down(&info->lock);
	uint32_t ret = (info->attr.mq_curmsgs < MAX_NUM_MESSAGES);
	up(&info->lock);
	return ret;
}

// Duplicate a message queue
file_descriptor_t* mq_duplicate(file_descriptor_t* f) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memcpy(d, f, sizeof(file_descriptor_t));
	int namelen = strlen(f->filename);
	d->filename = kmalloc(namelen + 1);
	if (!d->filename) {
		kfree(d);
		return NULL;
	}
	memcpy(d->filename, f->filename, namelen + 1);
	d->lock = MUTEX_UNLOCKED;
	
	mq_list_t* t = d->info;
	down(&t->lock);
	t->num_open++;
	up(&t->lock);
	
	return d;
}

void mq_dealloc_mem(mq_list_t* t) {
	mq_message_t* msg = t->messages;
	while (msg) {
		mq_message_t* next = msg->next;
		kfree(msg->buffer);
		kfree(msg);
		msg = next;
	}
	kfree(t->name);
	kfree(t);
}

// Close a message queue
uint32_t mq_close(file_descriptor_t* f) {
	kfree(f->filename);
	
	mq_list_t* t = f->info;
	down(&t->lock);
	if ((--t->num_open) == 0 && t->dealloc_on_close)
		mq_dealloc_mem(t);
	else
		up(&t->lock);
	
	return 0;
}

// Unlink a message queue
uint32_t mq_unlink(const char* filename) {
	mq_list_t* t = mq_get(filename, false, false);
	if (!t)
		return -ENOENT;
	
	// Unlink
	down(&mq_list_lock);
	if (mq_list == t)
		mq_list = t->next;
	up(&mq_list_lock);
	
	down(&t->lock);
	if (t->prev)
		down(&t->prev->lock);
	if (t->next)
		down(&t->next->lock);
	if (t->prev) {
		t->prev->next = t->next;
		up(&t->prev->lock);
	}
	if (t->next) {
		t->next->prev = t->prev;
		up(&t->next->lock);
	}
	
	// Wait until the last reference is closed to dealloc it
	if (t->num_open != 0) {
		t->dealloc_on_close = true;
		up(&t->lock);
	}
	else
		mq_dealloc_mem(t);
	
	return 0;
}
