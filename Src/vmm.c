#include "vmm.h"

page_directory_t* current_page_dir = 0; //physical address of the current page directory

void init_paging(page_directory_t* dir_physical_address)
{
    //set cr3 register to hold dir_physical address;
    set_page_directory(dir_physical_address);

    //register interrupt handler for page fault
    register_handler(PAGE_FAULT, page_fault);

    //read current cr0 register, set paging bit and put new cr0 register.
    // Use inline assembly to perform bitwise OR with CR0 register
    uint32_t value = 0x80000001;
    asm volatile (
        "mov %%cr0, %%eax;"
        "or %0, %%eax;"
        "mov %%eax, %%cr0;"
        : // no outputs
        : "r" (value)
        : "eax"
    );
}


static void page_fault(registers_t* regs)
{
    uint32_t address;
    asm volatile("mov %%cr2, %0" :"r=" (address)); //cr2 register stores the address that caused the page fault.
    uint32_t present = regs->err_code & PTE_PRESENT;
    uint32_t read_write = regs->err_code & PTE_WRITEABLE;
    uint32_t user = regs->err_code & PTE_USER;
    uint32_t reserved = regs->err_code & PTE_WRITETHOUGH;
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
    page_table_t* pt = (page_table_t*)(PDE_GET_FRAME((*pd_entry))); //get the physical page table from pd_entry
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
    page_table_t* pt = (page_table_t*)(PDE_GET_FRAME(*pd_entry));
    if ((uint32_t)(*pd_entry) & PDE_PRESENT != PDE_PRESENT)
    {
        void* block = allocate_block();
        if (!block) return 0;
        PDE_SET_FRAME(pd_entry, block);
        SET_ATTRIBUTE(pd_entry, PDE_PRESENT);
        SET_ATTRIBUTE(pd_entry, PDE_WRITEABLE);
    }
    page_table_entry_t* pt_entry = &(pt->pages[PT_INDEX(virtual_address)]);
    PTE_SET_FRAME(pt_entry, (uint32_t)(physical_address));
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
        void* block = (void*)PTE_GET_FRAME(page);
        free_block((uint32_t)block);
        PTE_SET_FRAME(page, 0);
        CLEAR_ATTRIBUTE(page, PTE_PRESENT);
    }
    flush_tlb_address(virtual_address);

    page_dir_entry_t* pd_entry = get_page_table((void*)virtual_address);
    page_table_t* page_table_physical = (page_table_t*)(PDE_GET_FRAME(*pd_entry));

    //if the unmaped page is the only page in the page table, we can free the page table aswell.
    for (int i = 0;i<ENTRIES_IN_PAGE_TABLE;i++)
        if ((PTE_PRESENT((page_table_physical->pages[i])))) //if we found another page present, it means we cant delete the entire page table, wo we can stop the function.
            return;

    //if we reached here, it means that the page table is empty.
    free_block((uint32_t)page_table_physical); //free the frame used by the page table.
    CLEAR_ATTRIBUTE(pd_entry, PDE_PRESENT);
}