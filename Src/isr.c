#include "isr.h"

isr_t handlers[256];

void isr_handler(struct registers* regs) //according to interrupts.s calling the isr_handler function pushes all the registers, so we created a struct that will be able to use all these values, but it is important to make sure not to change the values of some of them.
{
    printString("Unhandled interrupt: ");
    printNumber(regs->int_no);
    put_char('\n');
}

void register_handler(uint8_t num, isr_t handler)
{
    handlers[num] = handler;
}

void irq_handler(struct registers* regs)
{
    if (handlers[regs->int_no] != 0)
    {
        isr_t handler = handlers[regs->int_no];
        handler(regs);
    }
    if (regs->int_no >= 40)
        port_byte_out(0xA0, 0x20);
    port_byte_out(0x20, 0x20);
}
