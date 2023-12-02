#include "gdt.h"
#include "idt.h"
#include "Screen.h"

extern char end;

void kernel_main(void) 
{
    //initialize the descriptor tables
    init_gdt();
    init_idt();
    //clear the monitor from things that were written by GRUB.
    clearScreen();

    printString("Hello World!");
    while(1);
}