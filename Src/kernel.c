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
    init_keyboard();
    initialize_allocator();
    kprintf("Hello World\n");
    uint32_t ptr1 = kmalloc(10);
    uint32_t ptr2 = kmalloc(100);
    kprintf("ptr1 is %x and ptr2 is %x\n", ptr1, ptr2);
    kfree((void*)ptr1);
    kfree((void*)ptr2);
    ptr1 = kmalloc(4);
    int* ptr = ptr1;
    *ptr = 10;
    kprintf("ptr1 is %x and its value is %d", ptr, *ptr);
    asm volatile("sti");
}