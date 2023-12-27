#include "gdt.h"
#include "idt.h"
#include "Screen.h"
#include "keyboard.h"
#include "heap.h"

void kernel_main(void) 
{
    //initialize the descriptor tables
    init_gdt();
    init_idt();
    //clear the monitor from things that were written by GRUB.
    clearScreen();
    asm volatile("sti");
    kprintf("Hello World!\n");
    init_keyboard(); //initialize the keyboard driver
    initialize_allocator(); //start heap allocation.
    uint32_t ptr = kmalloc(4);
    uint32_t ptr2 = kmalloc(10);
    kprintf("ptr is %x and ptr2 is %x\n", ptr, ptr2);
    kfree((void*)ptr);
    kfree((void*)ptr2);
    ptr = kmalloc(4);
    ptr2 = kmalloc(0x1000);
    int* ptr3 = ptr;
    *ptr3 = 3;
}