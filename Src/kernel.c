#include "gdt.h"
#include "idt.h"

void kernel_main(void) 
{
    init_gdt();
    init_idt();
    unsigned char* ptr = (unsigned char*)0xB8000;
    *ptr = 'S';
    asm volatile ("int $0x3");
    while(1);
}