#include "pmm.h"

extern uint32_t end;

static uint32_t *pmm_bitmap = 0;
static uint32_t num_of_blocks = 0;
static uint32_t used_blocks = 0;

static uint32_t bitmap_end = 0;

void init_block_used(const uint32_t add)
{

}

void init_block_free(const uint32_t add)
{

}

void init_region_used(const uint32_t start_add, const uint32_t end_add)
{
    
}

void init_region_free(const uint32_t start_add, const uint32_t end_add) 
{

}

void init_physical_memory(const uint32_t add, const uint32_t size)
{
    pmm_bitmap = (uint32_t*) add;
    num_of_blocks = size / BLOCK_SIZE;
    used_blocks = num_of_blocks;
    memset(pmm_bitmap, 0xFF, num_of_blocks / BLOCKS_PER_BYTE);

    uint32_t align = end / BLOCK_SIZE;  // Convert memory address to blocks

    if (end % BLOCK_SIZE)
        align = (align & BLOCK_SIZE) + BLOCK_SIZE;
    
    uint32_t num_blocks = num_of_blocks - align;          // Get number of free blocks

    for (; num_blocks > 0; num_blocks--) { // setting unused blocks to free
        init_block_free(align++);
        used_blocks--;
    }
}

uint32_t allocate_block()
{
    allocate_blocks(1);
}

void free_block(const uint32_t address)
{
    free_blocks(address, 1);
}

uint32_t allocate_blocks(const uint32_t num_blocks)
{
    if (num_blocks == 0 || ((num_of_blocks - used_blocks) < num_blocks)) return -1; // Can't return no memory, error
    uint32_t address = 0;

    // Test 32 blocks at a time
    for (uint32_t i = 0; i < max_blocks / 32 && !address;  i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            // At least 1 bit is not set within this 32bit chunk of memory,
            //   find that bit by testing each bit
            for (uint32_t j = 0; j < 32; j++) {
                uint32_t bit = 1 << j;


                // If bit is unset/0, found start of a free region of memory
                if (!(pmm_bitmap[i] & bit)) {
                    // Checking every bit after the one found free, looking for the amount of blocks needed
                    for (uint32_t count = 0, free_blocks = 0, index = 0; count < num_blocks && !address; count++) {
                        index = count / 32;
                            if (!(pmm_bitmap[i+index] & (1 << ((j + count) - (32 * index)))))
                                free_blocks++;
                        

                        if (free_blocks == num_blocks) // Found enough free space
                            address = i*32 + j;
                    }
                }
            }
        }
    }

    if(address) 
    {
        for (uint32_t n = 0; n < num_blocks; n++)
        {
            init_block_used(address + n); // setting the blockes allocated to used in bitmap
        }
        return address;
    }
    else
        return -1;  // No free region of memory large enough
}

void free_blocks(const uint32_t address, const uint32_t num_blocks)
{
    uint32_t starting_block = (uint32_t)address / BLOCK_SIZE;   // Convert address to blocks 

    for (uint32_t i = 0; i < num_blocks; i++) 
        init_block_free(starting_block + i);    // Unset bits/blocks in memory map, to free

    used_blocks -= num_blocks;  // Decrease used block count
}