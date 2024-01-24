#include "gdt.h"
#include "idt.h"
#include "Screen.h"
#include "keyboard.h"
#include "fs.h"

void kernel_main(void) 
{
    const char* str = "Hello World";
    //initialize the descriptor tables
    init_gdt();
    init_idt();
    //clear the monitor from things that were written by GRUB.
    clearScreen();
    init_keyboard();
    initialize_allocator();
    //initializing fs
    init_fs(32, 64);

    //testing kprintf
    kprintf("Hello World!\n");

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