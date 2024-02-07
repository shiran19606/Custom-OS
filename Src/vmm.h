#include "pmm.h"
#include "isr.h"

#define ENTRIES_IN_PAGE_TABLE 1024
#define ENTRIES_IN_PAGE_DIR 1024

#define PAGE_OFFSET_BITS 12
#define PAGE_TABLE_ENTRY_BITS 10

//Paging releated:
//these structs and enums releate to paging structures and macros. they work with pages, page tables and page directories

#define PAGE_FAULT 14
#define PAGE_SIZE 4096

enum PAGE_PTE_FLAGS {

    PTE_PRESENT = 1,		    //00000000000000000000000000000001
    PTE_WRITEABLE = 2,		    //00000000000000000000000000000010
    PTE_USER = 4,		        //00000000000000000000000000000100
    PTE_WRITETHOUGH = 8,		//00000000000000000000000000001000
    PTE_NOT_CACHEABLE = 0x10,	//00000000000000000000000000010000
    PTE_ACCESSED = 0x20,		//00000000000000000000000000100000
    PTE_DIRTY = 0x40,		    //00000000000000000000000001000000
    PTE_PAT = 0x80,		        //00000000000000000000000010000000
    PTE_CPU_GLOBAL = 0x100,		//00000000000000000000000100000000
    PTE_LV4_GLOBAL = 0x200,		//00000000000000000000001000000000
    PTE_FRAME = 0xFFFFF000 	    //11111111111111111111000000000000
};

enum PAGE_PDE_FLAGS {

    PDE_PRESENT = 1,	        //00000000000000000000000000000001
    PDE_WRITEABLE = 2,	        //00000000000000000000000000000010
    PDE_USER = 4,		        //00000000000000000000000000000100
    PDE_PWT = 8,		        //00000000000000000000000000001000
    PDE_PCD = 0x10,		        //00000000000000000000000000010000
    PDE_ACCESSED = 0x20,        //00000000000000000000000000100000
    PDE_DIRTY = 0x40,		    //00000000000000000000000001000000
    PDE_4MB = 0x80,		        //00000000000000000000000010000000
    PDE_CPU_GLOBAL = 0x100,	    //00000000000000000000000100000000
    PDE_LV4_GLOBAL = 0x200,	    //00000000000000000000001000000000
    PDE_FRAME = 0xFFFFF000 	    //11111111111111111111000000000000
};

typedef uint32_t page_dir_entry_t;
typedef uint32_t page_table_entry_t;

typedef struct page_table
{
    page_table_entry_t pages[ENTRIES_IN_PAGE_TABLE];
} page_table_t;

typedef struct page_directory
{
    page_dir_entry_t pages[ENTRIES_IN_PAGE_DIR];
} page_directory_t;

//macros refering to Pages (Page table entry is a page)
#define PTE_IS_PRESENT(addr) ((uint32_t)addr & PTE_PRESENT)
#define PTE_IS_READ_WRITE(addr) ((uint32_t)adrr & PTE_WRITEABLE)
#define PTE_IS_USER(addr) ((uint32_t)addr & PTE_USER)
#define PTE_GET_FRAME(addr) ((uint32_t)addr & PTE_FRAME)
#define PTE_SET_FRAME(addrOfPtEntry, frame) (*(uint32_t*)addrOfPtEntry) = ((*(uint32_t*)addrOfPtEntry) & ~PTE_FRAME) | ((uint32_t)frame)

//macros refering to Page tables (Page Directory entry is a page table)
#define PDE_IS_PRESENT(addr) ((uint32_t)addr & PDE_PRESENT)
#define PDE_IS_READ_WRITE(addr) ((uint32_t)adrr & PDE_WRITEABLE)
#define PDE_IS_USER(addr) ((uint32_t)addr & PDE_USER)
#define PDE_IS_4MB(addr) ((uint32_t)addr & PDE_4MB)
#define PDE_GET_FRAME(addr) ((uint32_t)addr & PDE_FRAME)
#define PDE_SET_FRAME(addrOfPdEntry, frame) (*(uint32_t*)addrOfPdEntry) = ((*(uint32_t*)addrOfPdEntry) & ~PDE_FRAME) | ((uint32_t)frame)

