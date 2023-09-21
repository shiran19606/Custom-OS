#include "types.h"

void kernel_main(void) 
{
    unsigned char* ptr = (unsigned char*)0xB8000;
    *ptr = 'S';
}