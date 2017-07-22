//
//  dylib.h
//  NeilOS
//
//  Created by Neil Singh on 7/11/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef DYLIB_H
#define DYLIB_H

#include <common/types.h>
#include <memory/page_list.h>

struct pcb;
struct dylib;

typedef struct dylib_list {
	struct dylib* dylib;
	struct dylib_list* prev;
	struct dylib_list* next;
} dylib_list_t;

typedef struct {
	char* name;
	void* addr;
	bool valid;
} dylib_symbol;

typedef struct {
	char* name;
	uint32_t offset;
	union {
		uint32_t value;
		struct {
			uint32_t type : 8;
			uint32_t index : 24;
		} __attribute__((packed));
	};
	uint32_t size;
	void* data;
} dylib_rel_t;

typedef struct {
	dylib_rel_t* rels;
	uint32_t num_rels;
} dylib_rel_section_t;

typedef struct {
	void* data;
	uint32_t offset;
	uint32_t size;
} dylib_data_section_t;

typedef struct dylib {
	char* name;
	page_list_t* page_list;
	
	dylib_symbol* symbols;
	uint32_t num_symbols;
	
	dylib_rel_section_t* rel_sections;
	uint32_t num_rel_sections;
	
	dylib_list_t* dylibs;
	uint32_t num_instances;			// Number of objects that are linked against this dylib
} dylib_t;

// Returns true whether a dylib is loaded
bool dylib_is_loaded(char* name);

// Load a dylib
dylib_t* dylib_load(char* name);

// Copy a dylib list for a task
bool dylib_list_copy_for_pcb(dylib_list_t* list, struct pcb* pcb);

// Create a dylib that serves as basically the symbol table holder for a task
dylib_t* dylib_create_from_pcb(struct pcb* pcb);

// Load a dylib for a specific application by name
bool dylib_load_for_task_by_name(char* name, struct pcb* pcb, bool relocate);

// Load a dylib for a specific application
bool dylib_load_for_task(dylib_t* dylib, struct pcb* pcb, bool relocate);

// Unload a dylib for a specific application
void dylib_unload_for_task(dylib_t* dylib);

// Get the symbol address for a specific symbol
void* dylib_get_symbol_address(dylib_t* dylib, char* name, bool* found);

// Get the symbol address for a specific symbol
void* dylib_get_symbol_address_list(dylib_list_t* dylibs, char* name, bool* found);

// Unload a dylib
void dylib_unload(char* name);

// Dealloc a dylib
void dylib_dealloc(dylib_t* dylib);

// Dealloc a dylib list for a task
void dylib_list_dealloc(dylib_list_t* d);

#endif /* DYLIB_H */
