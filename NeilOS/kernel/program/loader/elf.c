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
#include <memory/page_list.h>
#include <program/dylib.h>

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

#define ELF_DYNAMIC_NULL			0
#define ELF_DYNAMIC_NEEDED			1
#define ELF_DYNAMIC_STRTAB			5

#define ELF_REL_NONE				0
#define ELF_REL_32					1
#define ELF_REL_PC32				2
#define ELF_REL_GOT32				3
#define ELF_REL_PLT32				4
#define ELF_REL_COPY				5
#define ELF_REL_GLOB_DATA			6
#define ELF_REL_JUMP_SLOT			7
#define ELF_REL_RELATIVE			8
#define ELF_REL_GOTOFF				9
#define ELF_REL_GOTPC				10

#define ELF_SYMBOL_UNDEFINED		0

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
} __attribute__((packed)) elf_header_t;

typedef struct {
	uint32_t type;				// null, load, dynamic, interp, note, etc...
	uint32_t offset;			// Offset in file where data can be found
	uint32_t vaddr;				// Where this segment should be put in virtual memory
	uint32_t _unused;
	uint32_t size;				// Size of segment in file
	uint32_t mem_size;			// Size of segment in memory
	uint32_t flags;				// executable, writable, readable
	uint32_t _unused2;
} __attribute__((packed)) elf_program_header_t;

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
} __attribute__((packed)) elf_section_header_t;

typedef struct {
	uint32_t tag;
	uint32_t value;
} __attribute__((packed)) elf_dynamic_tag_t;

typedef struct {
	uint32_t offset;
	union {
		uint32_t value;
		struct {
			uint32_t type : 8;
			uint32_t index : 24;
		} __attribute__((packed));
	};
} __attribute__((packed)) elf_rel_t;

typedef struct {
	uint32_t name;
	uint32_t value;
	uint32_t size;
	uint8_t info;
	uint8_t other;
	uint16_t shindex;
} elf_symbol_table_t;

// Check that an elf file is valid
bool elf_check_valid(char* filename, file_descriptor_t* file, elf_header_t* header) {
	// Check it is valid
	if (!fopen(filename, FILE_MODE_READ, file))
		return false;
	if (!(file->mode & FILE_TYPE_REGULAR)) {
		fclose(file);
		return false;
	}
	
	if (fread(header, 1, sizeof(elf_header_t), file) != sizeof(elf_header_t)) {
		fclose(file);
		return false;
	}
	
	// Ensure some things
	if (header->magic_number != ELF_MAGIC_NUMBER || header->bit != ELF_32BIT ||
		(header->isa != ELF_ISA_NONE && header->isa != ELF_ISA_x86)) {
		fclose(file);
		return false;
	}
	
	return true;
}

// Allocate a buffer filled with elf data
void* elf_read_data(file_descriptor_t* file, uint32_t addr, uint32_t size) {
	void* buffer = kmalloc(size);
	if (!buffer)
		return NULL;
	
	fseek(file, uint64_make(0, addr), SEEK_SET);
	fread(buffer, size, 1, file);
	
	return buffer;
}

bool elf_load_common(elf_header_t* header, file_descriptor_t* file,
					 elf_program_header_t** program_headers, elf_section_header_t** section_headers,
					 char*** strtabs) {
	// Load program headers, section headers, and strings
	if (program_headers) {
		*program_headers = elf_read_data(file, header->program_header_table_position,
										sizeof(elf_program_header_t) * header->program_header_num_entries);
		if (!(*program_headers))
			return false;
	}
	
	if (section_headers) {
		*section_headers = elf_read_data(file, header->section_header_table_position,
										sizeof(elf_section_header_t) * header->section_header_num_entries);
		if (!(*section_headers))
			return false;
	}
	
	if (strtabs) {
		*strtabs = (char**)kmalloc(sizeof(char**) * header->section_header_num_entries);
		if (!(*strtabs))
			return false;
		memset(*strtabs, 0, sizeof(char**) * header->section_header_num_entries);
		for (uint32_t z = 0; z < header->section_header_num_entries; z++) {
			if ((*section_headers)[z].type == ELF_SECTION_STRING_TABLE) {
				(*strtabs)[z] = elf_read_data(file, (*section_headers)[z].offset, (*section_headers)[z].size);
				if (!(*strtabs)[z])
					return false;
			}
		}
	}
	
	return true;
}

// Find the associated strtab for the given address
char* elf_get_strtab(elf_section_header_t* sections, uint32_t num_sections, uint32_t addr, char** strtabs) {
	for (uint32_t z = 0; z < num_sections; z++) {
		if (sections[z].addr == addr)
			return strtabs[z];
	}
	return NULL;
}

