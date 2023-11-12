#include "alloc_helpers.h"

extern struct block_meta *head;
extern struct block_meta *tail;

extern void* heap_end;

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

	add_in_list(&head, &tail, new_block, STATUS_MAPPED);

	return get_addr_from_blk(new_block, blk_meta_size);
}

void *first_heap_alloc(size_t threshold)
{
	// Mark the first heap allocation is being done
	first_brk_alloc = 1;

	// Do the prealloc
	void *ret_addr = sbrk(threshold);
	heap_end = ret_addr;

	// Check if sbrk failed
	DIE(ret_addr == ((void *) -1), "Error at sbrk in heap alloc\n");

	struct block_meta *new_block = (struct block_meta *)ret_addr;

	config_meta(new_block, threshold - blk_meta_size, STATUS_ALLOC);

	add_in_list(&head, &tail, new_block, STATUS_ALLOC);

	// Return the address of the allocated block
	return get_addr_from_blk(new_block, blk_meta_size);
}

struct block_meta *new_heap(size_t blk_size)
{
	// Alloc the size that we need on the heap
	void *ret_addr = sbrk(blk_size + blk_meta_size);
	heap_end = (char *)heap_end + blk_size + blk_meta_size;

	// Check if sbrk failed
	DIE(ret_addr == ((void *) -1), "Error at sbrk in alloc new heap\n");

	struct block_meta *new_block = (struct block_meta *)ret_addr;

	config_meta(new_block, blk_size, STATUS_ALLOC);

	add_in_list(&head, &tail, new_block, STATUS_ALLOC);

	return new_block;
}

void *get_addr_from_blk(struct block_meta *block, size_t meta_size)
{
	// The memory block starts right after the metadata
	return (void *)((char *)block + meta_size);
}

struct block_meta *get_block_from_addr(void *ret_addr, size_t loc_blk_meta_size)
{
	// The metadata starts right before the memory block
	struct block_meta *ret = (struct block_meta *)((char *)ret_addr - loc_blk_meta_size);
	return ret;
}

struct block_meta *expand_realloc(struct block_meta *head, struct block_meta *init, size_t req_size, size_t loc_blk_meta_size)
{
	if (!head) {
		// List is empty, nothing to do
		return NULL;
	}

	struct block_meta *current = head;

	// Traverse the list starting from the 'init' block
	while (current != NULL) {
		// Check if the current block is the one we want to expand
		if (current == init) {
			// Check if the next block is free and can be merged
			while (current->next && current->next->status == STATUS_FREE) {
				current->size += current->next->size + loc_blk_meta_size;
				struct block_meta *next = current->next;
				current->next = next->next;
				if (next->next && current != head) {
					next->next->prev = current;
				}

				// Check if the block is now big enough
				if (current->size >= req_size) {
					return split_blk(current, req_size, loc_blk_meta_size);
				}
			}

			break;
		}

		// Move to the next block in the list
		current = current->next;
	}

	// If 'init' is the last block, try to expand the heap
	if (init->next == NULL) {
		if (tail == heap_end)
			return NULL;

		void *ret_addr = sbrk(req_size - current->size);
		DIE(ret_addr == (void *)-1, "Error at sbrk in expand_realloc\n");

		heap_end = (char *)heap_end + (req_size - current->size);

		config_meta(init, req_size, STATUS_ALLOC);
		return init;
	}

	return NULL; // Expansion not possible
}

size_t get_available_heap_space() {
    size_t total_free_space = 0;
    struct block_meta *current = head;  // Assuming heap_start points to the start of your heap

    while (current != NULL) {
        if (current->status == STATUS_FREE) {
            total_free_space += current->size;
        }
        current = current->next;  // Assuming each block_meta has a pointer to the next block
    }

    return total_free_space;
}

size_t get_block_count() {
    size_t count = 0;
    struct block_meta *current = head;

    while (current != NULL) {
        count++;
        current = current->next;
    }

    return count;
}

size_t get_largest_free_block_size() {
    struct block_meta *current = head;
    size_t max_size = 0;

    while (current != NULL) {
        if (current->status == STATUS_FREE && current->size > max_size) {
            max_size = current->size;
        }
        current = current->next;
    }

    return max_size; // Returns 0 if no free block is found
}

size_t get_used_space() {
    size_t total_used = 0;
    struct block_meta *current = head;
    
    while (current != NULL) {
        if (current->status != STATUS_FREE) {
            total_used += current->size;
        }
        current = current->next;
    }

    return total_used;
}

size_t get_current_heap_size() {
    if (head == NULL || tail == NULL) {
        return 0; // Heap is empty
    }

    // Assuming each block_meta includes the size of the block it represents
    size_t heap_size = 0;
    struct block_meta *current = head;
    while (current != NULL) {
        heap_size += current->size + sizeof(struct block_meta);
        current = current->next;
    }

    return heap_size;
}
