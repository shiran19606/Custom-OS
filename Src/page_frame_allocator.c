#include "page_frame_allocator.h"

uint32_t last_free_page = 0;
uint32_t max_pages = 0;
uint32_t used_pages = 0;

void init_pfa()
{
    last_free_page = 0x70000000;
    max_pages = (0x80000000 - last_free_page) / 0x1000;
    used_pages = 0;
}

void* page_allocate()
{
    return pages_allocate(1);
}

void page_free(void* virt_address)
{
    pages_free(virt_address, 1);
}

void* pages_allocate(uint32_t num_pages)
{
    if (num_pages > (max_pages - used_pages))
        return 0;
    uint32_t start_address = last_free_page;
    uint32_t address_found = start_address;
    for (uint32_t i = 0; i<num_pages && address_found < 0x80000000; i++)
    {
        page_table_entry_t* pt_entry = get_page(start_address);
        if (pt_entry && PTE_IS_PRESENT(pt_entry)) //means the page is taken.
        {
            i = 0;
            address_found = start_address + 0x1000;
        }
        start_address += 0x1000;
    }
    if (address_found == 0x80000000)
        return 0;
    start_address = address_found;
    for (uint32_t num = num_pages; num>0;num--)
    {
        page_table_entry_t* pt_entry = get_page(start_address);
        if (!pt_entry || !PTE_IS_PRESENT(pt_entry))
        {
            void* physical_block = allocate_block();
            uint32_t result = map_page(start_address, physical_block, (PTE_PRESENT, PTE_WRITEABLE, PTE_USER));
            if (!result)
                return 0;
        }
        pt_entry = get_page(start_address);
        SET_ATTRIBUTE(pt_entry, PTE_PRESENT);
        start_address += 0x1000;
    }
    if (address_found == last_free_page)
        last_free_page += (0x1000 * num_pages);
    return (void*)address_found;
}

void pages_free(void* virt_address, uint32_t num_pages)
{
    void* address = virt_address;
    for (int i = 0;i<num_pages;i++)
    {
        page_table_entry_t* pt_entry = get_page(address);
        CLEAR_ATTRIBUTE(pt_entry, PTE_PRESENT);
        if (last_free_page > address)
            last_free_page = address;
        address += 0x1000;
    }
}