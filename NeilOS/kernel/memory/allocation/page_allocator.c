//
//  page_allocator.c
//  MemoryAllocator
//
//  Created by Neil Singh on 12/21/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#include "page_allocator.h"
#include "buddy.h"
#include <common/lib.h>
#include <memory/memory.h>
#include <program/task.h>
#include <common/concurrency/semaphore.h>

#define MIN_PAGE_SIZE		(1024 * 1024 / 16)			// 64KB
#define MAX_PAGE_SIZE		(1024 * 1024 * 1024)		// 1 GB

#define BUDDY_SIZE			(2 * (MAX_PAGE_SIZE / MIN_PAGE_SIZE) / 4)		// Number of nodes in the buddy tree
#define LEVEL_PAGE_SIZE		8								// Level that gives nodes a size of 4MB (log(1 GB / 4MB))
// Values for buddy nodes
#define NODE_FREE			0								// Free for use
#define NODE_USED			1								// Not free for use and directly allocated
#define NODE_USED_INDIRECT	2								// Not free for use but indirectly allocated

#define NUMBER_OF_PAGES					1024				// 4GB / 4MB
#define PAGE_SIZE						(1024 * 1024 * 4)	// 4MB

uint8_t buddy[BUDDY_SIZE];
mutex_t buddy_lock = MUTEX_UNLOCKED;
uint32_t space_used;

typedef struct page_four_kb {
	bool entries[FOUR_MB_SIZE / FOUR_KB_SIZE];
	uint32_t type;
	uint32_t next_entry;
	bool full;
	struct page_four_kb* prev;
	struct page_four_kb* next;
} page_four_kb_t;
page_four_kb_t* four_kb_pages;
mutex_t four_kb_lock = MUTEX_UNLOCKED;

// Checks whether a virtual page of a specific type maps to a physical page
bool page_mapping_exists(uint32_t paddr, uint32_t type, void** addr_out) {
	// TODO: this is pretty bad probably
	uint32_t start = (type == VIRTUAL_MEMORY_KERNEL) ? (VM_KERNEL_ADDRESS / FOUR_MB_SIZE) : 0;
	uint32_t end = (type == VIRTUAL_MEMORY_KERNEL) ? NUMBER_OF_PAGES : (VM_KERNEL_ADDRESS / FOUR_MB_SIZE);
	for (uint32_t z = start; z < end; z++) {
		uint32_t vaddr = z * PAGE_SIZE;
		if (vm_virtual_to_physical(vaddr) == (void*)paddr && vm_is_page_mapped(vaddr)) {
			*addr_out = (void*)vaddr;
			return true;
		}
	}
	
	return false;
}

// Initialize the page allocateor
void page_allocator_init() {
	// Set only the root to free
	buddy_clear_tree(buddy, BUDDY_SIZE);
	buddy_set_node_value(buddy, 0, NODE_FREE);
		
	space_used = 0;
	// Reserve space for the kernel code
	page_get_four_mb(num_kernel_pages_reserved, VIRTUAL_MEMORY_KERNEL);
}

// Helper to convert a physical address to a virtual one
void* convert_physical_to_virtual(uint32_t paddr, uint32_t size, uint32_t type) {
	// Check if it already mapped into virtual memory
	uint32_t paddr_offset = paddr % PAGE_SIZE;
	void* addr = NULL;
	if (!page_mapping_exists(paddr - paddr_offset, type, &addr)) {
		if (size == 0)
			size = 1;
		
		// We need to map new virtual pages
		uint32_t paddr_aligned = paddr - paddr_offset;
		const uint32_t num_pages = (size - 1) / PAGE_SIZE + 1;
		vm_lock();
		uint32_t vaddr = vm_get_next_unmapped_pages(num_pages, type);
		int z;
		for (z = 0; z < num_pages; z++) {
			vm_map_page(vaddr + z * PAGE_SIZE, paddr_aligned + z * PAGE_SIZE,
						MEMORY_WRITE | ((type == VIRTUAL_MEMORY_KERNEL) ? MEMORY_KERNEL : 0x0), false);
			
		}
		vm_unlock();
		addr = (void*)vaddr;
	}
	
	// Return the mapped virtual address
	return (void*)(paddr_offset + addr);
}

