#include "tss.h"

tss_entry_t TSS;
extern void flush_tss(void);

extern uint32_t PAGE_DIR_VIRTUAL;
extern uint32_t PAGE_DIR_PHYSICAL;

void set_tss_kernel_stack(uint32_t kernelSS, uint32_t kernelESP)
{
    TSS.ss0 = kernelSS;
    TSS.esp0 = kernelESP;
}

void install_tss(uint32_t idx, uint32_t kernelSS, uint32_t kernelESP)
{
    uint32_t base = (uint32_t) &TSS;
    memset ((void*) base, 0, sizeof (tss_entry_t));

    TSS.ss0 = kernelSS;
    TSS.esp0 = kernelESP;

    gdt_set_gate(idx, base, base + sizeof(tss_entry_t), 0xE9, 0);
    
    TSS.cs = 0x0b;
	TSS.ss = 0x13;
	TSS.es = 0x13;
	TSS.ds = 0x13;
	TSS.fs = 0x13;
	TSS.gs = 0x13;

    TSS.cr3 = (&PAGE_DIR_PHYSICAL);

    flush_tss();
}