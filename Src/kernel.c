#include "gdt.h"
#include "idt.h"
#include "Screen.h"
#include "callback.h"

void kernel_main(void) 
{
    //initialize the descriptor tables
    init_gdt();
    init_idt();
    //clear the monitor from things that were written by GRUB.
    clearScreen();
    printString("Hello World!");
    asm volatile("sti");
    init_keyboard();
}