// Gets a 4kb aligned 4kb virtual page
void* page_get_aligned_four_kb() {
	void* addr = page_physical_get_aligned_four_kb(VIRTUAL_MEMORY_KERNEL);
	if (!addr)
		return NULL;
	
	return convert_physical_to_virtual((uint32_t)addr, FOUR_KB_SIZE, VIRTUAL_MEMORY_KERNEL);
}

// Gets a 4kb aligned 4kb page (must be freed with page_free_aligned_four_kb)
void* page_physical_get_aligned_four_kb(uint32_t type) {
	// Find an open page in the list
	down(&four_kb_lock);
	page_four_kb_t* t = four_kb_pages;
	page_four_kb_t* prev = NULL;

	vm_lock();
	uint32_t vaddr = vm_get_next_unmapped_page(VIRTUAL_MEMORY_KERNEL);
	if (!vaddr) {
		vm_unlock();
		up(&four_kb_lock);
		return NULL;
	}
	while (t) {
		vm_map_page(vaddr, (uint32_t)t, MEMORY_WRITE | MEMORY_KERNEL, false);
		uint32_t paddr = (uint32_t)t;
		t = (page_four_kb_t*)vaddr;
		
		if (t->type != type || t->full)
			goto go_next;
		for (uint32_t z = 0; z < FOUR_MB_SIZE / FOUR_KB_SIZE; z++) {
			if (!t->entries[z]) {
				t->entries[z] = 1;
				vm_unmap_page(vaddr, false);
				vm_unlock();
				up(&four_kb_lock);
				return (void*)(paddr + FOUR_KB_SIZE * z);
			}
		}
		t->full = true;
	go_next:
		prev = (page_four_kb_t*)paddr;
		t = t->next;
	}
	vm_unmap_page(vaddr, true);
	vm_unlock();
	
	// Allocate a new page if needed
	t = page_get_four_mb(1, VIRTUAL_MEMORY_KERNEL);
	if (!t) {
		up(&four_kb_lock);
		return NULL;
	}
	
	memset(t, 0, sizeof(page_four_kb_t));
	t->entries[0] = 1;
	t->entries[1] = 1;
	t->type = type;
	t->next = NULL;
	t->prev = prev;
	vaddr = (uint32_t)t;
	t = vm_virtual_to_physical(vaddr);
	if (prev) {
		vm_map_page(vaddr, (uint32_t)prev, MEMORY_WRITE | MEMORY_KERNEL, false);
		prev = (page_four_kb_t*)vaddr;
		prev->next = t;
	}
	else
		four_kb_pages = t;
	
	vm_unmap_page(vaddr, false);
	
	up(&four_kb_lock);
	
	return (void*)((uint32_t)t + FOUR_KB_SIZE);
}

// Frees a 4kb aligned 4kb virtual page
void page_free_aligned_four_kb(void* addr) {
	bool ret = page_physical_free_aligned_four_kb(vm_virtual_to_physical((uint32_t)addr));
	if (ret)
		vm_unmap_page((uint32_t)addr * ~(FOUR_MB_SIZE - 1), false);
}

// Frees a 4kb aligned 4kb page
bool page_physical_free_aligned_four_kb(void* addr) {
	// Get 4MB aligned journal
	down(&four_kb_lock); // TODO: can make this more efficient by invidiualized locking in linked list
	uint32_t paddr = ((uint32_t)addr & ~(FOUR_MB_SIZE - 1));
	page_four_kb_t* t = (page_four_kb_t*)paddr;
	uint32_t offset = ((uint32_t)addr - paddr) / FOUR_KB_SIZE;
	
	vm_lock();
	uint32_t vaddr = vm_get_next_unmapped_page(VIRTUAL_MEMORY_KERNEL);
	if (!vaddr) {
		vm_unlock();
		up(&four_kb_lock);
		return false;
	}
	vm_map_page(vaddr, (uint32_t)t, MEMORY_WRITE | MEMORY_KERNEL, false);
	t = (page_four_kb_t*)vaddr;
	
	if (!t->entries[offset])
		printf("Error: physical four kb page 0x%x being freed was not allocated.\n", addr);
	t->entries[offset] = 0;
	t->full = false;
	
	// Check if all the entries are 0 (excluding the first)
	bool found = false;
	for (uint32_t z = 1; z < FOUR_MB_SIZE / FOUR_KB_SIZE; z++) {
		if (t->entries[z]) {
			found = true;
			break;
		}
	}
	if (found) {
		vm_unmap_page(vaddr, false);
		vm_unlock();
		up(&four_kb_lock);
		return false;
	}
	
	// Remove this entry
	page_four_kb_t* next = t->next, *prev = t->prev;
	if (prev) {
		vm_map_page(vaddr, (uint32_t)prev, MEMORY_WRITE | MEMORY_KERNEL, false);
		((page_four_kb_t*)vaddr)->next = next;
	}
	if (next) {
		vm_map_page(vaddr, (uint32_t)next, MEMORY_WRITE | MEMORY_KERNEL, false);
		((page_four_kb_t*)vaddr)->prev = prev;
	}
	if (t == four_kb_pages)
		four_kb_pages = next;
	up(&four_kb_lock);
	
	vm_unmap_page(vaddr, false);
	vm_unlock();
	
	page_physical_free((void*)paddr);
	return true;
}

