#include "gdt.h"
#include "idt.h"
#include "Screen.h"
#include "keyboard.h"
#include "fs.h"
#include "multiboot.h"
#include "pmm.h"
#include "vmm.h"
#include "ide.h"

extern uint32_t kernel_physical_start;
extern uint32_t kernel_physical_end;
extern uint32_t kernel_virtual_start;
extern uint32_t kernel_virtual_end;
extern uint32_t KERNEL_VIRTUAL;

extern uint32_t PAGE_DIR_VIRTUAL;
extern uint32_t PAGE_DIR_PHYSICAL;

extern uint32_t* pmm_bitmap;
extern uint32_t* bitmap_end;

void kernel_main(multiboot_info_t* mboot_ptr) 
{
    const char* str = "Hello World";
    //initialize the descriptor tables
    init_gdt();
    init_idt();
    //clear the monitor from things that were written by GRUB.
    clearScreen();
    kprintf("Initialized GDT and IDT\n");
    init_keyboard();
    kprintf("Initialized Keyboard Input\n");
    multiboot_memory_map_t * memory_map = (multiboot_memory_map_t *)(mboot_ptr->mmap_addr);
    uint32_t num_entries = mboot_ptr->mmap_length / sizeof(multiboot_memory_map_t);
    
    uint32_t kernel_base = &KERNEL_VIRTUAL;
    memory_map = ((uint32_t)memory_map + kernel_base);
    uint32_t memorySize = memory_map[num_entries-1].base_addr_low + (memory_map[num_entries-1].length_low-1);
    init_physical_memory(memorySize);
    //set regions of memory as free if they are free on the memory map
    for (uint32_t i = RESERVE_MEMORY_BELOW_1MB; i < num_entries; i++) {
        if (memory_map[i].type == MEMORY_MAP_REGION_FREE)
            init_region_free(memory_map[i].base_addr_low, memory_map[i].base_addr_low + memory_map[i].length_low);
    }
    //set memory used by the kernel and the bitmap as used.
    init_region_used((const uint32_t)&kernel_physical_start, (const uint32_t)&kernel_physical_end);
    init_region_used((const uint32_t)((uint32_t)pmm_bitmap - kernel_base), (const uint32_t)((uint32_t)bitmap_end - kernel_base)); //calculate physical addresses for the bitmap
    
    kprintf("Initialized physical memory\n");
    uint8_t result = initialize_vmm();

    if (!result)
        asm volatile("cli;hlt");
    kprintf("Initialized virtual memory\n");

    initialize_allocator();
    ide_initialize(0x1F0, 0x3F6, 0x170, 0x376, 0x000);

    //initializing fs
    init_fs(FS_FORMAT_DISK);
    
    //testing file and directory creation
	createFile("/File1");
	createDirectory("/dir1");
    createFile("/dir1/File2");

    //testing creation of a file that already exists, and a file in an invalid path
    createFile("File1");
    createFile("/dir2/File3");
    
    //testing file opening, writing, reading and closing.
    MyFile* file1 = openFile("/dir1/File2");
    if (file1 != NULL)
    {
        writeToFile(file1, "Hello World");
        uint8_t* data = readFromFile(file1);
        kprintf("Data of file is %s\n", data);
        kfree((void*)data);
    }
    closeFile(file1);
    
    //testing ls.
    listDir("/");
    listDir("/dir1/");


    kprintf("1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n");
    asm volatile("sti");
}