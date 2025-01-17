#include "heap.h"


block_header* free_list = NULL;

void initialize_allocator() {
    // Initialize the free list with the entire memory space
    uint32_t num_pages = MEMORY_SIZE / PAGE_SIZE;
    for (uint32_t i = 0, start_addr = KHEAP_START;i<num_pages;i++, start_addr += PAGE_SIZE)
    {
        void* block = (void*)allocate_block();
        if ((uint32_t)block == 0xFFFFFFFF)
        {
            kprintf("Not enough memory for heap");
            asm volatile("cli;hlt");
        }
        if (map_page((void*)start_addr, block, (PTE_PRESENT | PTE_WRITEABLE)))
            memset((void*)start_addr, 0, PAGE_SIZE); //clear page
        else
            asm volatile("cli;hlt");

    }
    block_header* initial_block = (block_header*)KHEAP_START;
    initial_block->size = (MEMORY_SIZE - sizeof(block_header));
    initial_block->next = NULL;
    free_list = initial_block;
}

//this function takes the head of a linked list and two nodes. it will swap the locations of the nodes
//for example, a list a->b->c->d->null called with parameters b, c will look like this after the run:
//a->c->b->d
void swapNodes(block_header** head_ref, block_header* nodeX, block_header* nodeY) {
    if (!nodeX || !nodeY || nodeX == nodeY || *head_ref == NULL || (*head_ref)->next == NULL) {
        return;
    }

    block_header** a = NULL, ** b = NULL;
    while (*head_ref) {
        if (*head_ref == nodeX)
            a = head_ref;
        else if (*head_ref == nodeY)
            b = head_ref;

        head_ref = &((*head_ref)->next);
    }

    if (a && b) {
        block_header* temp = *a;
        *a = *b;
        *b = temp;

        temp = (*a)->next;
        (*a)->next = (*b)->next;
        (*b)->next = temp;
    }
}

// Custom malloc function, that takes the size of the wanted memory and returns a pointer to the block allocated.
void* kmalloc(uint32_t size) {

    block_header* block = free_list;
    block_header* prev_block = NULL;
    // Find a suitable block in the free list
    while (block != NULL) {
        if (block->size >= size) {
            // Block found
            if (block->size > size + sizeof(block_header)) {
                // Split the block if it's larger than needed
                block_header* new_block = (block_header*)((char*)block + sizeof(block_header) + size);
                new_block->size = block->size - size - sizeof(block_header);
                new_block->next = block->next;
                block->next = new_block;
                block->size = size;
            }

            // Remove the allocated block from the free list
            if (prev_block == NULL) {
                free_list = block->next;
            }
            else {
                prev_block->next = block->next;
            }
            return ((char*)block + sizeof(block_header));
        }

        prev_block = block;
        block = block->next;
    }
    if (block) //if a block found
    {
        block->size = size;
        block->next = NULL;
        return ((char*)block + sizeof(block_header));
    }
    else
    {
        return 0;
    }
}


// Custom free function
void kfree(void* ptr) {
    if (!ptr) {
        return; // Null pointer
    }

    // Get the block header by moving back the pointer
    block_header* block_to_free = (block_header*)(((uint32_t)ptr) - sizeof(block_header));
    // Insert the block back into the free list
    block_to_free->next = free_list;
    free_list = block_to_free;

    // Coalesce adjacent free blocks
    block_header* current = free_list;
    while (current != NULL && current->next != NULL) {
        // Check if the next block is logically adjacent
        if (((char*)current) - current->next->size - sizeof(block_header) == (char*)current->next) {
            // Merge adjacent blocks
            block_header* temp = current->next;
            swapNodes(&free_list, current, current->next);
            current = temp;
        }
        if (((char*)current) + current->size + sizeof(block_header) == (char*)current->next) {
            // Merge adjacent blocks
            current->size += sizeof(block_header) + current->next->size;
            current->next = current->next->next;
            current = free_list;
        }
        else {
            current = current->next;
        }
    }

}