// Get a number of 4MB pages
void* page_get_four_mb(uint32_t num, uint32_t type) {
	return page_get(num * FOUR_MB_SIZE / MIN_PAGE_SIZE, type);
}

void* page_physical_get_four_mb(uint32_t num) {
	return page_physical_get(num * FOUR_MB_SIZE / MIN_PAGE_SIZE);
}

// Get a number of 64KB physical pages
void* page_get(uint32_t size, uint32_t type) {
	void* addr = page_physical_get(size);
	if (!addr)
		return NULL;
	
	return convert_physical_to_virtual((uint32_t)addr, size * MIN_PAGE_SIZE, type);
}

// Get a number of 64KB pages
void* page_physical_get(uint32_t s) {
	if (s == 0)
		return NULL;
	
	uint32_t real_size = s * MIN_PAGE_SIZE;
	// Don't support anything bigger than the max size
	if (real_size > MAX_PAGE_SIZE)
		return NULL;
	
	// Don't allow anything smaller than the min size
	if (real_size < MIN_PAGE_SIZE)
		real_size = MIN_PAGE_SIZE;
	
	// Find the smallest size that is a power of 2 that is bigger or equal to this
	uint32_t size = MAX_PAGE_SIZE;
	int32_t level = 0;
	while (real_size <= size) {
		size >>= 1;
		level++;
	}
	size <<= 1;
	level--;
	
	// Find the first available node of the same type by looping through all the levels
	int32_t l;
	uint32_t node = 0, node_index = 0;
	bool found = false;
	down(&buddy_lock);
	for (l = level; l >= 0; l--) {
		// Loop through all the nodes in this level
		int z, num_nodes = buddy_get_number_of_nodes_in_level(l);
		for (z = 0; z < num_nodes; z++) {
			// Check if this node is free and of the correct type
			node = buddy_get_indexed_node_at_level(l, z);
			if (buddy_get_node_value(buddy, node) == NODE_FREE) {
				// We've found a free one that is big enough
				found = true;
				node_index = z;
				break;
			}
		}
		// If we've already found one, we don't need to continue
		if (found)
			break;
	}
	
	// If we couldn't find a node, we don't have the available space
	if (!found) {
		up(&buddy_lock);
		return NULL;
	}
	
	space_used += size;
	
	// This node may have a size that is too big for us, so we may need to
	// split it up into smaller section
	uint32_t node_size = MAX_PAGE_SIZE >> l;
	if (node_size == size) {
		// If this size is an exact match, just set the node as in use and return
		buddy_set_node_value(buddy, node, NODE_USED);
		
		// We have the physical address
		up(&buddy_lock);
		return (void*)(node_index * node_size);
	}
	
	// Get the number of levels down we need to go
	uint32_t dl = level - l;
	
	// Get the left how ever many levels we need to go
	uint32_t left_node = node;
	int32_t i;
	for (i = 0; i < dl; i++) {
		// We only are using the leftmost child, so all the right children along the way are free
		uint32_t right_child = buddy_get_right_child(left_node);
		buddy_set_node_value(buddy, right_child, NODE_FREE);
		
		// Get the next left child and mark it as in use and directly if it is our last node
		left_node = buddy_get_left_child(left_node);
		buddy_set_node_value(buddy, left_node, (i == (dl - 1)) ? NODE_USED : NODE_USED_INDIRECT);
	}
	
	// Mark the parent as in use
	buddy_set_node_value(buddy, node, NODE_USED_INDIRECT);
	
	// Return the memory location of this left most node
	void* ret = (void*)(buddy_get_index_of_node_at_level(left_node, level) * size);
	up(&buddy_lock);
	return ret;
}

