#include "idt.h"

extern void idt_flush(uint32_t ptr);
static void set_idt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

struct idt_entry idt_entries[256];
struct idt_ptr ptr;


static void set_idt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
   idt_entries[num].base_low = base & 0xFFFF;
   idt_entries[num].base_high = (base >> 16) & 0xFFFF;

   idt_entries[num].segment_selector     = sel;
   idt_entries[num].unused = 0;
   idt_entries[num].flags   = flags;
} 

void init_idt()
{
    ptr.limit = sizeof(struct idt_entry) * 256 - 1;
    ptr.base = (uint32_t)&idt_entries;
    for (int i = 0;i<256;i++)
        set_idt_gate(i, 0, 0, 0);//0 all idt.

    port_byte_out(0x20, 0x11);
    port_byte_out(0xA0, 0x11);
    port_byte_out(0x21, 0x20);
    port_byte_out(0xA1, 0x28);
    port_byte_out(0x21, 0x04);
    port_byte_out(0xA1, 0x02);
    port_byte_out(0x21, 0x01);
    port_byte_out(0xA1, 0x01);
    port_byte_out(0x21, 0);
    port_byte_out(0xA1, 0);

    set_idt_gate( 0, (uint32_t)isr0 , 0x08, 0x8E);
    set_idt_gate( 1, (uint32_t)isr1 , 0x08, 0x8E);
    set_idt_gate( 2, (uint32_t)isr2 , 0x08, 0x8E);
    set_idt_gate( 3, (uint32_t)isr3 , 0x08, 0x8E);
    set_idt_gate( 4, (uint32_t)isr4 , 0x08, 0x8E);
    set_idt_gate( 5, (uint32_t)isr5 , 0x08, 0x8E);
    set_idt_gate( 6, (uint32_t)isr6 , 0x08, 0x8E);
    set_idt_gate( 7, (uint32_t)isr7 , 0x08, 0x8E);
    set_idt_gate( 8, (uint32_t)isr8 , 0x08, 0x8E);
    set_idt_gate( 9, (uint32_t)isr9 , 0x08, 0x8E);
    set_idt_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    set_idt_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    set_idt_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    set_idt_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    set_idt_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    set_idt_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    set_idt_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    set_idt_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    set_idt_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    set_idt_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    set_idt_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    set_idt_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    set_idt_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    set_idt_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    set_idt_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    set_idt_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    set_idt_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    set_idt_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    set_idt_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    set_idt_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    set_idt_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    set_idt_gate(31, (uint32_t)isr31, 0x08, 0x8E);
    set_idt_gate(32, (uint32_t)Timer_Handler , 0x08, 0x8E);
    set_idt_gate(33, (uint32_t)irq1 , 0x08, 0x8E);
    set_idt_gate(34, (uint32_t)irq2 , 0x08, 0x8E);
    set_idt_gate(35, (uint32_t)irq3 , 0x08, 0x8E);
    set_idt_gate(36, (uint32_t)irq4 , 0x08, 0x8E);
    set_idt_gate(37, (uint32_t)irq5 , 0x08, 0x8E);
    set_idt_gate(38, (uint32_t)irq6 , 0x08, 0x8E);
    set_idt_gate(39, (uint32_t)irq7 , 0x08, 0x8E);
    set_idt_gate(40, (uint32_t)irq8 , 0x08, 0x8E);
    set_idt_gate(41, (uint32_t)irq9 , 0x08, 0x8E);
    set_idt_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    set_idt_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    set_idt_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    set_idt_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    set_idt_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    set_idt_gate(47, (uint32_t)irq15, 0x08, 0x8E);

    idt_flush((uint32_t)&ptr);
}