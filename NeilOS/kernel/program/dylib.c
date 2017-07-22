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
	
	dylib_list_t* t = dylibs;
	uint32_t len = strlen(realname) + 1;
	while (t) {
		if (strncmp(realname, t->dylib->name, len) == 0) {
			kfree(realname);
			return true;
		}
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
	t->dylib = kmalloc(sizeof(dylib_t));
	if (!t->dylib) {
		kfree(t);
		return false;
	}
	memset(t->dylib, 0, sizeof(dylib_t));
	
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
	if (dylibs) {
		t->next = dylibs;
		dylibs->prev = t;
	}
	dylibs = t;
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
	
	dylib_list_t* list = kmalloc(sizeof(dylib_list_t));
	if (!list) {
		kfree(dylib);
		return NULL;
	}
	memset(list, 0, sizeof(dylib_list_t));
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
	
	dylib_list_t* t = dylibs;
	uint32_t len = strlen(realname) + 1;
	while (t) {
		if (strncmp(realname, t->dylib->name, len) == 0) {
			kfree(realname);
			return t->dylib;
		}
		t = t->next;
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
	
	// We must place this at the back because the order of linking is important for duplicate symbols
	dylib_list_push_back(&pcb->dylibs, l);
	
	return true;
}

// Load a dylib for a specific application
bool dylib_load_for_task_by_name(char* name, pcb_t* pcb, bool relocate) {
	dylib_t* dylib = dylib_get(name, true);
	if (!dylib)
		return false;
	
	return dylib_load_for_task(dylib, pcb, relocate);
}

bool dylib_load_for_task(dylib_t* dylib, pcb_t* pcb, bool relocate) {
	dylib_list_t* list = kmalloc(sizeof(dylib_list_t));
	if (!list)
		return false;
	
	if (relocate) {
		// Map the original dylib in
		page_list_t* t = dylib->page_list;
		while (t) {
			vm_map_page(t->vaddr, t->paddr, USER_PAGE_DIRECTORY_ENTRY);
			t = t->next;
		}
	}

	// Copy over the dylib data
	memset(list, 0, sizeof(dylib_list_t));
	list->dylib = dylib;
	page_list_t* p = dylib->page_list;
	page_list_t* pl = NULL;
	while (p) {
		page_list_t* page = page_list_add_copy(&pl, p, true);
		if (!page) {
			page_list_dealloc(pl);
			kfree(list);
			return false;
		}
		if (relocate)
			vm_map_page(page->vaddr, page->paddr, USER_PAGE_DIRECTORY_ENTRY);
	
		p = p->next;
	}
	page_list_t* pl_end = pl;
	while (pl_end && pl_end->next)
		pl_end = pl_end->next;
	pl_end->next = pcb->page_list;
	pcb->page_list->prev = pl_end;
	pcb->page_list = pl_end;
	
	// We must place this at the back because the order of linking is important for duplicate symbols
	dylib_list_push_back(&pcb->dylibs, list);
	
	dylib->num_instances++;
	
	if (relocate)
		elf_load_dylib_for_task(dylib, pcb);
	
	return true;
}

// Unload a dylib for a specific application
void dylib_unload_for_task(dylib_t* dylib) {
	if (--(dylib->num_instances) == 0)
		dylib_dealloc(dylib);
}

// Get the symbol address for a specific symbol
void* dylib_get_symbol_address(dylib_t* dylib, char* name, bool* found) {
	uint32_t len = strlen(name) + 1;
	*found = false;
	for (uint32_t z = 0; z < dylib->num_symbols; z++) {
		if (dylib->symbols[z].valid && strncmp(dylib->symbols[z].name, name, len) == 0) {
			*found = true;
			return dylib->symbols[z].addr;
		}
	}
	return NULL;
}

// Get the symbol address for a specific symbol
void* dylib_get_symbol_address_list(dylib_list_t* dylibs, char* name, bool* found) {
	dylib_list_t* t = dylibs;
	*found = false;
	while (t) {
		void* ret = dylib_get_symbol_address(t->dylib, name, found);
		if (*found)
			return ret;
		t = t->next;
	}
	return NULL;
}

// Unload a dylib
void dylib_unload(char* name) {
	char* realname = dylib_get_real_name(name);
	if (!realname)
		return;
	
	// Find the dylib
	dylib_list_t* t = dylibs;
	uint32_t len = strlen(realname) + 1;
	dylib_t* dylib = NULL;
	while (t) {
		if (strncmp(realname, t->dylib->name, len) == 0) {
			kfree(realname);
			dylib = t->dylib;
			// Unlink it
			if (t->prev)
				t->prev->next = t->next;
			else
				dylibs = t->next;
			break;
		}
	}
	if (!dylib)
		return;
	
	dylib_dealloc(dylib);
}

// Dealloc a dylib
void dylib_dealloc(dylib_t* dylib) {
	if (dylib->name)
		kfree(dylib->name);
	if (dylib->symbols) {
		for (uint32_t z = 0; z < dylib->num_symbols; z++) {
			if (dylib->symbols[z].name)
				kfree(dylib->symbols[z].name);
		}
		kfree(dylib->symbols);
	}
	if (dylib->rel_sections) {
		for (uint32_t z = 0; z < dylib->num_rel_sections; z++) {
			if (dylib->rel_sections[z].rels) {
				for (uint32_t i = 0; i < dylib->rel_sections[z].num_rels; i++) {
					if (dylib->rel_sections[z].rels[i].name)
						kfree(dylib->rel_sections[z].rels[i].name);
					if (dylib->rel_sections[z].rels[i].data)
						kfree(dylib->rel_sections[z].rels[i].data);
				}
				kfree(dylib->rel_sections[z].rels);
			}
		}
		kfree(dylib->rel_sections);
	}
	page_list_dealloc(dylib->page_list);
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