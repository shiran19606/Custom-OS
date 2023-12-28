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
    asm volatile("sti");
    clearScreen();
    kprintf("Hello World!\n");
    init_keyboard(); //initialize the keyboard driver
    initialize_allocator(); //start heap allocation.
    uint32_t ptr = kmalloc(10);
    uint32_t ptr2 = kmalloc(100);
    kprintf("ptr is %x and ptr2 is %x", ptr, ptr2);
    kfree((void*)ptr);
    kfree((void*)ptr2);
    uint32_t ptr3 = kmalloc(1000);
    kprintf("\nptr3 is %x", ptr3);
    kfree((void*)ptr3);
}