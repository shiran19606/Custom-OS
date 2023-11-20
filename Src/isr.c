#include "isr.h"

void isr_handler(struct registers regs) //according to interrupts.s calling the isr_handler function pushes all the registers, so we created a struct that will be able to use all these values, but it is important to make sure not to change the values of some of them.
{
    //we can print the interrupt number for example, but we havent implemented a printing mechanism yet.
    //instead we can debug the value of regs.
}