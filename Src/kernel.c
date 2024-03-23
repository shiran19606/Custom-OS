#include "gdt.h"
#include "idt.h"
#include "Screen.h"
#include "keyboard.h"
#include "fs.h"
#include "multiboot.h"
#include "pmm.h"
#include "vmm.h"
#include "ide.h"
#include "timer.h"
#include "process.h"

extern uint32_t kernel_physical_start;
extern uint32_t kernel_physical_end;
extern uint32_t kernel_virtual_start;
extern uint32_t kernel_virtual_end;
extern uint32_t KERNEL_VIRTUAL;

extern uint32_t PAGE_DIR_VIRTUAL;
extern uint32_t PAGE_DIR_PHYSICAL;

extern uint32_t* pmm_bitmap;
extern uint32_t* bitmap_end;

uint8_t waiting_for_input = 0;
MyFile* openedFile = 0;

void split_by_space(const char* string_to_split, char* buffer1, char* buffer2)
{
    uint32_t i = 0, j = 0;
    char* string1 = string_to_split;
    while (*string1 && *string1 != ' ')
    {
        buffer1[i] = *string1;
        i++;
        string1++;
    }
    if (*string1 && *string1 == ' ')
        string1++;
    while(*string1)
    {
        buffer2[j] = *string1;
        j++;
        string1++;
    }
}

void print_user_menu()
{
    kprintf("help - see this menu message\n");
    kprintf("echo - print a message to the screen\n");
    kprintf("ls - list a directory\n");
    kprintf("clear - clear the screen\n");
    kprintf("touch - create a new file\n");
    kprintf("cat - print a file content\n");
    kprintf("mkdir - create a new directory\n");
    kprintf("edit - edit the contents of a file, by overwriting the current file content\n");
}

void handle_user_input(const char* input)
{
    if (!waiting_for_input && strcmp(input, "help") == 0)
        print_user_menu();
    else if (!waiting_for_input && strcmp(input, "clear") == 0)
        clearScreen();
    else
    {
        uint8_t buffer1[256] = {0};
        uint8_t buffer2[256] = {0};
        split_by_space(input, buffer1, buffer2);
        if (waiting_for_input && openedFile)
        {
            writeToFile(openedFile, input);
            waiting_for_input = 0;
            closeFile(openedFile);
            openedFile = 0;
            kprintf("> ");
            return;
        }
        if (strcmp(buffer1, "ls") == 0)
            listDir(buffer2);
        if (strcmp(buffer1, "echo") == 0)
            kprintf("%s\n", buffer2);
        if (strcmp(buffer1, "cat") == 0)
        {
            openedFile = openFile(buffer2);
            if (openedFile)
            {
                char* buffer_to_read = (char*)readFromFile(openedFile);
                if (buffer_to_read)
                {
                    kprintf("%s\n", buffer_to_read);
                    kfree((void*)buffer_to_read);
                }
            }
        }
        if (strcmp(buffer1, "edit") == 0)
        {
            openedFile = openFile(buffer2);
            if (openedFile)
            {
                waiting_for_input = 1;
                return;
            }
        }
        if (strcmp(buffer1, "touch") == 0 || strcmp(buffer1, "mkdir") == 0)
            createFileOrDirectory(buffer2, buffer1[0] != 't'); //if buffer1[0] is not t it means we are creating a directory, so put isDir as true.
        if (openedFile)
        {
            closeFile(openedFile);
            openedFile = 0;
        }
    }
    kprintf("> ");
}

void func1(void)
{
    char ch = 'A';
    while(1) put_char(ch);
}

void func2(void)
{
    char ch = 'B';
    while(1) put_char(ch);
}

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
    multiboot_memory_map_t* memory_map = (multiboot_memory_map_t *)(mboot_ptr->mmap_addr);
    uint32_t num_entries = mboot_ptr->mmap_length / sizeof(multiboot_memory_map_t);
    
    //find virtual memory map.
    uint32_t kernel_base = &KERNEL_VIRTUAL;
    memory_map = ((uint32_t)memory_map + kernel_base);

    uint32_t memorySize = memory_map[num_entries-1].base_addr_low + (memory_map[num_entries-1].length_low-1);
    init_physical_memory(memorySize);

    //set regions of memory as free if they are free on the memory map. reserve the memory below 1MB as used.
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
    ide_initialize(0x1F0, 0x3F6, 0x170, 0x376, 0x000); //initialize disk driver to use in file system.
    init_multitasking();
    create_process(func1);
    create_process(func2);
    init_timer(1193);

    //initializing fs to use the current disk contents and not format it.
    init_fs(FS_DONT_FORMAT_DISK);

    //start menu.
    kprintf("Type 'help' to get the user menu\n");
    kprintf("> ");
    asm volatile("sti");
    while(1)
    {
        put_char(0x08);
    }
}