// Returns the name of a symbol
char* elf_get_symbol_name(elf_section_header_t* section, elf_symbol_table_t* symtab,
						  uint32_t sym_index, char** strtabs) {
	char* strtab = strtabs[section->link];
	return &strtab[symtab[sym_index].name];
}

// Load a symbol table
bool elf_load_symbol_table(elf_section_header_t* sections, elf_header_t* header,
						   file_descriptor_t* file, char* section_names, char** strtabs,
						   dylib_t* dylib, uint32_t offset, char* symtab_name) {
	uint32_t len = strlen(symtab_name);
	for (uint32_t z = 0; z < header->section_header_num_entries; z++) {
		elf_section_header_t* section_header = &sections[z];
		if (strncmp(symtab_name, &section_names[section_header->name], len + 1) == 0) {
			elf_symbol_table_t* dynsym = elf_read_data(file, section_header->offset, section_header->size);
			if (!dynsym)
				return false;
			
			uint32_t num_symbols = section_header->size / sizeof(elf_symbol_table_t);
			dylib->symbols = (dylib_symbol*)kmalloc(num_symbols * sizeof(dylib_symbol));
			if (!dylib->symbols) {
				kfree(dynsym);
				return false;
			}
			char* strtab = strtabs[section_header->link];
			uint32_t str_len = sections[section_header->link].size;
			dylib->symbol_names = kmalloc(str_len + 1);
			if (!dylib->symbol_names) {
				kfree(dylib->symbols);
				dylib->symbols = NULL;
				kfree(dynsym);
				return false;
			}
			memcpy(dylib->symbol_names, strtab, str_len + 1);
			
			// Load the symbol info
			for (uint32_t i = 0; i < num_symbols; i++) {
				dylib->symbols[i].name_index = dynsym[i].name;
				dylib->symbols[i].addr = (void*)(offset + dynsym[i].value);
				dylib->symbols[i].valid = dynsym[i].shindex != ELF_SYMBOL_UNDEFINED;
				dylib->num_symbols++;
			}
			
			kfree(dynsym);
			
			break;
		}
	}
	
	return true;
}

