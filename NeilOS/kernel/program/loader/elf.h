//
//  elf.h
//  NeilOS
//
//  Created by Neil Singh on 6/9/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#ifndef ELF_H
#define ELF_H

#include <common/types.h>
#include <program/task.h>
#include <program/dylib.h>

// Load an elf into memory
bool elf_load(char* filename, pcb_t* pcb, pcb_t* parent);

// Load dynamic library
bool elf_load_dylib(char* filename, dylib_t* dylib);

// Copy information and perform relocation for a dylib
bool elf_load_dylib_for_task(dylib_t* dylib, pcb_t* pcb, uint32_t offset);

// Compute the hash of a symbol name
uint32_t elf_compute_hash(char* name);

#endif /* ELF_H */
