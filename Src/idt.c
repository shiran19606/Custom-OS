#include "idt.h"

struct idt_entry idt_entries[256];
struct idt_ptr ptr;

void (*isrFunctions[32])(void) = {
    isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7,
    isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15,
    isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
    isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
};

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
   idt_entries[num].base_low = base & 0xFFFF;
   idt_entries[num].base_high = (base >> 16) & 0xFFFF;

   idt_entries[num].segment_selector     = sel;
   idt_entries[num].unused = 0;
   // We must uncomment the OR below when we get to using user-mode.
   // It sets the interrupt gate's privilege level to 3.
   idt_entries[num].flags   = flags /* | 0x60 */;
} 

void init_idt(void)
{
    ptr.base = (uint32_t)&idt_entries;
    ptr.limit = sizeof(struct idt_entry) * 256 - 1;

    for (int i = 0;i < 32;i++)
        idt_set_gate(i, (uint32_t)(isrFunctions[i]), 0x08, 0x8E);
    idt_flush((uint32_t)&ptr);
}