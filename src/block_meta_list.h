#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>

#include "block_meta.h"
#include "os_utils.h"
#include "alloc_helpers.h"

void add_in_list(struct block_meta **head, struct block_meta **tail, struct block_meta *new_block, int status);

void remove_from_list(struct block_meta **head, struct block_meta **tail, struct block_meta *current);

struct block_meta *find_block_with_size(struct block_meta *head, size_t needed_size, size_t loc_blk_meta_size);

struct block_meta *get_last_brk_blk(struct block_meta *head);

struct block_meta *split_blk(struct block_meta *initial, size_t needed_size, size_t loc_blk_meta_size);
