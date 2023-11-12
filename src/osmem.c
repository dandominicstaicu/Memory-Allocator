// SPDX-License-Identifier: BSD-3-Clause

#include "osmem.h"
#include "printf.h"
#include "os_utils.h"
#include "block_meta.h"
#include "alloc_helpers.h"
#include "block_meta_list.h"

struct block_meta *head;
struct block_meta *tail;

void *heap_end;

size_t blk_meta_size = BLOCK_SIZE;

int first_brk_alloc;

void *os_alloc_helper(size_t blk_size, size_t threshold, int zero)
{
	// Return NULL for a request of zero size
	if (blk_size == 0)
		return NULL;

	// Determine if the requested size exceeds the threshold
	int isLargeBlock = (blk_size + blk_meta_size) >= threshold;

	// Allocate memory based on block size
	void *allocated_mem = NULL;

	if (isLargeBlock) {
		// Allocate using mmap for large blocks
		allocated_mem = mmap_alloc(blk_size);

		// Optionally zero out the memory for calloc
		if (zero && allocated_mem)
			memset(allocated_mem, 0, blk_size);

	} else {
		// Handle small block allocations
		if (first_brk_alloc == 0) {
			allocated_mem = first_heap_alloc(MMAP_THRESHOLD);
			if (zero && allocated_mem)
				memset(allocated_mem, 0, blk_size);

		} else {
			// Search for a suitable free block
			struct block_meta *new_block = find_block_with_size(head, blk_size, blk_meta_size);

			// Allocate a new block if no suitable free block is found
			if (!new_block)
				new_block = new_heap(blk_size);

			allocated_mem = get_addr_from_blk(new_block, blk_meta_size);

			// Zero out memory for calloc
			if (zero && allocated_mem)
				memset(allocated_mem, 0, blk_size);
		}
	}

	return allocated_mem;
}

void *os_malloc(size_t size)
{
	int calloc = 0;

	size_t alginment = ALIGN(size);

	return os_alloc_helper(alginment, MMAP_THRESHOLD, calloc);
}

void os_free(void *ptr)
{
	// Ignore freeing if the pointer is NULL
	if (!ptr)
		return;

	// Retrieve the metadata block for the given memory address
	struct block_meta *block_to_free = get_block_from_addr(ptr, blk_meta_size);

	// Check the status of the block and perform the appropriate free operation
	switch (block_to_free->status) {
	case STATUS_ALLOC:
		// Mark the block as free if it was previously allocated
		block_to_free->status = STATUS_FREE;
		break;

	case STATUS_MAPPED:
		// Remove the block from the list and unmap it if it was mapped
		remove_from_list(&head, &tail, block_to_free);
		if (munmap((void *) block_to_free, block_to_free->size + blk_meta_size) == -1) {
			fprintf(stderr, "Error during munmap in free\n");
			exit(EXIT_FAILURE);
		}
		break;

	default:
		break;
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
	// Handle NULL pointer case
	if (!ptr)
		return os_malloc(size);

	// Handle case where size is zero
	if (size == 0) {
		os_free(ptr);
		return NULL;
	}

	struct block_meta *block = get_block_from_addr(ptr, blk_meta_size);
	size_t new_size = ALIGN(size);

	// Return NULL if the block is already free
	if (block->status == STATUS_FREE)
		return NULL;

	// Handle large sizes or mapped blocks
	if (new_size + blk_meta_size >= MMAP_THRESHOLD || block->status == STATUS_MAPPED) {
		void *new_block_ptr = os_malloc(new_size);
		size_t copy_size = block->size < new_size ? block->size : new_size;

		memcpy(new_block_ptr, ptr, copy_size);
		os_free(ptr);
		return new_block_ptr;
	}

	// Handle resizing within the same block
	if (new_size <= block->size) {
		if (new_size < block->size)
			block = split_blk(block, new_size, blk_meta_size);

		return get_addr_from_blk(block, blk_meta_size);
	}

	// Try expanding the block in place
	struct block_meta *expanded_block = expand_realloc(head, block, new_size, blk_meta_size);

	if (expanded_block)
		return get_addr_from_blk(expanded_block, blk_meta_size);

	// Allocate a new block and copy data if in-place expansion is not possible
	void *new_block_ptr = os_malloc(size);

	memcpy(new_block_ptr, ptr, block->size);
	os_free(ptr);
	return new_block_ptr;
}
