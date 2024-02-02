#include "pmm.h"

extern uint32_t kernel_end;

uint32_t* pmm_bitmap = 0;
uint32_t* bitmap_end = 0;
uint32_t maxBlocks = 0;
uint32_t usedBlocks = 0;

void init_block_used(const uint32_t add)
{
    uint32_t index = ADDRESS_TO_BLOCK(add);
    if (!pmm_bitmap)
        return;
    uint32_t word = index / (sizeof(uint32_t) * 8); // Calculate which word (integer) in the bitmap
	uint32_t bit = index % (sizeof(uint32_t) * 8); // Calculate which bit within the word

	pmm_bitmap[word] |= (1 << bit); // Set the bit by ORing with a mask
}

void init_block_free(const uint32_t add)
{
    uint32_t index = ADDRESS_TO_BLOCK(add);
    if (!pmm_bitmap)
        return;
    uint32_t word = index / (sizeof(uint32_t) * 8); // Calculate which word (integer) in the bitmap
	uint32_t bit = index % (sizeof(uint32_t) * 8); // Calculate which bit within the word

	pmm_bitmap[word] &= ~(1 << bit); // Clear the bit by ANDing with the complement of a mask
}

void init_region_used(const uint32_t start_add, const uint32_t end_add)
{
    if (end_add <= start_add)
        return;
    uint32_t startBlock = start_add % BLOCK_SIZE == 0 ? start_add : ALIGN_4KB_DOWN(start_add);
    uint32_t endBlock = end_add % BLOCK_SIZE == 0 ? end_add : (ALIGN_4KB_UP(end_add));
    for (uint32_t i = startBlock;i < endBlock;i+=BLOCK_SIZE)
    {
        init_block_used(i);
        usedBlocks++;
    }
}

void init_region_free(const uint32_t start_add, const uint32_t end_add) 
{
    if (end_add <= start_add)
        return;
    uint32_t startBlock = start_add % BLOCK_SIZE == 0 ? start_add : ALIGN_4KB_DOWN(start_add);
    uint32_t endBlock = end_add % BLOCK_SIZE == 0 ? end_add : (ALIGN_4KB_UP(end_add));
    for (uint32_t i = startBlock;i < endBlock;i+=BLOCK_SIZE)
    {
        init_block_free(i);
        usedBlocks--;
    }
    init_block_used(0x00000000); //set the first block of memory as used to not cause problems later.
}

void init_physical_memory(const uint32_t size)
{
    uint32_t kernel_end_address = &kernel_end;
    if (kernel_end_address % BLOCK_SIZE != 0)
        kernel_end_address = ALIGN_4KB_UP(kernel_end_address);
    pmm_bitmap = (uint32_t*) kernel_end_address;
    maxBlocks = size / BLOCK_SIZE;
    usedBlocks = maxBlocks;
    memset(pmm_bitmap, 0xFF, maxBlocks / BLOCKS_PER_BYTE);        // set all blocks used.
    bitmap_end = (uint32_t)(pmm_bitmap + (maxBlocks/32));
    if ((uint32_t)bitmap_end % BLOCK_SIZE != 0)
        bitmap_end = ALIGN_4KB_UP((uint32_t)bitmap_end);
    kprintf("Initializing physical memory of size %x bytes\n", size);
}

uint32_t allocate_block()
{
    return allocate_blocks(1);
}

void free_block(const uint32_t address)
{
    free_blocks(address, 1);
}

uint32_t allocate_blocks(const uint32_t num_blocks)
{
    if (num_blocks == 0 || ((maxBlocks - usedBlocks) < num_blocks)) return -1; // Can't return no memory, error
    uint32_t block = 0;

    // Test 32 blocks at a time
    for (uint32_t i = 0; i < maxBlocks / 32 && !block;  i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            // At least 1 bit is not set within this 32bit chunk of memory,
            //   find that bit by testing each bit
            for (uint32_t j = 0; j < 32 && !block; j++) {
                uint32_t bit = 1 << j;

                // If bit is unset/0, found start of a free region of memory
                if (!(pmm_bitmap[i] & bit)) {
                    
                    // Checking every bit after the one found free, looking for the amount of blocks needed
                    for (uint32_t count = 0, free_blocks = 0, index = 0; count < num_blocks && !block; count++) {
                        index = (count + j) / 32;
                        if (!(pmm_bitmap[i+index] & (1 << ((j + count) - (32 * index)))))
                            free_blocks++;
                        

                        if (free_blocks == num_blocks) // Found enough free space
                            block = i*32 + j;
                    }
                }
            }
        }
    }

    if(block) 
    {
        uint32_t tempBlock = BLOCK_TO_ADDRESS(block);
        for (uint32_t n = 0; n < num_blocks; n++ ,tempBlock+=BLOCK_SIZE)
            init_block_used(tempBlock); // setting the blocks allocated to used in bitmap
        usedBlocks += num_blocks;
        return BLOCK_TO_ADDRESS(block);
    }
    else
        return -1;  // No free region of memory large enough
}

void free_blocks(const uint32_t address, const uint32_t num_blocks)
{

    for (uint32_t i = 0; i < num_blocks; i++) 
        init_block_free((uint32_t)(address + (i * BLOCK_SIZE)));    // Unset bits/blocks in memory map, to free
    usedBlocks -= num_blocks;  // Decrease used block count
}