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

void config_meta(struct block_meta *new_block, size_t size, int status);

void *first_heap_alloc(size_t threshold);

void *get_addr_from_blk(struct block_meta *block, size_t meta_size);

struct block_meta *new_heap(size_t blk_size);