// Compute the hash of a symbol name
uint32_t elf_compute_hash(char* name) {
	uint32_t h = 0, g;
	while (*name) {
		h = (h << 4) + *name++;
		if ((g = h & 0xf0000000))
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

// Load a hash table
bool elf_load_hash_table(elf_section_header_t* sections, elf_header_t* header,
						 file_descriptor_t* file, char* section_names, char** strtabs,
						 dylib_t* dylib) {
	for (uint32_t q = 0; q < header->section_header_num_entries; q++) {
		if (sections[q].type != ELF_SECTION_HASH)
			continue;
		
		// Read
		uint32_t* buffer = elf_read_data(file, sections[q].offset, sections[q].size);
		if (!buffer)
			return false;
		
		// Allocate
		dylib->hash.nbuckets = buffer[0];
		dylib->hash.nchains = buffer[1];
		dylib->hash.buckets = kmalloc(buffer[0] * sizeof(uint32_t));
		if (!dylib->hash.buckets) {
			kfree(buffer);
			return false;
		}
		dylib->hash.chains = kmalloc(buffer[1] * sizeof(uint32_t));
		if (!dylib->hash.chains) {
			kfree(dylib->hash.buckets);
			dylib->hash.buckets = NULL;
			kfree(buffer);
			return false;
		}
		
		// Copy
		memcpy(dylib->hash.buckets, &buffer[2], buffer[0] * sizeof(uint32_t));
		memcpy(dylib->hash.chains, &buffer[2 + buffer[0]], buffer[1]);
	}
	
	return true;
}

// Perform relocation for a dylib
bool elf_perform_relocation_dylib(dylib_rel_section_t* rel_sections, uint32_t num_rel_sections,
								  dylib_list_t* dylibs, uint32_t offset) {
	for (uint32_t q = 0; q < num_rel_sections; q++) {
		dylib_rel_t* rels = rel_sections[q].rels;
		char* rel_names = rel_sections[q].rel_names;
		for (uint32_t z = 0; z < rel_sections[q].num_rels; z++) {
			void** dest = (void**)(rels[z].offset + offset);
			switch (rels[z].type) {
				case ELF_REL_GLOB_DATA:	// TODO: this is where we can copy things over for shared data
				case ELF_REL_COPY:
				case ELF_REL_JUMP_SLOT: {
					bool found = false;
					void* value = dylib_get_symbol_address_list(dylibs, &rel_names[rels[z].name_index], &found);
					if (!found) {
						//printf("Undefined symbol - %s %s\n", rels[z].name);
						//ret = false;*/
						continue;
					}
					//printf("Symbol(%d) - %s (0x%x)\n", (rels[z].type == ELF_REL_COPY), rels[z].name, value);
					if (rels[z].type == ELF_REL_COPY)
						memcpy(dest, value, rels[z].size);
					else
						*dest = value;
					break;
				}
				case ELF_REL_RELATIVE: {
					*dest += offset;
					break;
				}
				default:
					printf("Unknown relative symbol type: %d.\n", rels[z].type);
					return false;
			}
		}
	}
	
	return true;
}

// Perform relocation
bool elf_perform_relocation(elf_section_header_t* sections, elf_header_t* header,
							file_descriptor_t* file, char* section_names, char** strtabs,
							dylib_list_t* dylibs, uint32_t offset, bool relative) {
	bool ret = true;
	for (uint32_t z = 0; z < header->section_header_num_entries; z++) {
		elf_section_header_t* section = &sections[z];
		if (section->type == ELF_SECTION_RELOCATION) {
			elf_rel_t* rels = elf_read_data(file, section->offset, section->size);
			if (!rels)
				return false;
			
			// Get associated symbol table
			elf_symbol_table_t* symtab = elf_read_data(file, sections[section->link].offset,
													   sections[section->link].size);
			if (!symtab) {
				kfree(rels);
				return false;
			}
			
			for (uint32_t i = 0; i < section->size / sizeof(elf_rel_t); i++) {
				void** dest = (void**)(rels[i].offset + offset);
				switch (rels[i].type) {
					case ELF_REL_GLOB_DATA:	// TODO: this is where we can copy things over for shared data
					case ELF_REL_COPY:
					case ELF_REL_JUMP_SLOT: {
						char* symbol_name = elf_get_symbol_name(&sections[section->link], symtab, rels[i].index, strtabs);
						// Try to find it within our symbol tables
						bool found = false;
						void* value = dylib_get_symbol_address_list(dylibs, symbol_name, &found);
						if (!found) {
							/*printf("Undefined symbol - %s in section %s for 0x%x\n",
								   symbol_name, &section_names[section->name], dest);*/
							continue;
						}
						/*printf("Symbol(%d) - %s (*0x%x = 0x%x) at 0x%x\n", (rels[i].type == ELF_REL_COPY),
							   symbol_name, value, *(uint32_t*)value, dest);*/
						if (rels[i].type == ELF_REL_COPY) {
							uint32_t size = symtab[rels[i].index].size;
							memcpy(dest, value, size);
						} else
							*dest = value;
						break;
					}
					case ELF_REL_RELATIVE: {
						if (relative)
							*dest += offset;
						break;
					}
					default:
						printf("Unknown relative symbol type: %d.\n", rels[i].type);
						return false;
				}
			}
			
			kfree(symtab);
			kfree(rels);
		}
	}
	
	return ret;
}

// Load an elf into memory
bool elf_load(char* filename, pcb_t* pcb) {
	file_descriptor_t file;
	elf_header_t header;
	// Check it is valid
	if (!elf_check_valid(filename, &file, &header))
		return false;
	
	pcb->entry = header.entry;
	
	bool ret = false;
	
	elf_program_header_t* program_headers = NULL;
	elf_section_header_t* section_headers = NULL;
	char** strtabs = NULL;
	
	// Load program headers, section headers, and strings
	if (!elf_load_common(&header, &file, &program_headers, &section_headers, &strtabs))
		goto cleanup;
	char* section_names = strtabs[header.section_header_name_index];
	
	// Load program headers
	uint16_t z;
	for (z = 0; z < header.program_header_num_entries; z++) {
		elf_program_header_t* program_header = &program_headers[z];
		
		if (program_header->type != ELF_SEGMENT_LOAD)
			continue;
		
		// Check that we can load this segment into memory and allocate new pages if needed
		uint32_t pos = program_header->vaddr - (program_header->vaddr % FOUR_MB_SIZE);
		uint32_t real_size = program_header->mem_size > program_header->size ?
			program_header->mem_size : program_header->size;
		for (; pos < program_header->vaddr + real_size; pos += FOUR_MB_SIZE) {
			page_list_t* t = page_list_get(&pcb->page_list, pos, MEMORY_WRITE, true);
			if (!t)
				return false;
			page_list_map(t);
		}
		
		// Load the program segment into memory
		fseek(&file, uint64_make(0, program_header->offset), SEEK_SET);
		fread((void*)program_header->vaddr, program_header->size, 1, &file);
		if (program_header->size < program_header->mem_size) {
			memset((void*)(program_header->vaddr + program_header->size), 0,
				   program_header->mem_size - program_header->size);
		}
	}
	
	// Load the dynamic symbol table
	dylib_t* dylib = dylib_create_from_pcb(pcb);
	if (!elf_load_symbol_table(section_headers, &header, &file, section_names, strtabs, dylib, 0, ".symtab"))
		goto cleanup;
	
	// Load dynamic info
	for (z = 0; z < header.section_header_num_entries; z++) {
		elf_section_header_t* section_header = &section_headers[z];
		if (section_header->type == ELF_SECTION_DYNAMIC) {
			elf_dynamic_tag_t* tags = elf_read_data(&file, section_header->offset, section_header->size);
			if (!tags)
				goto cleanup;
			
			// Find strtab
			char* strtab = NULL;
			for (uint32_t i = 0; i < section_header->size / sizeof(elf_dynamic_tag_t); i++) {
				if (tags[i].tag == ELF_DYNAMIC_STRTAB) {
					strtab = elf_get_strtab(section_headers, header.section_header_num_entries, tags[i].value, strtabs);
					if (!strtab) {
						kfree(tags);
						goto cleanup;
					}
					break;
				}
			}
			
			for (uint32_t i = 0; i < section_header->size / sizeof(elf_dynamic_tag_t); i++) {
				if (tags[i].tag == ELF_DYNAMIC_NEEDED) {
					if (!dylib_load_for_task_by_name(&strtab[tags[i].value], pcb)) {
						printf("Could not load needed library: %s\n", &strtab[tags[i].value]);
						kfree(tags);
						goto cleanup;
					}
				}
			}
			
			kfree(tags);
		}
	}
	
	// Perform relocation (but don't include symbols from ourself)
	if (!elf_perform_relocation(section_headers, &header, &file, section_names, strtabs,
								pcb->dylibs->next, 0, true))
		goto cleanup;
	
	// Load hash table
	if (!elf_load_hash_table(section_headers, &header, &file, section_names, strtabs, pcb->dylibs->dylib))
		goto cleanup;
	
	ret = true;
	
cleanup:
	fclose(&file);
	if (program_headers)
		kfree(program_headers);
	if (section_headers)
		kfree(section_headers);
	if (strtabs) {
		for (uint32_t z = 0; z < header.section_header_num_entries; z++) {
			if (strtabs[z])
				kfree(strtabs[z]);
		}
		kfree(strtabs);
	}
	return ret;
}

// Load dynamic library
bool elf_load_dylib(char* filename, dylib_t* dylib) {
	file_descriptor_t file;
	elf_header_t header;
	// Check it is valid
	if (!elf_check_valid(filename, &file, &header))
		return false;
	
	bool ret = false;
	
	elf_program_header_t* program_headers = NULL;
	elf_section_header_t* section_headers = NULL;
	char** strtabs = NULL;
	
	// Load program headers, section headers, and strings
	if (!elf_load_common(&header, &file, &program_headers, &section_headers, &strtabs))
		goto cleanup;
	char* section_names = strtabs[header.section_header_name_index];
	
	// Get the number of pages we need
	uint16_t z;
	uint32_t num_pages = 0;
	for (z = 0; z < header.program_header_num_entries; z++) {
		elf_program_header_t* program_header = &program_headers[z];
		
		if (program_header->type != ELF_SEGMENT_LOAD)
			continue;
		
		// Check that we can load this segment into memory and allocate new pages if needed
		uint32_t pos = program_header->vaddr - (program_header->vaddr % FOUR_MB_SIZE);
		uint32_t real_size = program_header->mem_size > program_header->size ?
		program_header->mem_size : program_header->size;
		for (; pos < program_header->vaddr + real_size; pos += FOUR_MB_SIZE) {
			page_list_t* t = page_list_get(&dylib->page_list, pos, MEMORY_WRITE, true);
			if (!t)
				return false;
			num_pages++;
		}
	}
	
	// Offset these pages and map them
	// TODO: might need a memory lock or something
	bool flags = set_multitasking_enabled(false);
	uint32_t offset = vm_get_next_unmapped_pages(num_pages, VIRTUAL_MEMORY_SHARED);
	page_list_t* t = dylib->page_list;
	while (t) {
		t->vaddr += offset;
		page_list_map(t);
		t = t->next;
	}
	
	// Acutally load the program segment
	for (z = 0; z < header.program_header_num_entries; z++) {
		elf_program_header_t* program_header = &program_headers[z];
		
		if (program_header->type != ELF_SEGMENT_LOAD)
			continue;
		
		// Load the program segment into memory
		uint32_t real_vaddr = program_header->vaddr + offset;
		fseek(&file, uint64_make(0, program_header->offset), SEEK_SET);
		fread((void*)real_vaddr, program_header->size, 1, &file);
		if (program_header->size < program_header->mem_size) {
			memset((void*)(real_vaddr + program_header->size), 0,
				   program_header->mem_size - program_header->size);
		}
	}
	set_multitasking_enabled(flags);
	
	// Load the dynamic symbol table
	if (!elf_load_symbol_table(section_headers, &header, &file, section_names, strtabs, dylib, 0, ".dynsym"))
		goto cleanup;
	
	// Get number of rel sections
	uint32_t num_rel_sections = 0;
	for (uint32_t z = 0; z < header.section_header_num_entries; z++) {
		elf_section_header_t* section = &section_headers[z];
		if (section->type == ELF_SECTION_RELOCATION)
			num_rel_sections++;
	}
	dylib->rel_sections = kmalloc(sizeof(dylib_rel_section_t));
	if (!dylib->rel_sections)
		goto cleanup;
	dylib->num_rel_sections = 0;
	
	for (uint32_t z = 0; z < header.section_header_num_entries; z++) {
		elf_section_header_t* section = &section_headers[z];
		if (section->type == ELF_SECTION_RELOCATION) {
			elf_rel_t* rels = elf_read_data(&file, section->offset, section->size);
			if (!rels)
				goto cleanup;
			
			// Get associated symbol table
			elf_symbol_table_t* symtab = elf_read_data(&file, section_headers[section->link].offset,
													   section_headers[section->link].size);
			if (!symtab) {
				kfree(rels);
				goto cleanup;
			}
			
			// Allocate rel table
			uint32_t num_rels = section->size / sizeof(elf_rel_t);
			
			dylib_rel_t* dylib_rels = kmalloc(sizeof(dylib_rel_t) * num_rels);
			if (!dylib_rels) {
				kfree(symtab);
				kfree(rels);
				goto cleanup;
			}
			uint32_t strtab_index = section_headers[section->link].link;
			uint32_t str_len = section_headers[strtab_index].size;
			dylib->rel_sections[dylib->num_rel_sections].rel_names = kmalloc(str_len);
			if (!dylib->rel_sections[dylib->num_rel_sections].rel_names) {
				kfree(dylib_rels);
				kfree(symtab);
				kfree(rels);
				goto cleanup;
			}
			memcpy(dylib->rel_sections[dylib->num_rel_sections].rel_names, strtabs[strtab_index], str_len);
			
			dylib->rel_sections[dylib->num_rel_sections].rels = dylib_rels;
			dylib->rel_sections[dylib->num_rel_sections++].num_rels = 0;
			uint32_t dylib_num_rels = 0;
			
			// Copy the info
			for (uint32_t i = 0; i < section->size / sizeof(elf_rel_t); i++) {
				dylib_rels[dylib_num_rels].offset = rels[i].offset;
				dylib_rels[dylib_num_rels].value = rels[i].value;
				dylib_rels[dylib_num_rels].name_index = symtab[rels[i].index].name;
				dylib_rels[dylib_num_rels].size = symtab[rels[i].index].size;
				
				dylib_num_rels++;
				dylib->rel_sections[dylib->num_rel_sections - 1].num_rels++;
			}
			
			kfree(symtab);
			kfree(rels);
		}
	}
	
	// Load hash table
	if (!elf_load_hash_table(section_headers, &header, &file, section_names, strtabs, dylib))
		goto cleanup;
	
	ret = true;
	
cleanup:
	fclose(&file);
	if (program_headers)
		kfree(program_headers);
	if (section_headers)
		kfree(section_headers);
	if (strtabs) {
		for (uint32_t z = 0; z < header.section_header_num_entries; z++) {
			if (strtabs[z])
				kfree(strtabs[z]);
		}
		kfree(strtabs);
	}
	return ret;
}

// Copy information and perform relocation for a dylib
bool elf_load_dylib_for_task(dylib_t* dylib, pcb_t* pcb, uint32_t offset) {
	return elf_perform_relocation_dylib(dylib->rel_sections, dylib->num_rel_sections,
										pcb->dylibs, offset);
}
