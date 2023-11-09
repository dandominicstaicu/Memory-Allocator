#include "block_meta_list.h"

void add_in_list(struct block_meta **head, struct block_meta *new_block, int status)
{
	// Insertion when head is NULL
	if (*head == NULL) {
		*head = new_block;
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

			(*head)->next->prev = new_block;
		} else {
			// Inserting a STATUS_ALLOC before head
			new_block->next = *head;
			new_block->prev = NULL;
			(*head)->prev = new_block;
			*head = new_block;
		}
		return;
	}

	struct block_meta *current = *head;

	if (status == STATUS_MAPPED) {
		// Find the last node in the list to add the STATUS_MAPPED block
		while (current->next != NULL) {
			current = current->next;
		}

		current->next = new_block;
		new_block->prev = current;
		new_block->next = NULL;
	} else if (status == STATUS_ALLOC) {
		// Find the position to insert the STATUS_ALLOC block before
		// any STATUS_MAPPED block

		while (current->next != NULL && current->next->status != STATUS_MAPPED) {
			current = current->next;
		}

		// Insert STATUS_ALLOC block before the first STATUS_MAPPED block
		new_block->next = current->next;

		// Update the next node's prev
		if (current->next != NULL)
			current->next->prev = new_block;
	}

	new_block->prev = current;
	current->next = new_block;
}

struct block_meta *find_block_with_size(struct block_meta *head, size_t needed_size, size_t loc_blk_meta_size)
{
	// Return NULL if the list is empty
	if (!head) {
		return NULL;
	}

	// Merge consecutive free blocks
	for (struct block_meta *current = head; current && current->next; ) {
		if (current->status == STATUS_FREE && current->next->status == STATUS_FREE) {
			// Merge current and next blocks
			current->size += current->next->size + loc_blk_meta_size;
			current->next = current->next->next;
		} else {
			// Move to the next block
			current = current->next;
		}
	}

	// Find the best fitting block
	struct block_meta *best_fit = NULL;
	for (struct block_meta *current = head; current; current = current->next) {
		if (current->status == STATUS_FREE && current->size >= needed_size) {
			if (!best_fit || current->size < best_fit->size) {
				best_fit = current;
			}
		}
	}

	if (best_fit) {
		// Allocate the best fitting block
		best_fit->status = STATUS_ALLOC;
		return split_blk(best_fit, needed_size, loc_blk_meta_size);
	} else {
		// Attempt to expand the last block if it's free
		struct block_meta *last_block = get_last_brk_blk(head);
		if (last_block && last_block->status == STATUS_FREE) {
			void *expansion = sbrk(needed_size - last_block->size);
			if (expansion == (void *) -1) {
				return NULL; // Expansion failed
			}
			config_meta(last_block, needed_size, STATUS_ALLOC);
			return last_block;
		}
	}

	// No suitable block found
	return NULL;
}

struct block_meta *split_blk(struct block_meta *initial, size_t needed_size, size_t loc_blk_meta_size)
{
	// Determine if the block is large enough to be split
	// The block can be split if it's large enough to hold the requested size, 
	// the metadata for a new block, and at least one byte for the new block's data
	if (initial->size >= needed_size + ALIGN(1) + loc_blk_meta_size) {
		// Calculate the start address of the new block
		// This is done by moving forward from the initial block's start by the needed size and the metadata size
		struct block_meta *new_block = (struct block_meta *)((char *)initial + loc_blk_meta_size + needed_size);
		
		// Configure the new block
		// The size is the remaining size after deducting the needed size and the metadata size
		// The new block is marked as free and is linked to the initial block's next block
		new_block->size = initial->size - needed_size - loc_blk_meta_size;
		new_block->status = STATUS_FREE;
		new_block->next = initial->next;

		// Update the initial block
		// Its size is set to the needed size and it is marked as allocated
		// The next pointer is updated to point to the new block
		initial->size = needed_size;
		initial->status = STATUS_ALLOC;
		initial->next = new_block;
	}

	// Return the initial block, which is now the correctly sized allocated block
	return initial;
}

struct block_meta *get_last_brk_blk(struct block_meta *head)
{
	struct block_meta *current = head;

	// Traverse the linked list of blocks to find the last block before a mmaped block
	// The loop stops when it encounters a block that is followed by a mmaped block
	while (current->next != NULL) {
		if (current->next->status == STATUS_MAPPED) {
			break;  // Found a block followed by a mmaped block
		}
		current = current->next;
	}

	// Check if the found block is either allocated or free
	// If so, this block is the last block managed by brk/sbrk system
	if (current->status == STATUS_ALLOC || current->status == STATUS_FREE) {
		return current;  // Return the last brk/sbrk managed block
	}

	// If no such block is found, return NULL
	return NULL;
}

void remove_from_list(struct block_meta **head, struct block_meta *current)
{
	// Check if the list is empty
	if (*head == NULL || current == NULL) {
		return;
	}

	// If the node to be deleted is the head node
	if (*head == current) {
		*head = current->next;
	}

	// If the node to be deleted is NOT the last node, then change the next of previous node
	if (current->next != NULL) {
		current->next->prev = current->prev;
	}

	// If the node to be deleted is NOT the first node, then change the prev of next node
	if (current->prev != NULL) {
		current->prev->next = current->next;
	}
}
