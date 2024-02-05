#include "vmm.h"

current_page_dir = 0;

void init_paging(page_directory_t* dir_physical_address)
{
    //set cr3 register to hold dir_physical address;
    set_page_directory(dir_physical_address);

    //register interrupt handler for page fault
    register_handler(PAGE_FAULT, page_fault);

    //read current cr0 register, set paging bit and put new cr0 register.
    asm volatile("mov %cr0, %eax; or $80000001, %eax; mov %eax, %cr0;");
}


static void page_fault(registers_t* regs)
{
    uint32_t address;
    asm volatile("mov %%cr2, %0" :"r=" (address)); //cr2 register stores the address that caused the page fault.
    uint32_t present = regs->err_code & PTE_PRESENT;
    uint32_t read_write = regs->err_code & PTE_WRITEABLE;
    uint32_t user = regs->err_code & PTE_USER;
    uint32_t reserved = regs->err_code & PTE_WRITETHOUGH
    if (present)
        kprintf("page fault present at address %x", address);
    else if (read_write)
        kprintf("page fault read-only at address %x", address);
    else if (user)
        kprintf("page fault user-mode at address %x", address);
    else if (reserved)
        kprintf("page fault reserved page at address %x", address);
}


page_table_entry_t* get_page(const void* virtual_address)
{
    page_directory_t* dir = current_page_dir;
    page_dir_entry_t* pd_entry = &(dir->pages[PD_INDEX(virtual_address)]); //get pd_entry which is the page table
    page_table_t* pt = (page_table_t*)(PDE_GET_FRAME((uint32_t)*pd_entry)); //get the physical page table from pd_entry
    return &(pt->pages[PT_INDEX(virtual_address)]);
}


page_dir_entry_t* get_page_table(const void* virtual_address)
{
    page_directory_t* dir = current_page_dir;
    return &(dir->pages[PD_INDEX(virtual_address)]);
}


uint32_t map_page(void* virtual_address ,void* physical_address, const uint32_t flags)
{
    page_directory_t* dir = current_page_dir;
    page_dir_entry_t* pd_entry = &(dir->pages[PD_INDEX(virtual_address)]);
    page_table_t* pt = PDE_GET_FRAME((uint32_t)*pd_entry);
    if (*pd_entry & PDE_PRESENT != PDE_PRESENT)
    {
        void* block = allocate_block();
        if (!block) return 0;
        PDE_SET_FRAME(pd_entry, block);
        SET_ATTRIBUTE(pd_entry, PDE_PRESENT);
        SET_ATTRIBUTE(pd_entry, PDE_READ_WRITE);
    }
    page_table_entry_t* pt_entry = &(pt->pages[PT_INDEX(virtual_address)]);
    PTE_SET_FRAME(pt_entry, (uint32_t)(physical_address & PTE_FRAME));
    SET_ATTRIBUTE(pt_entry, (flags & (~PTE_FRAME)));
    SET_ATTRIBUTE(pt_entry, PTE_PRESENT);
    flush_tlb_address(virtual_address); //flush the tlb entry of the virtual address mapped.
    return 1;
}

void unmap_page(void* virtual_address)
{
    page_table_entry_t* page = get_page((uint32_t)virtual_address);
    if (PTE_PRESENT(page))
    {
        PTE_GET_FRAME(page);
        free_block(block);
        PTE_SET_FRAME(page, 0);
        CLEAR_ATTRIBUTE(page, PTE_PRESENT);
    }
    flush_tlb_address(virtual_address);
}