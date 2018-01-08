//
//  shmem.c
//  product
//
//  Created by Neil Singh on 1/7/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "shmem.h"
#include <memory/allocation/heap.h>
#include <syscalls/interrupt.h>

// List for keeping track of 4kb blocks in the memory
typedef struct paddr_list {
	uint32_t paddr;
	
	struct paddr_list* next;
	struct paddr_list* prev;
} paddr_list_t;

// Shared memory list
typedef struct shm_list {
	char* name;
	paddr_list_t* paddrs;
	uint32_t size;
	
	uint32_t num_open;
	bool dealloc_on_close;
	semaphore_t lock;
	
	struct shm_list* next;
	struct shm_list* prev;
} shm_list_t;

shm_list_t* shared_memories = NULL;
semaphore_t shared_memory_lock = MUTEX_UNLOCKED;

shm_list_t* shm_get(const char* filename, bool allocate, bool increment_open) {
	int name_len = strlen(filename);
	// Find it in this list
	down(&shared_memory_lock);
	shm_list_t* t = shared_memories;
	if (t)
		down(&t->lock);
	up(&shared_memory_lock);
	while (t) {
		if (strncmp(t->name, filename, name_len + 1) == 0) {
			if (increment_open)
				t->num_open++;
			up(&t->lock);
			return t;
		}
		
		shm_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	
	// Nothing was found, allocate if desired
	if (!allocate)
		return NULL;
	
	t = kmalloc(sizeof(shm_list_t));
	if (!t)
		return NULL;
	memset(t, 0, sizeof(shm_list_t));
	t->lock = MUTEX_LOCKED;
	if (increment_open)
		t->num_open = 1;
	t->name = kmalloc(name_len);
	if (!t->name) {
		kfree(t);
		return NULL;
	}
	memcpy(t->name, filename, name_len + 1);
	
	// Link this in
	down(&shared_memory_lock);
	if (shared_memories) {
		down(&shared_memories->lock);
		shared_memories->prev = t;
		up(&shared_memories->lock);
	}
	t->next = shared_memories;
	shared_memories = t;
	up(&shared_memory_lock);
	up(&t->lock);
	
	return t;
}

// Get an array of the paddrs available (must be kfreed)
uint32_t* shm_get_paddrs(file_descriptor_t* f, uint32_t* length) {
	down(&f->lock);
	shm_list_t* t = f->info;
	down(&t->lock);
	up(&f->lock);
	
	// Get count
	uint32_t count = 0;
	paddr_list_t* p = t->paddrs;
	while (p) {
		count++;
		p = p->next;
	}
	if (count == 0)
		return NULL;
	
	p = t->paddrs;
	
	// Allocate
	uint32_t* ret = kmalloc(sizeof(uint32_t) * count);
	if (!ret) {
		up(&t->lock);
		return NULL;
	}
	
	// Fill
	*length = count;
	count = 0;
	while (p) {
		ret[count++] = p->paddr;
		p = p->next;
	}
	up(&t->lock);
	
	return ret;
}

// Open a shared memory region
file_descriptor_t* shm_open(const char* filename, uint32_t mode) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memset(d, 0, sizeof(file_descriptor_t));
	// Mark in use
	d->lock = MUTEX_UNLOCKED;
	d->type = SHARED_MEMORY_FILE_TYPE;
	d->mode = ((mode & ~FILE_MODE_CREATE & ~FILE_MODE_TRUNCATE)) | FILE_TYPE_CHARACTER;
	int namelen = strlen(filename);
	d->filename = kmalloc(namelen + 1);
	if (!d->filename) {
		kfree(d);
		return NULL;
	}
	memcpy(d->filename, filename, namelen + 1);
	
	// Assign the functions
	d->read = shm_read;
	d->write = shm_write;
	d->stat = shm_stat;
	d->llseek = shm_llseek;
	d->can_read = shm_can_read;
	d->can_write = shm_can_write;
	d->truncate = shm_truncate;
	d->duplicate = shm_duplicate;
	d->close = shm_close;
	
	shm_list_t* info = NULL;
	if (mode & FILE_MODE_CREATE) {
		if (mode & FILE_MODE_EXCLUSIVE) {
			if (shm_get(d->filename, false, false)) {
				kfree(d->filename);
				kfree(d);
				return NULL;
			}
		}
		info = shm_get(d->filename, true, true);
	} else
		info = shm_get(d->filename, false, true);
	if (!info) {
		kfree(d->filename);
		kfree(d);
		return NULL;
	}
	d->info = info;
	
	if (mode & FILE_MODE_TRUNCATE)
		shm_truncate(d, uint64_make(0, 0));
	
	return d;
}

