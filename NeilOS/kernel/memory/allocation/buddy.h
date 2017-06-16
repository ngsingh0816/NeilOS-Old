//
//  buddy.h
//  MemoryAllocator
//
//  Created by Neil Singh on 12/21/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#ifndef buddy_h
#define buddy_h

#include <common/types.h>

// Get the log of a power of two (note: only works for powers of two)
static inline uint32_t log2(uint32_t val) {
	uint32_t z;
	for (z = 0; z < 32; z++) {
		if ((val >> z) & 0x1)
			return z;
	}
	
	// log 0 doesn't work
	return -1;
}

// Clear the buddy tree
static inline void buddy_clear_tree(uint8_t* b, uint32_t size) {
	int z;
	for (z = 0; z < size; z++)
		b[z] = 0xAA;
}

// All these operations are done node space (not bit space)

// Get the left child for a node
static inline uint32_t buddy_get_left_child(uint32_t node) {
	return (node << 1) + 1;
}

// Get the right child for a node
static inline uint32_t buddy_get_right_child(uint32_t node) {
	return (node << 1) + 2;
}

// Returns if a node is the right child
static inline bool buddy_node_is_right_child(uint32_t node) {
	return !(node & 0x1);
}

// Get the parent for a node
static inline uint32_t buddy_get_parent(uint32_t node) {
	return (node - 1) >> 1;
}

// Get the number of nodes in a specific level
static inline uint32_t buddy_get_number_of_nodes_in_level(uint32_t level) {
	if (level == 0)
		return 1;
	return (2 << (level - 1));
}

// Get the buddy node for a given node
static inline uint32_t buddy_get_buddy_for_node(uint32_t node) {
	if (buddy_node_is_right_child(node))
		return node - 1;
	else
		return node + 1;
}

// Get the nth node at a specific level
static inline uint32_t buddy_get_indexed_node_at_level(uint32_t level, uint32_t n) {
	if (level == 0)
		return 0;
	return n + ((2 << (level - 1)) - 1);
}

// Get the index of a node in its level
static inline uint32_t buddy_get_index_of_node_at_level(uint32_t node, uint32_t level) {
	if (level == 0)
		return 0;
	return node + 1 - (2 << (level - 1));
}

// All these operations take inputs for node space but perform actions in bit space

// Get the value of a node
static inline uint8_t buddy_get_node_value(uint8_t* b, uint32_t node) {
	return (b[node >> 2] >> ((node & 0x3) << 1)) & 0x3;
}

// Set the value of a node
static inline void buddy_set_node_value(uint8_t* b, uint32_t node, bool val) {
	const uint32_t v = ((node & 0x3) << 1);
	const uint32_t n = node >> 2;
	
	b[n] = (b[n] & ~(0x3 << v)) | (val << v);
}

#endif /* buddy_h */