void page_physical_free_impl(void* addr, void* vaddr, bool use_vadr);

void page_free(void* addr) {
	void* vaddr = addr;
	
	// Convert this virtual address to a physical one
	addr = vm_virtual_to_physical((uint32_t)vaddr);
	
	page_physical_free_impl(addr, vaddr, true);
}

void page_physical_free(void* addr) {
	page_physical_free_impl(addr, NULL, false);
}

void page_physical_free_impl(void* addr, void* vaddr, bool use_vadr) {
	// Many blocks could have this address but only one can be in use (and all below it are in use too)
	// Find the smallest size block and work our way up to the one that
	uint32_t size = MIN_PAGE_SIZE;
	uint32_t node = buddy_get_indexed_node_at_level(log2(MAX_PAGE_SIZE / MIN_PAGE_SIZE), (uint32_t)addr / MIN_PAGE_SIZE);
	
	// If this node is not in use, then nothing could have been allocated to this spot
	down(&buddy_lock);
	if (buddy_get_node_value(buddy, node) == NODE_FREE) {
		up(&buddy_lock);
		printf("Error (1): page (0x%x) being freed was not allocated.\n", addr);
		return;
	}
	
	// Move up the tree until we either find the allocated node or get to a different address
	bool found = false;
	while (node != 0) {
		if (buddy_get_node_value(buddy, node) == NODE_USED) {
			// We found the node that is allocated
			found = true;
			break;
		}
		// If our node is the right child, then moving up the tree will be a different address
		if (buddy_node_is_right_child(node))
			break;
		
		// Move up the tree
		node = buddy_get_parent(node);
		size <<= 1;
	}
	
	// If we didn't find a node that could be allocated, we still need to check the top most node
	if (!found) {
		if (!(buddy_get_node_value(buddy, 0) == NODE_USED && addr == 0)) {
			// We couldn't find anything allocated
			up(&buddy_lock);
			printf("Error (2): page (0x%x) being freed was not allocated.\n", addr);
			return;
		}
	}
	
	// Mark the node as free and let the heap know about the space that was freed up
	buddy_set_node_value(buddy, node, NODE_FREE);
	space_used -= size;
	
	// Iteratively merge the blocks if we can
	uint32_t page_node = buddy_get_indexed_node_at_level(LEVEL_PAGE_SIZE, (uint32_t)addr / PAGE_SIZE);
	bool unmapped = false;
	while (node != 0) {
		// Nodes can be merged with its buddy if they are both free
		// We only have to worry about the node we freed and above
		// because everything else will not have changed
		uint32_t bud = buddy_get_buddy_for_node(node);
		// Check if we can merge these blocks
		if (buddy_get_node_value(buddy, node) == NODE_FREE &&
			buddy_get_node_value(buddy, bud) == NODE_FREE) {
			// Mark these two blocks as used
			buddy_set_node_value(buddy, node, NODE_USED_INDIRECT);
			buddy_set_node_value(buddy, bud, NODE_USED_INDIRECT);
			
			// Mark the parent as free
			uint32_t parent = buddy_get_parent(node);
			buddy_set_node_value(buddy, parent, NODE_FREE);
			
			// If the physical page corresponding to the virtual page we had is now completely free,
			// we can unmap the virtual page
			if (use_vadr) {
				if ((page_node == node || page_node == bud || page_node == parent ) && !unmapped) {
					// Unmap all pages associated with this
					uint32_t z;
					for (z = 0; z < NUMBER_OF_PAGES; z++) {
						uint32_t test_vaddr = z * PAGE_SIZE;
						if (vm_is_page_mapped(test_vaddr) && vm_virtual_to_physical(test_vaddr))
							vm_unmap_page((uint32_t)vaddr, false);
					}
					unmapped = true;
				}
			}
			
			node = parent;
		} else {
			// If we can't merge these two blocks, then we can't keep going, so we stop
			break;
		}
	}
	
	up(&buddy_lock);
}
