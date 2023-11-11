#pragma once

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "block_meta.h"
#include "os_utils.h"
#include "block_meta_list.h"

void *mmap_alloc(size_t blk_size);

void *first_heap_alloc(size_t threshold);

struct block_meta *new_heap(size_t blk_size);

void config_meta(struct block_meta *new_block, size_t size, int status);

void *get_addr_from_blk(struct block_meta *block, size_t meta_size);

struct block_meta *get_block_from_addr(void *ret_addr, size_t loc_blk_meta_size);

struct block_meta *expand_realloc(struct block_meta *head, struct block_meta *init, size_t req_size, size_t loc_blk_meta_size);

// struct block_meta *
// expandBlockRealloc(struct block_meta *head, struct block_meta *initial, size_t neededSize, size_t blkMetaSize);

size_t get_available_heap_space();

size_t get_block_count();

size_t get_largest_free_block_size();

size_t get_used_space();

size_t get_current_heap_size();
