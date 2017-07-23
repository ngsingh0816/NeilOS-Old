//
//  heap.c
//  MemoryAllocator
//
//  Created by Neil Singh on 12/12/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#include "heap.h"
#include "buddy.h"
#include <common/lib.h>
#include "page_allocator.h"
#include <memory/memory.h>

// (4 * 1024 * 1024) / (12 + 65  / 64 * (1024 * 1024 / 16))
// for current implementation gives 63 heaps out of 63.007 possible
// so it wastes the least space.
// Note: that 8 actually could be changed depending on how the struct is packed
// (on a 64-bit Mac it is 16 when it actually could be 12)
// If we use a 1GB MAX_SIZE and 64 KB MIN_SIZE for the page frame allocator, the buddy
// system is exactly 8192 bytes or 2 4KB pages!!

#define MIN_SIZE		32									// Min allocation size
#define	MAX_SIZE		(1024 * 1024 / 16)					// Max allocation size
#define BUDDY_SIZE		(2 * (MAX_SIZE / MIN_SIZE) / 4)		// Number of nodes in the buddy tree
															// Should be 2^(h+1) - 1 but we include 1 extra node to make it
															// divisible by 4 always
#define HEAP_BLOCK_SIZE	(MAX_SIZE + sizeof(heap_block_t))	// Number of bytes occupied by 1 heap block
#define NUM_HEAPS		(4096 * 1024 / HEAP_BLOCK_SIZE)		// Number of heaps that can fit in one block

// Values for buddy nodes
#define NODE_FREE			0								// Free for use
#define NODE_USED			1								// Not free for use and directly allocated
#define NODE_USED_INDIRECT	2								// Not free for use but indirectly allocated

// Linked list of head nodes creates a heap
typedef struct heap_block_t {
	// Next heap block
	struct heap_block_t* next;
	struct heap_block_t* prev;
	
	// Information about this heap node
	uint32_t space_used;
	
	// The buddy bitmap - full, complete binary tree (root specifies whole space, immediate children divide that in half)
	uint8_t buddy[BUDDY_SIZE];
	
	// This is where the free memory starts (MAX_SIZE bytes)
} heap_block_t;

// The starting address of the heap
heap_block_t* heap = NULL;

