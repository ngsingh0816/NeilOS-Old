//
//  dylib.c
//  NeilOS
//
//  Created by Neil Singh on 7/11/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "dylib.h"
#include <common/lib.h>
#include "task.h"
#include <program/loader/elf.h>

dylib_list_t* dylibs = NULL;
semaphore_t dylib_lock = MUTEX_UNLOCKED;

// Gets the actual name of a dylib given its "name"
char* dylib_get_real_name(char* name) {
	if (!name)
		return NULL;
	
	if (name[0] == '/') {
		uint32_t len = strlen(name);
		char* ret = kmalloc(len + 1);
		if (!ret)
			return NULL;
		memcpy(ret, name, len + 1);
		return ret;
	}
	
	const char* prefix = "/usr/lib/";
	uint32_t len = strlen(name);
	uint32_t len2 = strlen(prefix);
	char* ret = kmalloc(len + len2 + 1);
	if (!ret)
		return NULL;
	memcpy(ret, prefix, len2);
	memcpy(&ret[9], name, len + 1);
	
	return ret;
}

// Returns true whether a dylib is loaded
bool dylib_is_loaded(char* name) {
	char* realname = dylib_get_real_name(name);
	if (!realname)
		return false;
	
	down(&dylib_lock);
	if (dylibs)
		down(&dylibs->lock);
	dylib_list_t* t = dylibs;
	up(&dylib_lock);
	uint32_t len = strlen(realname) + 1;
	while (t) {
		down(&t->dylib->lock);
		if (t->dylib && strncmp(realname, t->dylib->name, len) == 0) {
			up(&t->dylib->lock);
			up(&t->lock);
			kfree(realname);
			return true;
		}
		up(&t->dylib->lock);
		
		dylib_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	
	kfree(realname);
	return false;
}

// Load a dylib
dylib_t* dylib_load(char* name) {
	char* real_name = dylib_get_real_name(name);
	if (!real_name)
		return false;
	
	// Allocate a new dylib list entry
	dylib_list_t* t = kmalloc(sizeof(dylib_list_t));
	if (!t)
		return false;
	memset(t, 0, sizeof(dylib_list_t));
	t->lock = MUTEX_UNLOCKED;
	t->dylib = kmalloc(sizeof(dylib_t));
	if (!t->dylib) {
		kfree(t);
		return false;
	}
	memset(t->dylib, 0, sizeof(dylib_t));
	t->dylib->lock = MUTEX_UNLOCKED;
	
	uint32_t len = strlen(real_name);
	t->dylib->name = (char*)kmalloc(len + 1);
	if (!t->dylib->name) {
		kfree(t->dylib);
		kfree(t);
		kfree(real_name);
		return NULL;
	}
	memcpy(t->dylib->name, real_name, len + 1);
	
	// Load it
	if (!elf_load_dylib(real_name, t->dylib)) {
		kfree(t->dylib);
		kfree(t);
		kfree(real_name);
		return NULL;
	}
	
	// Link it in
	down(&dylib_lock);
	if (dylibs) {
		t->next = dylibs;
		dylibs->prev = t;
	}
	dylibs = t;
	up(&dylib_lock);
	kfree(real_name);
	
	return t->dylib;
}

// Create a dylib that serves as basically the symbol table holder for a task
dylib_t* dylib_create_from_pcb(pcb_t* pcb) {
	dylib_t* dylib = kmalloc(sizeof(dylib_t));
	if (!dylib)
		return NULL;
	
	memset(dylib, 0, sizeof(dylib_t));
	dylib->num_instances = 1;
	dylib->lock = MUTEX_UNLOCKED;
	
	dylib_list_t* list = kmalloc(sizeof(dylib_list_t));
	if (!list) {
		kfree(dylib);
		return NULL;
	}
	memset(list, 0, sizeof(dylib_list_t));
	list->lock = MUTEX_UNLOCKED;
	list->dylib = dylib;
	list->next = pcb->dylibs;
	// Must be first to hide duplicate external symbols
	pcb->dylibs = list;
	
	return dylib;
}

// Gets a dylib from the list
dylib_t* dylib_get(char* name, bool allocate) {
	char* realname = dylib_get_real_name(name);
	if (!realname)
		return false;
	
	down(&dylib_lock);
	if (dylibs)
		down(&dylibs->lock);
	dylib_list_t* t = dylibs;
	up(&dylib_lock);
	uint32_t len = strlen(realname) + 1;
	while (t) {
		down(&t->dylib->lock);
		if (t->dylib && strncmp(realname, t->dylib->name, len) == 0) {
			up(&t->dylib->lock);
			up(&t->lock);
			kfree(realname);
			return t->dylib;
		}
		up(&t->dylib->lock);
		
		dylib_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	
	if (allocate) {
		dylib_t* ret = dylib_load(realname);
		kfree(realname);
		return ret;
	}
	
	kfree(realname);
	return NULL;
}

// Helper to place a dylib_list entry at the end of a list
// Note: does not lock
void dylib_list_push_back(dylib_list_t** list, dylib_list_t* entry) {
	dylib_list_t* prev = *list;
	while (prev && prev->next)
		prev = prev->next;
	
	if (!(*list))
		*list = entry;
	else {
		entry->prev = prev;
		prev->next = entry;
	}
}

// Copy a dylib list
bool dylib_list_copy_for_pcb(dylib_list_t* list, pcb_t* pcb) {
	dylib_list_t* l = kmalloc(sizeof(dylib_list_t));
	if (!l)
		return false;
	
	memset(l, 0, sizeof(dylib_list_t));
	l->dylib = list->dylib;
	l->dylib->num_instances++;
	l->offset = list->offset;
	
	// We must place this at the back because the order of linking is important for duplicate symbols
	dylib_list_push_back(&pcb->dylibs, l);
	
	return true;
}

// Load a dylib for a specific application
bool dylib_load_for_task_by_name(char* name, pcb_t* pcb) {
	dylib_t* dylib = dylib_get(name, true);
	if (!dylib)
		return false;
	
	return dylib_load_for_task(dylib, pcb);
}

// Helper that returns true if the pcb contains a dylib with the given name
bool dylib_task_contains_dylib_by_name(char* name, pcb_t* pcb, bool full_name) {
	char* realname = full_name ? name : dylib_get_real_name(name);
	
	dylib_list_t* t = pcb->dylibs;
	if (t)
		down(&t->lock);
	uint32_t len = strlen(realname) + 1;
	while (t) {
		down(&t->dylib->lock);
		if (t->dylib->name && strncmp(realname, t->dylib->name, len) == 0) {
			up(&t->dylib->lock);
			up(&t->lock);
			if (!full_name)
				kfree(realname);
			return true;
		}
		up(&t->dylib->lock);
		
		dylib_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	
	if (!full_name)
		kfree(realname);
	return false;
}

bool dylib_load_for_task(dylib_t* dylib, pcb_t* pcb) {
	dylib_list_t* list = kmalloc(sizeof(dylib_list_t));
	if (!list)
		return false;
	
	down(&dylib->lock);

	// Copy over the dylib data
	memset(list, 0, sizeof(dylib_list_t));
	list->lock = MUTEX_UNLOCKED;
	list->dylib = dylib;
	page_list_t* p = dylib->page_list;
	page_list_t* pl = NULL;
	uint32_t num_pages = 0;
	uint32_t offset = (uint32_t)-1;
	while (p) {
		page_list_t* page = page_list_add_copy(&pl, p);
		if (!page) {
			up(&dylib->lock);
			page_list_dealloc(pl);
			kfree(list);
			
			return false;
		}
	
		num_pages++;
		if (offset > page->vaddr)
			offset = page->vaddr;
		p = p->next;
	}
	up(&dylib->lock);
	
	// Map these pages to a new offset
	vm_lock();
	uint32_t vaddr = vm_get_next_unmapped_pages(num_pages, VIRTUAL_MEMORY_USER);
	if (!vaddr) {
		vm_unlock();
		page_list_dealloc(pl);
		kfree(list);
		return false;
	}
	list->offset = vaddr;

	p = pl;
	while (p) {
		p->vaddr += vaddr - offset;
		p = p->next;
	}
	
	// Add the dylib pages to the pcb pages
	page_list_t* t = pcb->page_list;
	page_list_t* prev = NULL;
	if (t)
		down(&t->lock);
	while (t) {
		prev = t;
		t = t->next;
		
		if (t) {
			down(&t->lock);
			up(&prev->lock);
		}
	}
	if (prev) {
		pl->prev = prev;
		prev->next = pl;
		up(&prev->lock);
	} else
		pcb->page_list = pl;
	
	// Map these pages in
	p = pl;
	while (p) {
		page_list_map(p, false);
		p = p->next;
	}
	vm_unlock();
	
	// We must place this at the back because the order of linking is important for duplicate symbols
	dylib_list_push_back(&pcb->dylibs, list);
	
	down(&dylib->lock);
	dylib->num_instances++;
	if (dylib->dylibs)
		down(&dylib->dylibs->lock);
	dylib_list_t* d = dylib->dylibs;
	up(&dylib->lock);
	
	// Link dependent dylibs if they haven't already been linked
	while (d) {
		down(&d->dylib->lock);
		char* name = d->dylib->name;
		up(&d->dylib->lock);
		
		if (!dylib_task_contains_dylib_by_name(name, pcb, true))
			dylib_load_for_task_by_name(name, pcb);
		
		dylib_list_t* prev = d;
		d = d->next;
		if (d)
			down(&d->lock);
		up(&prev->lock);
	}
		
	return true;
}

// Unload a dylib for a specific application
void dylib_unload_for_task(dylib_t* dylib) {
	down(&dylib->lock);
	if (--(dylib->num_instances) == 0) {
		up(&dylib->lock);
		dylib_unload(dylib->name);
	} else
		up(&dylib->lock);
}

// Load a dylib for another dylib by name
bool dylib_load_for_dylib_by_name(char* name, dylib_t* dylib) {
	dylib_t* d = dylib_get(name, true);
	if (!d)
		return false;
	
	return dylib_load_for_dylib(d, dylib);
}

// Load a dylib for another dylib
bool dylib_load_for_dylib(dylib_t* n, dylib_t* dylib) {
	dylib_list_t* t = kmalloc(sizeof(dylib_list_t));
	if (!t)
		return false;
	memset(t, 0, sizeof(dylib_list_t));
	t->lock = MUTEX_UNLOCKED;
	t->dylib = n;
	
	dylib_list_push_back(&dylib->dylibs, t);
	
	return true;
}

// Get the symbol address for a specific symbol by name
void* dylib_get_symbol_address_name(dylib_t* dylib, char* name, bool* found) {
	uint32_t len = strlen(name) + 1;
	*found = false;
	down(&dylib->lock);
	for (uint32_t z = 0; z < dylib->num_symbols; z++) {
		if (dylib->symbols[z].valid && strncmp(&dylib->symbol_names[dylib->symbols[z].name_index],
											   name, len) == 0) {
			*found = true;
			void* ret = dylib->symbols[z].addr;
			up(&dylib->lock);
			return ret;
		}
	}
	up(&dylib->lock);
	return NULL;
}

// Get the symbol address for a specific symbol
void* dylib_get_symbol_address_hash(dylib_t* dylib, uint32_t hash, char* name, bool* found) {
	uint32_t len = strlen(name) + 1;
	*found = false;
	down(&dylib->lock);
	uint32_t index = dylib->hash.buckets[hash % dylib->hash.nbuckets];
	while (index != 0) {
		if (index >= dylib->num_symbols || index >= dylib->hash.nchains)
			break;
		
		if (dylib->symbols[index].valid && strncmp(&dylib->symbol_names[dylib->symbols[index].name_index],
											   name, len) == 0) {
			*found = true;
			void* ret = dylib->symbols[index].addr;
			up(&dylib->lock);
			return ret;
		}
		index = dylib->hash.chains[index];
	}
	up(&dylib->lock);
	return NULL;
}

// Get the symbol address for a specific symbol
void* dylib_get_symbol_address(dylib_t* dylib, char* name, bool* found) {
	return dylib_get_symbol_address_hash(dylib, elf_compute_hash(name), name, found);
}

// Get the symbol address for a specific symbol
void* dylib_get_symbol_address_list(dylib_list_t* list, char* name, bool* found) {
	*found = false;
	uint32_t hash = elf_compute_hash(name);
	dylib_list_t* t = list;
	if (t)
		down(&t->lock);
	while (t) {
		void* ret = NULL;
		if (t->dylib) {
			if (t->dylib->hash.nbuckets != 0)
				ret = dylib_get_symbol_address_hash(t->dylib, hash, name, found);
			else
				ret = dylib_get_symbol_address_name(t->dylib, name, found);
		}
		if (*found) {
			up(&t->lock);
			return (void*)((uint32_t)ret + t->offset);
		}
		
		dylib_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		up(&prev->lock);
	}
	return NULL;
}

// Perform initialization functions for a list of dylibs
void dylib_list_perform_init(dylib_list_t* list) {
	// Initialize all the dylibs
	dylib_list_t* dylib_list = list;
	if (dylib_list)
		down(&dylib_list->lock);
	while (dylib_list) {
		dylib_t* dylib = dylib_list->dylib;
		
		// Perform the initialization function
		down(&dylib->lock);
		uint32_t dinit = dylib->init;
		uint32_t array_length = dylib->init_array_length;
		void (**arrays)() = dylib->init_array;
		up(&dylib->lock);
		// TODO: do these in user space
		if (dinit) {
			void (*init)() = (void (*)())(dylib_list->offset + dinit);
			init();
		}
		for (uint32_t z = 0; z < array_length; z++) {
			void (*init)() = (void (*)())(dylib_list->offset + (uint32_t)arrays[z]);
			init();
		}
		
		dylib_list_t* prev = dylib_list;
		dylib_list = dylib_list->next;
		if (dylib_list)
			down(&dylib_list->lock);
		up(&prev->lock);
	}
}

// Unload a dylib
void dylib_unload(char* name) {
	char* realname = dylib_get_real_name(name);
	if (!realname)
		return;
	
	uint32_t len = strlen(realname) + 1;

	// Find the dylib
	down(&dylib_lock);
	dylib_list_t* t = dylibs;
	dylib_t* dylib = NULL;
	if (t)
		down(&t->lock);
	up(&dylib_lock);
	while (t) {
		down(&t->dylib->lock);
		if (t->dylib && strncmp(realname, t->dylib->name, len) == 0) {
			up(&t->dylib->lock);
			dylib = t->dylib;
			if (t->next)
				down(&t->next->lock);
			// Unlink it
			if (t->prev)
				t->prev->next = t->next;
			else
				dylibs = t->next;
			if (t->next) {
				t->next->prev = t->prev;
				up(&t->next->lock);
			}
			
			if (t->prev)
				up(&t->prev->lock);
			up(&t->lock);
			
			kfree(realname);
			break;
		}
		up(&t->dylib->lock);
		
		if (t->prev)
			up(&t->prev->lock);
		dylib_list_t* prev = t;
		t = t->next;
		if (t)
			down(&t->lock);
		else
			up(&prev->lock);
	}
	if (!dylib)
		return;
	
	dylib_dealloc(dylib);
}

// Dealloc a dylib
void dylib_dealloc(dylib_t* dylib) {
	down(&dylib->lock);
	
	if (dylib->name)
		kfree(dylib->name);
	if (dylib->symbols)
		kfree(dylib->symbols);
	if (dylib->symbol_names)
		kfree(dylib->symbol_names);
	if (dylib->rel_sections) {
		for (uint32_t z = 0; z < dylib->num_rel_sections; z++) {
			if (dylib->rel_sections[z].rels) {		
				if (dylib->rel_sections[z].rel_names)
					kfree(dylib->rel_sections[z].rel_names);
				kfree(dylib->rel_sections[z].rels);
			}
		}
		kfree(dylib->rel_sections);
	}
	if (dylib->hash.buckets)
		kfree(dylib->hash.buckets);
	if (dylib->hash.chains)
		kfree(dylib->hash.chains);
	if (dylib->init_array)
		kfree(dylib->init_array);
	page_list_dealloc(dylib->page_list);
	
	dylib_list_t* t = dylib->dylibs;
	while (t) {
		dylib_list_t* prev = t;
		t = t->next;
		kfree(prev);
	}
	
	up(&dylib->lock);
	kfree(dylib);
}

// Dealloc a dylib list for a task
void dylib_list_dealloc(dylib_list_t* d) {
	while (d) {
		dylib_unload_for_task(d->dylib);
		dylib_list_t* prev = d;
		d = d->next;
		kfree(prev);
	}
}