// Read a shared memory region (reads 0's to clear newly allocated memory)
uint32_t shm_read(file_descriptor_t* f, void* buf, uint32_t bytes) {
	memset(buf, 0, bytes);
	return bytes;
}

// Write a shared memory region (nop)
uint32_t shm_write(file_descriptor_t* f, const void* buf, uint32_t bytes) {
	return 0;
}

// Seek a shared memory region (nop)
uint64_t shm_llseek(file_descriptor_t* f, uint64_t offset, int whence) {
	return uint64_make(0, 0);
}

// Resize a shared memory region
uint64_t shm_truncate(file_descriptor_t* f, uint64_t s) {
	if (!(f->mode & FILE_MODE_WRITE))
		return uint64_make(-1, -1);
	
	uint32_t size = s.low;
	
	shm_list_t* t = f->info;
	down(&t->lock);
	
	// How many 4kb blocks we have now
	uint32_t num_blocks = (t->size - 1) / FOUR_KB_SIZE + 1;
	if (t->size == 0)
		num_blocks = 0;
	// How many blocks we desire
	uint32_t desired_blocks = (size - 1) / FOUR_KB_SIZE + 1;
	if (size == 0)
		desired_blocks = 0;
	
	if (desired_blocks > num_blocks) {
		// Allocate memory into the back
		// Find tail
		paddr_list_t* p = t->paddrs;
		while (p && p->next)
			p = p->next;
		
		// Allocate a new paddr for each one we need
		while (num_blocks < desired_blocks) {
			paddr_list_t* n = kmalloc(sizeof(paddr_list_t));
			memset(n, 0, sizeof(paddr_list_t));
			n->paddr = (uint32_t)page_physical_get_aligned_four_kb(VIRTUAL_MEMORY_USER);
			if (!n->paddr) {
				kfree(n);
				size = num_blocks * FOUR_KB_SIZE;
				break;
			}
			
			n->prev = p;
			if (p)
				p->next = n;
			p = n;
			if (!t->paddrs)
				t->paddrs = n;
			num_blocks++;
		}
		
	} else if (desired_blocks < num_blocks) {
		// Deallocate memory from the back
		// Find tail
		paddr_list_t* p = t->paddrs;
		while (p && p->next)
			p = p->next;
		
		// Deallocate the last paddr however many times we need
		while (num_blocks > desired_blocks) {
			paddr_list_t* prev = p->prev;
			page_physical_free_aligned_four_kb((void*)p->paddr);
			kfree(p);
			p = prev;
			num_blocks--;
		}
		if (num_blocks == 0)
			t->paddrs = NULL;
	}
	
	t->size = size;
	up(&t->lock);
	
	return uint64_make(0, size);
}

// Get info
uint32_t shm_stat(file_descriptor_t* f, sys_stat_type* data) {
	shm_list_t* info =(shm_list_t*)f->info;
	data->dev_id = f->type;
	data->size = info->size;
	data->mode = f->mode;
	return 0;
}

// Can read or write to a shared memory region
bool shm_can_read(file_descriptor_t* f) {
	return (f->mode & FILE_MODE_READ) != 0;
}

bool shm_can_write(file_descriptor_t* f) {
	return (f->mode & FILE_MODE_WRITE) != 0;
}

// Duplicate a shared memory region
file_descriptor_t* shm_duplicate(file_descriptor_t* f) {
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
	
	shm_list_t* t = d->info;
	down(&t->lock);
	t->num_open++;
	up(&t->lock);
	
	return d;
}

void shm_dealloc_mem(shm_list_t* t) {
	// Free the paddr list
	paddr_list_t* p = t->paddrs;
	while (p) {
		paddr_list_t* next = p->next;
		page_physical_free_aligned_four_kb((void*)p->paddr);
		kfree(p);
		p = next;
	}
	kfree(t->name);
	kfree(t);
}

// Close a shared memory region
uint32_t shm_close(file_descriptor_t* f) {
	kfree(f->filename);
	
	shm_list_t* t = f->info;
	down(&t->lock);
	if ((--t->num_open) == 0 && t->dealloc_on_close)
		shm_dealloc_mem(t);
	else
		up(&t->lock);
	
	return 0;
}

// Unlink a shared memory region
uint32_t shm_unlink(const char* filename) {
	shm_list_t* t = shm_get(filename, false, false);
	if (!t)
		return -ENOENT;
	
	// Unlink
	down(&shared_memory_lock);
	if (shared_memories == t)
		shared_memories = t->next;
	up(&shared_memory_lock);
	
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
		shm_dealloc_mem(t);
	
	return 0;
}
