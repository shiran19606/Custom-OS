#include <stdint.h>

typedef struct gdt_entry {
    uint16_t limit_low;         // lower 16 bits of the limit
    uint16_t base_low;          // lower 16 bits of the base address
    uint8_t base_middle;        // next 8 bits of the base address
    uint8_t access;             // access flags
    uint8_t granularity;        // granularity and flags
    uint8_t base_high;          // upper 8 bits of the bsae address
} gdt_entry;

typedef struct gdt_ptr {
    uint16_t limit;             // 16bit limit of the GDT
    uint32_t base;              // 32bit base address of the GDT
} gdt_ptr;

// GDT entries
struct gdt_entry my_gdt[3];

// GDT pointer
struct gdt_ptr gp;

// function to load the GDT 
extren void gdt_flush(uint32_t); // need to write in assembly

void init_gdt() 
{
    // null segment
    my_gdt[0].limit_low = 0;
    my_gdt[0].base_low = 0;
    my_gdt[0].base_middle = 0;
    my_gdt[0].access = 0;
    my_gdt[0].granularity = 0;
    my_gdt[0].base_high = 0;

    // code segment
    my_gdt[1].limit_low = 0xFFFF;
    my_gdt[1].base_low = 0;
    my_gdt[1].base_middle = 0;
    my_gdt[1].access = 0x9A;
    my_gdt[1].granularity = 0b11001111;
    my_gdt[1].base_high = 0;

    // data segment
    my_gdt[2].limit_low = 0xFFFF;
    my_gdt[2].base_low = 0;
    my_gdt[2].base_middle = 0;
    my_gdt[2].access = 0x92;
    my_gdt[2].granularity = 0b11001111;
    my_gdt[2].base_high = 0;

    gp.limit = sizeof(my_gdt) - 1;
    gp.base = (struct gdt*)&my_gdt;

    gdt_flush((struct gdt*)&gp);
}


