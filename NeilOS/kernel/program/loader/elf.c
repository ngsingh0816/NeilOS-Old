//
//  elf.c
//  NeilOS
//
//  Created by Neil Singh on 6/9/17.
//  Copyright © 2017 Neil Singh. All rights reserved.
//

#include "elf.h"

#include <syscalls/descriptor.h>
#include <program/task.h>

#define ELF_MAGIC_NUMBER			0x464c457f

#define ELF_TYPE_RELOCATABLE		1
#define ELF_TYPE_EXECUTABLE			2
#define ELF_TYPE_SHARED				3
#define ELF_TYPE_CORE				4

#define ELF_32BIT					1
#define ELF_64BIT					2

#define ELF_ISA_NONE				0
#define ELF_ISA_x86					3

#define ELF_SEGMENT_NULL			0
#define ELF_SEGMENT_LOAD			1
#define ELF_SEGMENT_DYNAMIC			2
#define ELF_SEGMENT_INTERP			3
#define ELF_SEGMENT_NOTE			4

#define ELF_FLAGS_EXECUTABLE		1
#define ELF_FLAGS_WRITABLE			2
#define ELF_FLAGS_READABLE			4

#define ELF_SECTION_NULL			0
#define ELF_SECTION_PROGRAM			1		// text
#define ELF_SECTION_SYMBOL_TABLE	2
#define ELF_SECTION_STRING_TABLE	3
#define ELF_SECTION_RELOCATIONA		4		// relocation with addens
#define ELF_SECTION_HASH			5		// symbol hash map
#define ELF_SECTION_DYNAMIC			6
#define ELF_SECTION_NOTE			7
#define ELF_SECTION_NOBITS			8		// bss
#define ELF_SECTION_RELOCATION		9		// relocation no addens
#define ELF_SECTION_SHLIB			10
#define ELF_SECTION_DYNSYM			11		// dynamic symbol linker table
#define ELF_SECTION_INIT_ARRAY		14		// constructor table
#define ELF_SECTION_FINI_ARRAY		15		// destructor table
#define ELF_SECTION_PREINIT_ARRAY	16		// pre constructor table
#define ELF_SECTION_GROUP			17
#define ELF_SECTION_SYMTAB			18		// extended section indices
#define ELF_SECTION_NUM				19		// number of defined types

typedef struct {
	uint32_t magic_number;		// 0x464c457f
	uint8_t bit;				// 1 = 32bit, 2 = 64bit
	uint8_t endianess;			// 1 = little, 2 = big
	uint8_t version;
	uint8_t os;
	uint8_t _unused[8];
	uint16_t type;				// relocatable, executable, shared, or core
	uint16_t isa;
	uint32_t elf_version;
	uint32_t entry;				// entry point
	uint32_t program_header_table_position;
	uint32_t section_header_table_position;
	uint32_t flags;
	uint16_t header_size;
	uint16_t program_header_entry_size;
	uint16_t program_header_num_entries;
	uint16_t section_header_entry_size;
	uint16_t section_header_num_entries;
	uint16_t section_header_name_index;
} elf_header_t;

typedef struct {
	uint32_t type;				// null, load, dynamic, interp, note, etc...
	uint32_t offset;			// Offset in file where data can be found
	uint32_t vaddr;				// Where this segment should be put in virtual memory
	uint32_t _unused;
	uint32_t size;				// Size of segment in file
	uint32_t mem_size;			// Size of segment in memory
	uint32_t flags;				// executable, writable, readable
	uint32_t _unused2;
} elf_program_header_t;

typedef struct {
	uint32_t name;				// offset in shstrtab section that defines name
	uint32_t type;				// type of section
	uint32_t flags;
	uint32_t addr;				// virtual address
	uint32_t offset;			// offset in image
	uint32_t size;
	uint32_t link;				// link to another section
	uint32_t info;				// extra info
	uint32_t addralign;			// alignment
	uint32_t entry_size;
} elf_section_header_t;

// Load an elf into memory
bool elf_load_into_memory(char* filename, pcb_t* pcb) {
	// Open the file
	file_descriptor_t file;
	// Check it is valid
	if (!fopen(filename, FILE_MODE_READ, &file))
		return false;
	if (file.type != FILE_FILE_TYPE) {
		fclose(&file);
		return false;
	}
	
	elf_header_t header;
	if (fread(&header, 1, sizeof(header), &file) != sizeof(header))
		return false;
	
	// Ensure some things
	if (header.magic_number != ELF_MAGIC_NUMBER || header.bit != ELF_32BIT ||
		(header.isa != ELF_ISA_NONE && header.isa != ELF_ISA_x86)) {
		fclose(&file);
		return false;
	}
	
	pcb->entry = header.entry;
	
	uint16_t z;
	for (z = 0; z < header.program_header_num_entries; z++) {
		elf_program_header_t program_header;
		fseek(&file, uint64_make(0, header.program_header_table_position + z * sizeof(program_header)), SEEK_SET);
		fread(&program_header, 1, sizeof(program_header), &file);
		
		if (program_header.type != ELF_SEGMENT_LOAD)
			continue;
		
		// Check that we can load this segment into memory and allocate new pages if needed
		uint32_t pos = program_header.vaddr - (program_header.vaddr % FOUR_MB_SIZE);
		uint32_t real_size = program_header.mem_size > program_header.size ? program_header.mem_size : program_header.size;
		for (; pos < program_header.vaddr + real_size; pos += FOUR_MB_SIZE) {
			bool found = false;
			
			memory_list_t* t = pcb->memory_map;
			memory_list_t* last = t;
			while (t) {
				if (t->vaddr == pos) {
					found = true;
					break;
				}
				last = t;
				t = t->next;
			}
			
			if (found)
				continue;
			
			memory_list_t* map = kmalloc(sizeof(memory_list_t));
			if (!map)
				return false;
			memset(map, 0, sizeof(memory_list_t));
			
			void* page = page_get_four_mb(1, VIRTUAL_MEMORY_USER);
			if (!page) {
				kfree(map);
				return false;
			}
			map->vaddr = pos;
			map->paddr = (uint32_t)vm_virtual_to_physical((uint32_t)page);
			vm_unmap_page((uint32_t)page);
			vm_map_page(map->vaddr, map->paddr, USER_PAGE_DIRECTORY_ENTRY);
			
			map->prev = last;
			if (last)
				last->next = map;
			else
				pcb->memory_map = map;
		}
		
		// Load the program segment into memory
		fseek(&file, uint64_make(0, program_header.offset), SEEK_SET);
		fread((void*)program_header.vaddr, program_header.size, 1, &file);
		if (program_header.size < program_header.mem_size)
			memset((void*)(program_header.vaddr + program_header.size), 0, program_header.mem_size - program_header.size);
	}
	
	return true;
}
