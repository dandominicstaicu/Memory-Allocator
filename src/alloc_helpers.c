#include "alloc_helpers.h"

// TODO separate file for list operations

extern struct block_meta *head;

extern int first_brk_alloc;

extern size_t blk_meta_size;

void config_meta(struct block_meta *new_block, size_t size, int status)
{
	new_block->size = size;
	new_block->status = status;
	new_block->next = NULL;
	new_block->prev = NULL;
}

void *mmap_alloc(size_t blk_size)
{
	void *mem = mmap(NULL, blk_size + blk_meta_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	DIE(mem == ((void *) -1), "Error at mmap in alloc\n");

	struct block_meta *new_block = (struct block_meta *)mem;

	config_meta(new_block, blk_size, STATUS_MAPPED);

	add_in_list(&head, new_block, STATUS_MAPPED);

	return get_addr_from_blk(new_block, blk_meta_size);
}

void *first_heap_alloc(size_t threshold)
{
	// Mark the first heap allocation is being done
	first_brk_alloc = 1;

	// Do the prealloc
	void *ret_addr = sbrk(threshold);

	// Check if sbrk failed
	DIE(ret_addr == ((void *) -1), "Error at sbrk in heap alloc\n");

	struct block_meta *new_block = (struct block_meta *)ret_addr;

	config_meta(new_block, threshold - blk_meta_size, STATUS_ALLOC);

	add_in_list(&head, new_block, STATUS_ALLOC);

	// Return the address of the allocated block
	return get_addr_from_blk(new_block, blk_meta_size);
}

struct block_meta *new_heap(size_t blk_size)
{
	// Alloc the size that we need on the heap
	void *ret_addr = sbrk(blk_size + blk_meta_size);

	// Check if sbrk failed
	DIE(ret_addr == ((void *) -1), "Error at sbrk in alloc new heap\n");

	struct block_meta *new_block = (struct block_meta *)ret_addr;

	config_meta(new_block, blk_size, STATUS_ALLOC);

	add_in_list(&head, new_block, STATUS_ALLOC);

	return new_block;
}

void *get_addr_from_blk(struct block_meta *block, size_t meta_size)
{
	// The memory block starts right after the metadata
	return (void *)((char *)block + meta_size);
}



