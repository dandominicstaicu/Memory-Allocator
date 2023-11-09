#pragma once

#define ALIGNMENT 8
#define MMAP_THRESHOLD (128*1024)

#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define BLOCK_SIZE (ALIGN(sizeof(struct block_meta)))
