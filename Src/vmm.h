#include <stdint.h>

#define ENTRIES_IN_PAGE_TABLE 1024
#define ENTRIES_IN_PAGE_DIR 1024

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

//a struct representing an entry in the page directory - a page table
typedef struct page_dir_entry {
    uint32_t present : 1;
    uint32_t rw : 1;
    uint32_t user : 1;
    uint32_t w_through : 1;
    uint32_t cache : 1;
    uint32_t access : 1;
    uint32_t reserved : 1;
    uint32_t page_size : 1;
    uint32_t global : 1;
    uint32_t available : 3;
    uint32_t frame : 20;
} page_dir_entry_t;

//a struct representing an entry in the page table - a page 
typedef struct page_table_entry {
    uint32_t present : 1;
    uint32_t rw : 1;
    uint32_t user : 1;
    uint32_t reserved : 2;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t reserved2 : 2;
    uint32_t available : 3;
    uint32_t frame : 20;
} page_table_entry_t;

typedef struct page_table
{
    page_table_entry_t pages[ENTRIES_IN_PAGE_TABLE];
} page_table_t;

typedef struct page_directory
{
    page_dir_entry_t pages[ENTRIES_IN_PAGE_DIR];
} page_directory_t;

page_directory_t* current_page_dir = 0; //physical address of the current page directory

//macros refering to Pages (Page table entry is a page)
#define PTE_PRESENT(addr) (addr & PTE_PRESENT)
#define PTE_READ_WRITE(addr) (adrr & PTE_WRITEABLE)
#define PTE_USER(addr) (addr & PTE_USER)
#define PTE_GET_FRAME(addr) (addr & PTE_FRAME)
#define PTE_SET_FRAME(addrOfPtEntry, frame) (*(uint32_t*)addrOfPtEntry) = ((*(uint32_t*)addrOfPtEntry) & ~PTE_FRAME) | ((uint32_t)frame)

//macros refering to Page tables (Page Directory entry is a page table)
#define PDE_PRESENT(addr) (addr & PDE_PRESENT)
#define PDE_READ_WRITE(addr) (adrr & PDE_WRITEABLE)
#define PDE_USER(addr) (addr & PDE_USER)
#define PDE_GET_FRAME(addr) (addr & PDE_FRAME)
#define PDE_IS_4MB(addr) (addr & PDE_4MB)
#define PDE_SET_FRAME(addrOfPtEntry, frame) (*(uint32_t*)addrOfPtEntry) = ((*(uint32_t*)addrOfPtEntry) & ~PDE_FRAME) | ((uint32_t)frame)
