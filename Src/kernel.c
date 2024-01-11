#include "gdt.h"
#include "idt.h"
#include "Screen.h"
#include "keyboard.h"
#include "heap.h"
#include "utils.h"

void kernel_main(void) 
{
    const char* str = "Hello World";
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
    if (!ptr1 || !ptr2)
    {    
        kprintf("Error allocating memory");
        return;
    }
    kprintf("ptr1 is %x and ptr2 is %x\n", ptr1, ptr2);
    kfree(ptr1);
    kfree(ptr2);
    ptr1 = kmalloc(4);
    if (!ptr1)
    {    
        kprintf("Error allocating memory");
        return;
    }
    int* ptr = ptr1;
    *ptr = 10;
    kprintf("ptr1 is %x and its value is %d\n", ptr, *ptr);
    ptr2 = kmalloc(strlen(str));
    if (!ptr2)
    {    
        kprintf("Error allocating memory");
        return;
    }
    memcpy((void*)ptr2, str, strlen(str)+1);
    kprintf("The string in ptr2 is: %s\n", (char*)ptr2);
    asm volatile("sti");
}