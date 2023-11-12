// SPDX-License-Identifier: BSD-3-Clause

#include "block_meta_list.h"

extern struct block_meta *tail;

extern void *heap_end;

void add_in_list(struct block_meta **head, struct block_meta **tail, struct block_meta *new_block, int status)
{
	// Insertion when head is NULL
	if (*head == NULL) {
		*head = new_block;
		*tail = new_block;
		new_block->prev = NULL;
		new_block->next = NULL;
		return;
	}

	// Insertion when head is STATUS_MAPPED
	if ((*head)->status == STATUS_MAPPED) {
		if (status == STATUS_MAPPED) {
			// Inserting another STATUS_MAPPED after head
			new_block->next = (*head)->next;
			new_block->prev = *head;

			// Update the next block's prev
			if ((*head)->next != NULL)
				(*head)->next->prev = new_block;
			else
				*tail = new_block; // Update tail if we're at the end

		} else {
			// Inserting a STATUS_ALLOC before head
			new_block->next = *head;
			new_block->prev = NULL;
			(*head)->prev = new_block;
			*head = new_block; // Update head
		}
		return;
	}

	struct block_meta *current = *head;

	if (status == STATUS_MAPPED) {
		// Find the last node in the list to add the STATUS_MAPPED block
		while (current->next != NULL)
			current = current->next;


		current->next = new_block;
		new_block->prev = current;
		new_block->next = NULL;
		*tail = new_block; // Update tail
	} else if (status == STATUS_ALLOC) {
		// Find the position to insert the STATUS_ALLOC block before any STATUS_MAPPED block
		while (current->next != NULL && current->next->status != STATUS_MAPPED)
			current = current->next;

		new_block->next = current->next;
		new_block->prev = current;
		current->next = new_block;

		// Update the next node's prev and possibly the tail
		if (current->next->next != NULL)
			current->next->next->prev = new_block;
		else
			*tail = new_block; // Update tail if we're at the end
	}
}


struct block_meta *find_block_with_size(struct block_meta *head, size_t needed_size, size_t loc_blk_meta_size)
{
	// Return NULL if the list is empty
	if (!head)
		return NULL;

	// Merge consecutive free blocks
	for (struct block_meta *current = head; current && current->next; ) {
		if (current && current->next) {
			if (current->status == STATUS_FREE && current->next->status == STATUS_FREE) {
				// Merge current and next blocks
				current->size += current->next->size + loc_blk_meta_size;
				current->next = current->next->next;
			} else {
				// Move to the next block
				current = current->next;
			}
		}
	}

	// Find the best fitting block
	struct block_meta *best_fit = NULL;

	for (struct block_meta *current = head; current; current = current->next) {
		if (current->status == STATUS_FREE && current->size >= needed_size) {
			if (!best_fit || current->size < best_fit->size)
				best_fit = current;
		}
	}

	if (best_fit) {
		// Allocate the best fitting block
		best_fit->status = STATUS_ALLOC;
		return split_blk(best_fit, needed_size, loc_blk_meta_size);
	}

	// Attempt to expand the last block if it's free
	struct block_meta *last_block = get_last_brk_blk(head);

	if (last_block && last_block->status == STATUS_FREE) {
		void *expansion = sbrk(needed_size - last_block->size);

		if (expansion == (void *) -1)
			return NULL; // Expansion failed

		heap_end = (char *)heap_end + (needed_size - last_block->size);

		// TODO check if this is correct
		tail = (struct block_meta *)(tail + (needed_size - last_block->size));

		set_meta(last_block, needed_size, STATUS_ALLOC);
		return last_block;
	}

	// No suitable block found
	return NULL;
}

struct block_meta *split_blk(struct block_meta *initial, size_t req_size, size_t loc_blk_meta_size)
{
	// Ensure the block is large enough to be split
	int large_block = initial->size >= req_size + ALIGN(1) + loc_blk_meta_size;

	if (large_block) {
		// Create a new block at the position after the needed size in the initial block
		char *new_block_addr = (char *)initial + req_size + loc_blk_meta_size;
		struct block_meta *new_block = (struct block_meta *)(new_block_addr);

		// Configure the new block
		new_block->size = initial->size - req_size - loc_blk_meta_size;
		new_block->status = STATUS_FREE;
		new_block->next = initial->next;
		new_block->prev = initial; // Set the previous pointer to the initial block

		// Update the initial block
		initial->size = req_size;
		initial->status = STATUS_ALLOC;
		initial->next = new_block;

		// Update the previous pointer of the next block in the list, if it exists
		if (new_block->next)
			new_block->next->prev = new_block;
	}

	// Return the initial block, now resized
	return initial;
}

struct block_meta *get_last_brk_blk(struct block_meta *head)
{
	struct block_meta *current = head;

	// Iterating through the list to locate the last block before any mmaped block
	while (current->next) {
		// If the next block is mmaped, we have reached the last non-mmaped block
		if (current->next && current->next->status == STATUS_MAPPED)
			return current; // This is the last block managed by brk/sbrk

		current = current->next;
	}

	// Check if the found block is either allocated or free
	// If so, this block is the last block managed by brk/sbrk system
	if (current && (current->status == STATUS_ALLOC || current->status == STATUS_FREE))
		return current;  // Return the last brk/sbrk managed block

	// If no such block is found, return NULL
	return NULL;
}

void remove_from_list(struct block_meta **head, struct block_meta **tail, struct block_meta *current)
{
	// If the list is empty or current is NULL, there's nothing to delete
	if (*head == NULL || current == NULL)
		return;

	// If the node to be deleted is the head node
	if (*head == current) {
		*head = current->next;
		// If there's a next node, update its prev pointer
		if (current->next != NULL) {
			current->next->prev = NULL;
		} else {
			// If there's no next node, then the list is now empty
			*tail = NULL;
		}
	} else if (*tail == current) { // If the node to be deleted is the tail node
		*tail = current->prev;
		if (current->prev != NULL)
			current->prev->next = NULL;

	} else { // If the node is in the middle of the list
		current->prev->next = current->next;
		if (current->next != NULL)
			current->next->prev = current->prev;
	}
}
