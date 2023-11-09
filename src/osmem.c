// SPDX-License-Identifier: BSD-3-Clause

#include "osmem.h"
#include "printf.h"
#include "os_utils.h"
#include "block_meta.h"
#include "alloc_helpers.h"
#include "block_meta_list.h"

struct block_meta *head;

size_t blk_meta_size = BLOCK_SIZE;

int first_brk_alloc;

void *os_alloc_helper(size_t blk_size, size_t threshold, int zero) {
	// Return NULL for a request of zero size
	if (blk_size == 0) {
		return NULL;
	}

	// Determine if the requested size exceeds the threshold
	int isLargeBlock = (blk_size + blk_meta_size) >= threshold;

	// Allocate memory based on block size
	void *allocated_mem = NULL;
	if (isLargeBlock) {
		// Allocate using mmap for large blocks
		allocated_mem = mmap_alloc(blk_size);

		// Optionally zero out the memory for calloc
		if (zero && allocated_mem) {
			memset(allocated_mem, 0, blk_size);
		}
	} else {
		// Handle small block allocations
		if (first_brk_alloc == 0) {
			allocated_mem = first_heap_alloc(MMAP_THRESHOLD);
			if (zero && allocated_mem) {
				memset(allocated_mem, 0, blk_size);
			}
		} else {
			// Search for a suitable free block
			struct block_meta *new_block = find_block_with_size(head, blk_size, blk_meta_size);

			// Allocate a new block if no suitable free block is found
			if (!new_block) {
				new_block = new_heap(blk_size);
			}

			allocated_mem = get_addr_from_blk(new_block, blk_meta_size);

			// Zero out memory for calloc
			if (zero && allocated_mem) {
				memset(allocated_mem, 0, blk_size);
			}
		}
	}

	return allocated_mem;
}


void *os_malloc(size_t size)
{
	int calloc = 0;

	size_t alginment = ALIGN(size);
	// printf("align: %u\n", alginment);
	return os_alloc_helper(alginment, MMAP_THRESHOLD, calloc);
}

void os_free(void *ptr)
{
	// If try to free a NULL pointer, don't do anything
	if (ptr == NULL)
		return;

	// Get the block at the given address
	struct block_meta *current = (struct block_meta *) (((char *) ptr) - blk_meta_size);

	if (current->status == STATUS_ALLOC) {
		current->status = STATUS_FREE;
	} else if (current->status == STATUS_MAPPED) {
		remove_from_list(&head, current);
		int return_val = munmap((void *) current, current->size + blk_meta_size);

		DIE(return_val == -1, "Error at munmap in os_free\n");
	}
}

void *os_calloc(size_t nmemb, size_t size)
{
	int calloc = 1;
	size_t total = ALIGN(nmemb * size);

	return os_alloc_helper(total, getpagesize(), calloc);
}

void *os_realloc(void *ptr, size_t size)
{
	/* TODO: Implement os_realloc */
	return NULL;
}