// Returns a pointer to newly allocated memory or NULL if it cannot be allocated
void* kmalloc(uint32_t real_size) {
	// TODO: we are wasting a lot of memory, but doing this breaks things
	/*if (real_size == 0)
		return NULL;*/
	
	uint32_t original_size = real_size;
	
	// Needed for the extra byte of flags (but align it to 4 for fastness)
	real_size += sizeof(uint32_t);
	
	// Don't support anything bigger than the max size
	if (real_size > MAX_SIZE) {
		// Try and allocate a page for this
		uint32_t new_size = (real_size - 1) / MEMORY_PAGE_SIZE + 1;
		uint32_t* ret = page_get(new_size, VIRTUAL_MEMORY_KERNEL);
		if (!ret)
			return NULL;
		// Mark this as using a page instead of kmalloc
		*(ret++) = 1;
		return (void*)ret;
	}
	
	// Don't allow anything smaller than the min size
	if (real_size < MIN_SIZE)
		real_size = MIN_SIZE;
	
	// Find the smallest size that is a power of 2 that is bigger or equal to this
	uint32_t size = MAX_SIZE;
	int32_t level = 0;
	while (real_size <= size) {
		size >>= 1;
		level++;
	}
	size <<= 1;
	level--;
	
	// Loop through all available heap blocks
	heap_block_t* heap_block = heap;
	heap_block_t* prev_block = NULL;
	for (heap_block = heap; heap_block != NULL; heap_block = heap_block->next) {
		// Cache the last block
		prev_block = heap_block;
		
		// Find the first available node by looping through all the levels
		uint8_t* buddy = heap_block->buddy;
		int32_t l;
		uint32_t node = 0, node_index = 0;
		bool found = false;
		for (l = level; l >= 0; l--) {
			// Loop through all the nodes in this level
			int z, num_nodes = buddy_get_number_of_nodes_in_level(l);
			for (z = 0; z < num_nodes; z++) {
				// Check if this node is free
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
		
		// If we couldn't find a node, go on to the next one
		if (!found)
			continue;
		
		heap_block->space_used += size;
		
		// This node may have a size that is too big for us, so we may need to
		// split it up into smaller section
		uint32_t node_size = MAX_SIZE >> l;
		if (node_size == size) {
			// If this size is an exact match, just set the node as in use and return
			buddy_set_node_value(buddy, node, NODE_USED);
			uint32_t* ret =  (uint32_t*)((uint32_t)heap_block + sizeof(heap_block_t) + node_index * node_size);
			// Mark this as using kmalloc, not a page
			*(ret++) = 0;
			return (void*)ret;
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
		uint32_t* ret = (uint32_t*)((uint32_t)heap_block + sizeof(heap_block_t) +
							buddy_get_index_of_node_at_level(left_node, level) * size);
		// Mark as using kmalloc and not a page
		*(ret++) = 0;
		return (void*)ret;
	}
	
	// None could be found, so try to allocate a new heap block in physical memory
	heap_block_t* new_heap = (heap_block_t*)page_get_four_mb(1, VIRTUAL_MEMORY_KERNEL);
	// If we couldn't get more memory, give up
	if (!new_heap)
		return NULL;
	
	// Set the heap starting point
	new_heap->next = NULL;
	new_heap->prev = NULL;
	
	// Put as many heap blocks as you can in the 4MB page
	heap_block_t* hnode = new_heap;
	int z;
	for (z = 0; z < NUM_HEAPS - 1; z++) {
		hnode->next = (heap_block_t*)((uint8_t*)hnode + HEAP_BLOCK_SIZE);
		hnode->next->next = NULL;
		hnode->next->prev = hnode;
		
		hnode = hnode->next;
	}
	
	// Initialize the buddy system
	hnode = new_heap;
	while (hnode) {
		// Set only the root to free
		buddy_clear_tree(hnode->buddy, BUDDY_SIZE);
		buddy_set_node_value(hnode->buddy, 0, NODE_FREE);
		
		// Go to the next heap block
		hnode = hnode->next;
	}
	
	// Link this new heap into the heap list
	if (!prev_block)
		heap = new_heap;
	else {
		prev_block->next = new_heap;
		new_heap->prev = prev_block;
	}
	
	// Try again (this isn't recursion because it will happen a max of one extra time)
	return kmalloc(original_size);
}

// Free allocated memory and combine blocks
void kfree(void* addr) {
	// Check if we used a page or kmalloc
	uint32_t* real_addr = addr;
	if (*(--real_addr) == 1)
		return page_free(real_addr);
	addr = (void*)real_addr;
	
	// Figure out which heap block this address is in
	heap_block_t* heap_block;
	uint32_t naddr = 0;
	bool foundAddress = false;
	for (heap_block = heap; heap_block != NULL; heap_block = heap_block->next) {
		// Find the node address for this memory address
		naddr = (uint32_t)addr - (uint32_t)heap_block - sizeof(heap_block_t);
		// Make sure it is possible
		if (naddr < MAX_SIZE) {
			foundAddress = true;
			break;
		}
	}
	
	// It is not possible for an allocated pointer to not be aligned to our min node size
	if (!foundAddress || (naddr % MIN_SIZE) != 0) {
		printf("Error (1): pointer (0x%x) being freed was not allocated.\n", addr);
		return;
	}
	
	uint8_t* buddy = heap_block->buddy;
	// Many blocks could have this address but only one can be in use (and all below it are in use too)
	// Find the smallest size block and work our way up to the one that
	uint32_t size = MIN_SIZE;
	uint32_t node = buddy_get_indexed_node_at_level(log2(MAX_SIZE / MIN_SIZE), naddr / MIN_SIZE);
	
	// If this node is not in use, then nothing could have been allocated to this spot
	if (buddy_get_node_value(buddy, node) == NODE_FREE) {
		printf("Error (2): pointer (0x%x) being freed was not allocated.\n", addr);
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
		if (buddy_get_node_value(buddy, 0) == NODE_USED && naddr == 0) {
			// The top node was the one allocated, so free it
			buddy_set_node_value(buddy, 0, NODE_FREE);
			heap_block->space_used -= MAX_SIZE;
			
			// We just released all the space in the heap node, so there shouldn't be anything to combine
			return;
		}
		
		// We couldn't find anything allocated
		printf("Error (3): pointer (0x%x) being freed was not allocated.\n", addr);
		return;
	}
	
	// Mark the node as free and let the heap know about the space that was freed up
	buddy_set_node_value(buddy, node, NODE_FREE);
	heap_block->space_used -= size;
	if (heap_block->space_used == 0) {
		// If there is nothing being used in this heap, we can remove it from the heap block list
		// Get the 4MB aligned block this heap resides in
		uint32_t faddr = (uint32_t)heap_block & ~(0x3FFFFF);
		heap_block_t* hb = NULL;
		bool found = true;
		for (hb = (heap_block_t*)faddr; hb != NULL; hb = hb->next) {
			// Check that all the heaps are free
			if (hb->space_used != 0) {
				found = false;
				break;
			}
		}
		// This heap is completely empty, remove it
		if (found) {
			heap_block_t* h = (heap_block_t*)(faddr);
			heap_block_t* n = ((heap_block_t*)(faddr + (NUM_HEAPS - 1) * HEAP_BLOCK_SIZE));
			// Unlink this heap
			if (h->prev)
				h->prev->next = n->next;
			if (n->next)
				n->next->prev = n->prev;
			
			// Update our head pointer if needed
			if (h == heap)
				heap = n->next;
			
			// Free the heap memory
			page_free(h);
			return;
		}
	}
	
	// Iteratively merge the blocks if we can
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
			node = parent;
		} else {
			// If we can't merge these two blocks, then we can't keep going, so we stop
			break;
		}
	}
}
