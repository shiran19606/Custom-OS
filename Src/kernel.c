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

    //testing memory allocation
    uint32_t ptr1 = kmalloc(10);
    uint32_t ptr2 = kmalloc(100);
    if (!ptr1 || !ptr2)
    {    
        kprintf("Error allocating memory");
        return;
    }
    kprintf("ptr1 is %x and ptr2 is %x\n", ptr1, ptr2);
    kfree(ptr1);
    kfree(ptr2);

    //testing assignment of values to allocated memory
    ptr1 = kmalloc(4);
    if (!ptr1)
    {    
        kprintf("Error allocating memory");
        return;
    }
    int* ptr = ptr1;
    *ptr = 10;
    kprintf("ptr1 is %x and its value is %d\n", ptr, *ptr);

    //testing strlen and memcpy functions
    ptr2 = kmalloc(strlen(str)+1);
    if (!ptr2)
    {    
        kprintf("Error allocating memory");
        return;
    }
    memcpy((void*)ptr2, str, strlen(str)+1);
    kprintf("The string in ptr2 is: %s\n", (char*)ptr2);

    //testing strcmp
    kprintf("strcmp with the string in ptr2 and str2 is %d\n", strcmp((const char*)ptr2, str));

    //testing memset
    memset((void*)ptr2, 'A', 2); //putting A as the first two chars in ptr2.
    kprintf("%s\n", (const char*)ptr2);

    kfree(ptr1);
    kfree(ptr2);

    //initializing fs
    init_fs(32, 64);
    
	createFile("/File1");
	createFile("File1");
	createDirectory("/dir1");
	createFile("/dir1/File3");
	createDirectory("dir1/dir1");
	createFile("/dir1/dir1/File4");
	MyFile* file1 = openFile("/File1");
	if (file1 == NULL)
	{
		kprintf("Error opening file1");
		return;
	}
	writeToFile(file1, "My World!");
	MyFile* file2 = openFile("/dir1/dir1/File4");
	if (file2 == NULL)
	{
		kprintf("Error opening file4");
		return;
	}
	writeToFile(file2, "My World Is File4!");
	kprintf("%s %s\n", readFromFile(file1), readFromFile(file2));
	listDirectory("/dir1/dir2");
	closeFile(file1);
	closeFile(file2);
    
    asm volatile("sti");
}