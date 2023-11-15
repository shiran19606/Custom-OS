#include "gdt.h"

void kernel_main(void) 
{
    init_gdt();
    unsigned char* ptr = (unsigned char*)0xB8000;
    *ptr = 'S';
    while(1);
}