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
#include <common/concurrency/semaphore.h>
#include <memory/page_list.h>

struct pcb;
struct dylib;

typedef struct dylib_list {
	struct dylib* dylib;
	uint32_t offset;
	
	semaphore_t lock;
	struct dylib_list* prev;
	struct dylib_list* next;
} dylib_list_t;

typedef struct {
	uint32_t name_index;
	void* addr;
	bool valid;
} dylib_symbol;

typedef struct {
	uint32_t name_index;
	uint32_t offset;
	union {
		uint32_t value;
		struct {
			uint32_t type : 8;
			uint32_t index : 24;
		} __attribute__((packed));
	};
	uint32_t size;
} dylib_rel_t;

typedef struct {
	dylib_rel_t* rels;
	char* rel_names;
	uint32_t num_rels;
} dylib_rel_section_t;

typedef struct {
	void* data;
	uint32_t offset;
	uint32_t size;
} dylib_data_section_t;

typedef struct {
	uint32_t nbuckets;
	uint32_t nchains;
	uint32_t* buckets;
	uint32_t* chains;
} dylib_hash_section_t;

typedef struct dylib {
	char* name;
	page_list_t* page_list;
	
	dylib_hash_section_t hash;
	
	dylib_symbol* symbols;
	char* symbol_names;
	uint32_t num_symbols;
	
	dylib_rel_section_t* rel_sections;
	uint32_t num_rel_sections;
	
	dylib_list_t* dylibs;
	uint32_t num_instances;			// Number of objects that are linked against this dylib
	
	// Init function
	uint32_t init;
	void (**init_array)();
	uint32_t init_array_length;
	
	mutex_t lock;
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
bool dylib_load_for_task_by_name(char* name, struct pcb* pcb);

// Load a dylib for a specific application
bool dylib_load_for_task(dylib_t* dylib, struct pcb* pcb);

// Unload a dylib for a specific application
void dylib_unload_for_task(dylib_t* dylib);

// Helper that returns true if the pcb contains a dylib with the given name
bool dylib_task_contains_dylib_by_name(char* name, struct pcb* pcb, bool full_name);

// Load a dylib for another dylib by name
bool dylib_load_for_dylib_by_name(char* name, dylib_t* dylib);

// Load a dylib for another dylib
bool dylib_load_for_dylib(dylib_t* n, dylib_t* dylib);

// Get the symbol address for a specific symbol
void* dylib_get_symbol_address(dylib_t* dylib, char* name, bool* found);

// Get the symbol address for a specific symbol
void* dylib_get_symbol_address_list(dylib_list_t* dylibs, char* name, bool* found);

// Perform initialization functions for a list of dylibs
void dylib_list_perform_init(dylib_list_t* list);

// Unload a dylib
void dylib_unload(char* name);

// Dealloc a dylib
void dylib_dealloc(dylib_t* dylib);

// Dealloc a dylib list for a task
void dylib_list_dealloc(dylib_list_t* d);

#endif /* DYLIB_H */
