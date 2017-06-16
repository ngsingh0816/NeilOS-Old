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

// Load an elf into memory
bool elf_load_into_memory(char* filename, pcb_t* pcb);

#endif /* ELF_H */
