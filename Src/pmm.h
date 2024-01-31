#include "utils.h"
#include "Screen.h"

#define BLOCK_SIZE      4096
#define BLOCKS_PER_BYTE 8
#define MEM_SIZE        0xFFFFFFFF
#define BITMAP_SIZE     ((MEM_SIZE / BLOCK_SIZE) / BLOCKS_PER_BYTE)
#define START_BELOW_1MB 0

#define MEMORY_MAP_REGION_FREE 0x01
#define MEMORY_MAP_REGION_USED 0x02

#define ADDRESS_TO_BLOCK(addr)  (((uint32_t)addr) / BLOCK_SIZE)
#define BLOCK_TO_ADDRESS(block) (((uint32_t)block) * BLOCK_SIZE)
#define ALIGN_4KB_UP(addr)      (((uint32_t)addr & 0xFFFFF000) + 0x1000)
#define ALIGN_4KB_DOWN(addr)    (((uint32_t)addr & 0xFFFFF000))

void init_block_used(const uint32_t add);
void init_block_free(const uint32_t add);
void init_region_used(const uint32_t start_add, const uint32_t end_add);
void init_region_free(const uint32_t start_add, const uint32_t end_add);
void init_physical_memory(const uint32_t size);
uint32_t allocate_block();
void free_block(const uint32_t address);
uint32_t allocate_blocks(const uint32_t num_blocks);
void free_blocks(const uint32_t address, const uint32_t num_blocks);

void print_bitmap();