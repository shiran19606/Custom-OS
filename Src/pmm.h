#include "utils.h"
#include "Screen.h"

#define BLOCK_SIZE      4096
#define BLOCKS_PER_BYTE 8
#define BITMAP_SIZE     ((MEM_SIZE / BLOCK_SIZE) / BLOCKS_PER_BYTE)
#define RESERVE_MEMORY_BELOW_1MB 1 //if true, it means we dont touch the memory under even if it is shown as free in the memory map.

#define MEMORY_MAP_REGION_FREE  0x01
#define MEMORY_MAP_REGION_USED  0x02

#define ADDRESS_TO_BLOCK(addr)  (((uint32_t)addr) / BLOCK_SIZE)
#define BLOCK_TO_ADDRESS(block) (((uint32_t)block) * BLOCK_SIZE)
#define ALIGN_4KB_UP(addr)      (((uint32_t)addr & 0xFFFFF000) + 0x1000)
#define ALIGN_4KB_DOWN(addr)    (((uint32_t)addr & 0xFFFFF000))

/*
function to set the bit in the bitmap that corresponds to the address given as parameter.
by setting the bit, we mark the block as used.
add: the address in memory of the block to set in the bitmap.
return value: none. it sets the bit in the bitmap.
*/
void init_block_used(const uint32_t add);

/*
function to clear the bit in the bitmap that corresponds to the address given as parameter.
by clearing the bit, we mark the block as free.
add: the address in memory of the block to free in the bitmap.
return value: none. it clears the bit in the bitmap.
*/
void init_block_free(const uint32_t add);

/*
functions that set an entire range of memory as freed/used
by setting/clearing the bits in the bitmap, we mark the region as free/used.
start_add: the starting address of the region.
end_add: the ending address of the region
if init_region_used takes start_add 0x127000 and end_add 0x129000, the blocks at 0x127000 and 0x128000 are marked as used in the bitmap, the block in 0x129000 is free because the end of the region is 0x129000.
return value: none. it sets/clears the bits in the bitmap.
*/
void init_region_used(const uint32_t start_add, const uint32_t end_add);
void init_region_free(const uint32_t start_add, const uint32_t end_add);

/*
this function takes as parameter the size of physical memory.
it divides it to BLOCKs of 4096 bytes each, stores the amount of blocks in maxBlocks, and initializes usedBlocks using maxBlocks (initially all blocks are used, we change that using the memory map).
sets pmm_bitmap as the start of the bitmap, after the end of the kernel.
stores bitmap_end as the end of the bitmap, 
*/
void init_physical_memory(const uint32_t size);

//allocate_block() expands to allocate_blocks(1), free_block(address) expands to free_blocks(address, 1)
uint32_t allocate_block();
void free_block(const uint32_t address);

/*
this function iterates through the bitmap, finds the first free num_blocks blocks, sets them as used and modifies usedBlocks, and then returns the address of the first block.
num_blocks: the amount of blocks to allocate
return value: the address of the first block allocated.
*/
uint32_t allocate_blocks(const uint32_t num_blocks);

/*
this function frees num_blocks blocks starting at address
address: the starting address of the blocks to free.
num_blocks: pretty self explanatory, the amount of blocks to free
return value: no return value. frees the blocks specified.
*/
void free_blocks(const uint32_t address, const uint32_t num_blocks);