//These macros are used to break down a virtual address into its physical parts. according to little os book section 9.2
#define PD_INDEX(virtual_address) ((uint32_t)virtual_address >> PAGE_TABLE_ENTRY_BITS >> PAGE_OFFSET_BITS) //takes a virtual address and finds the PDE of the virtual address
#define PT_INDEX(virtual_address) ((uint32_t)virtual_address >> PAGE_OFFSET_BITS) & 0x3FF //we need the bits from 12 to 21. so we move them to start, and then end & removes the bits used by the PDE
#define PAGE_INDEX(virtual_address) ((uint32_t)virtual_address & 0xFFF) //takes only the 12 bits on the right of the virtual address, which specify the index in the page.

#define SET_ATTRIBUTE(addr, attr) (*(uint32_t*)addr |= attr)
#define CLEAR_ATTRIBUTE(addr, attr) (*(uint32_t*)addr &= ~(attr))


/*
this function is responsible to initialize paging by setting the paging bit in cr0 register. it is also responsible to set up the page fault handler.
*/
void init_paging();

//virtual memory manager:
//these function releate to actions that are done by the virtual memory manager
//1. mapping or unmapping pages
//2. allocating and freeing pages.
//3. accessing pages and page tables.
//4. converting virtual addresses to physical ones.
//5. changing current_page_dir and flushing tlb addresses.

/*
this function initializes the virtual memory manager.
creating a page directory and calling init_paging with it as parameter. the page directory should identity map the first 4MB of memory, to continue execution.
return value: 1 if successful, otherwise 0.
*/
uint32_t initialize_vmm();

/*
this function takes a virtual address and returns its page.
it finds the PDE using the PD_INDEX macro, and then finds the page in that PDE using the PT_INDEX macro.
virtual_address: the address of the page
return value: the page as a pointer to a page_table_entry_t struct.
*/
page_table_entry_t* get_page(const void* virtual_address);


/*
this function takes a virtual address and returns the page table (PDE) the virtual address resides in.
virtual_address: the address we are indexing.
return value: a pointer to a page_dir_entry_t struct, which represents a page table.
*/
page_dir_entry_t* get_page_table(const void* virtual_address);


/*
this function takes a virtual address, and will assaign the physical address physical_address as the physical frame of the page.
if the page at virtual_address is not present, it will create it. if the page table of virtual_address is not present it will create it.
virtual_address: the virtual address that gets asigned a page.
physical_address: the address of the page to assaign, assuming 0x1000 alligned.
flags: the flags to use on the page in virtual_address
return value: 1 if success, else 0
*/
uint32_t map_page(void* virtual_address ,void* physical_address, const uint32_t flags, page_directory_t* dir);

/*
this function takes a page, and unmaps it from a frame. it will free the frame, set the 20 bits of the frame as clear, and will clear the PRESENT bit.
page: the page to unmap from its frame.
return value: None
*/
void unmap_page(void* virtual_address);

/*
this function takes a page and allocates a frame to it.
page: the page we allocate a frame for.
flags: the flags to set for the page taken.
return value: the address of the frame we allocated.
*/
void* allocate_page(page_table_entry_t* page, uint32_t flags);

/*
this function takes a page and frees the frame in it.
page: the page we free its frame
return value: None.
*/
void free_page(page_table_entry_t* page);

/*
this function takes a pointer to a page directory, and sets the page directory to be the current page directory.
edit the current_page_dir pointer, and modify cr3 register.
dir: the directory to set as current
return value: 1 if succsessful, otherwise 0.
*/
uint32_t set_page_directory(page_directory_t* dir);

/*
this function takes a virtual address, and flushes it from the TLB. the TLB is a table that stores cache about virtual->physical addresses, to improve runtime. if a page was chanegd, we need to flush the TLB entry of that address.
virtual_address: the virtual address to flush.
return value: None.
*/
void flush_tlb_address(const void* virtual_address);

/*
this function is used to flush the entire tlb, not a specific address.
this is done by resetting the cr3 register
implementation is basically moving him to some other register (eax for example), and then moving that register back to cr3.
*/
void flush_all_tlb();

/*
this function takes a virtual address and returns the physical address it represents.
for example, a virtual address 0xC879F345. to convert it to physical address:
find the page using getPage(0xC879F345 & 0xFFFFF000), and then get the index in the page using PAGE_INDEX, and then do page->frame | PAGE_INDEX and return the result.
*/
void* virtual_to_physical(const void* virtual_address);


/*
this function returns the current page directory
*/
page_directory_t* get_page_dir();