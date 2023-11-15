// gdt.c
#include "gdt.h"

// Global GDT array
struct gdt_entry gdt_entries[3];
struct gdt_ptr gp;


// Set the value of one GDT entry.
static void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;
    
    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}


// Function to initialize GDT
void init_gdt(void) {
    // Null descriptor
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base  = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    
    gdt_flush((uint32_t)&gp);
}
