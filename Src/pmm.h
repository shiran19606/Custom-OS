#include "utils.h"
#define BLOCK_SIZE 4096
#define BLOCKS_PER_BYTE 8
#define MEM_SIZE 0xFFFFFFFF
#define BITMAP_SIZE ((MEM_SIZE / BLOCK_SIZE) / BLOCKS_PER_BYTE)

void init_block_used(const uint32_t add);
void init_block_free(const uint32_t add);
void init_region_used(const uint32_t start_add, const uint32_t end_add);
void init_region_free(const uint32_t start_add, const uint32_t end_add);
void init_physical_memory(const uint32_t add, const uint32_t size);
uint32_t allocate_block();
void free_block(const uint32_t address);
uint32_t allocate_blocks(const uint32_t num_blocks);
void free_blocks(const uint32_t address, const uint32_t num_blocks);