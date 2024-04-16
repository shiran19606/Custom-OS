#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>

#include "vmm.h"

// Define the block header structure
typedef struct block_header {
    uint32_t size;
    struct block_header* next;
} block_header;

#define KHEAP_START 0xE0000000 //put the heap in the last 512mb of memory.
#define MEMORY_SIZE 0x100000

void initialize_allocator();
void* kmalloc(uint32_t size);
void kfree(void* ptr);

#endif