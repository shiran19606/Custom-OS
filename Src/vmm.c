#include "vmm.h"

extern void load_page_directory(uint32_t base_reg);
extern void flush_tlb(void);
extern void enable_paging(void);

page_directory_t* current_page_dir = 0; //physical address of the current page directory

/*
this function handles a page fault interrupt. it should read the cr2 register to found out information about the faulting address, and the error code pushed by
the CPU can tell us what caused page fault (access rights, page not present etc.)
regs: a struct representing the cpu state when the interrupt happend.
return value: None.
*/
static void page_fault(registers_t* regs)
{
    uint32_t address;
    asm volatile("movl %%cr2, %0" :"r=" (address)); //cr2 register stores the address that caused the page fault.
    uint8_t present = !(regs->err_code & PTE_PRESENT);
    uint8_t read_write = regs->err_code & PTE_WRITEABLE;
    uint8_t user = regs->err_code & PTE_USER;
    uint8_t reserved = regs->err_code & PTE_WRITETHOUGH;
    if (present)
    {
        kprintf("page fault present at address %x\n", address);
        map_page(address, allocate_block(), PTE_PRESENT | PTE_WRITEABLE, get_page_dir());
    }
    if (read_write)
        kprintf("page fault read-only at address %x\n", address);
    if (user)
        kprintf("page fault user-mode at address %x\n", address);
    if (reserved)
        kprintf("page fault reserved page at address %x\n", address);
}

void init_paging(page_directory_t* dir_physical_address)
{
    //set cr3 register to hold dir_physical address;
    set_page_directory(dir_physical_address);

    //register interrupt handler for page fault
    register_handler(PAGE_FAULT, page_fault);

    enable_paging();
}

page_table_entry_t* get_page(const void* virtual_address)
{
    page_directory_t* dir = current_page_dir;
    page_dir_entry_t* pd_entry = &(dir->pages[PD_INDEX(virtual_address)]); //get pd_entry which is the page table
    if (PDE_IS_PRESENT(*pd_entry))
    {
        page_table_t* pt = (page_table_t*)(PDE_GET_FRAME((*pd_entry))); //get the physical page table from pd_entry
        return &(pt->pages[PT_INDEX(virtual_address)]);
    }
    return 0;
}


page_dir_entry_t* get_page_table(const void* virtual_address)
{
    page_directory_t* dir = current_page_dir;
    return &(dir->pages[PD_INDEX(virtual_address)]);
}


uint32_t map_page(void* virtual_address ,void* physical_address, const uint32_t flags, page_directory_t* dir)
{
    page_dir_entry_t* pd_entry = &(dir->pages[PD_INDEX(virtual_address)]);
    uint32_t flags1 = *pd_entry & ~(PDE_FRAME);
    if ((flags1 & PDE_PRESENT) != PDE_PRESENT)
    {
        void* block = allocate_block();
        if (!block) return 0;
        PDE_SET_FRAME(pd_entry, (uint32_t)block);
        SET_ATTRIBUTE(pd_entry, PDE_PRESENT);
        SET_ATTRIBUTE(pd_entry, PDE_WRITEABLE);
    }
    page_table_t* pt = (page_table_t*)(PDE_GET_FRAME(*pd_entry));
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
    if (PTE_IS_PRESENT(page))
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
        if ((PTE_IS_PRESENT((page_table_physical->pages[i])))) //if we found another page present, it means we cant delete the entire page table, wo we can stop the function.
            return;

    //if we reached here, it means that the page table is empty.
    free_block((uint32_t)page_table_physical); //free the frame used by the page table.
    CLEAR_ATTRIBUTE(pd_entry, PDE_PRESENT);
}


void* allocate_page(page_table_entry_t* page, uint32_t flags)
{
    void* block = allocate_block();
    if(block != -1)
    {
        PTE_SET_FRAME(page, (uint32_t*)block);
        SET_ATTRIBUTE(page, flags);
    }
    return block;
}


void free_page(page_table_entry_t* page)
{
    if(page)
        free_block(PTE_GET_FRAME(page));
    CLEAR_ATTRIBUTE(page, PTE_PRESENT);
}


uint32_t set_page_directory(page_directory_t* dir)
{
    if(!dir) return 0;

    current_page_dir = dir;
    load_page_directory(current_page_dir); 
    return 1;
}


void flush_tlb_address(const void* virtual_address)
{
    asm volatile ("invlpg (%0)" : : "r"(virtual_address) : "memory");
}


void flush_all_tlb()
{
    flush_tlb();
}


void* virtual_to_physical(const void* virtual_address)
{
    page_table_entry_t* page = get_page((uint32_t)virtual_address);
    if ((uint32_t)page == 0) return 0;
    uint32_t frame = PTE_GET_FRAME(page);
    return (void*)(frame | PAGE_INDEX(virtual_address));
}


uint32_t initialize_vmm()
{
    page_directory_t* pd = (page_directory_t*) allocate_block();
    if (!pd) return 0;
    page_table_t* pt = (page_table_t*) allocate_block();
    if (!pt) return 0;
    
    for(uint32_t i = 0; i < ENTRIES_IN_PAGE_DIR; i++)
        pd->pages[i] = 0x2;

    page_dir_entry_t* first_page_table = &(pd->pages[0]);
    SET_ATTRIBUTE(first_page_table, PDE_PRESENT);
    SET_ATTRIBUTE(first_page_table, PDE_WRITEABLE);
    PDE_SET_FRAME(first_page_table, (uint32_t)pt);

    uint32_t phys_addr = 0x0;
    // identity mapping the first 4mb 
    for(uint32_t i = 0; i < ENTRIES_IN_PAGE_TABLE; ++i) {
        map_page(phys_addr, phys_addr, PTE_PRESENT | PTE_WRITEABLE, pd);
        phys_addr += PAGE_SIZE;
    }
    init_paging(pd);
    return 1;
}

page_directory_t* get_page_dir()
{
    return current_page_dir;
}