#include "isr.h"

void isr_handler(struct registers regs) //according to interrupts.s calling the isr_handler function pushes all the registers, so we created a struct that will be able to use all these values, but it is important to make sure not to change the values of some of them.
{
    printString("Unhandled interupt: ");
    printNumber(regs.int_no);
}