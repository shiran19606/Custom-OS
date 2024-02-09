#include <stdint.h>
#include <stddef.h>

#include "Screen.h"

// Define the block header structure
typedef struct block_header {
    uint32_t size;
    struct block_header* next;
} block_header;

#define MEMORY_SIZE 0x100000

void initialize_allocator();
void* kmalloc(uint32_t size);
void kfree(void* ptr);