#ifndef PAGE_FRAME_ALLOCATOR
#define PAGE_FRAME_ALLOCATOR

#include "utils.h"
#include "vmm.h"

void init_pfa();

void* page_allocate();
void* pages_allocate(uint32_t num_pages);

void page_free(void* virt_address);
void pages_free(void* virt_address, uint32_t num_pages);
#endif