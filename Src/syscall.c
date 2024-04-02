#include "syscall.h"

void* syscalls[MAX_SYSCALLS] = {0};

static void syscall_dispatcher(registers_t* regs)
{
    static int index = 0;
    asm volatile (
        "movl %%eax, %0;" 
        : "=r" (index)      
    );

    if (index >= MAX_SYSCALLS)
        asm volatile("cli;hlt");
    void* func = syscalls[index];
    kprintf("useresp is %x, esp is %x\n", regs->useresp, regs->esp);
    return;
}

void syscall_init()
{
    syscalls[0] = openFile;
    syscalls[1] = closeFile;
    syscalls[2] = readFromFile;
    syscalls[3] = writeToFile;
    syscalls[4] = listDir;
    syscalls[5] = create_process;
    syscalls[6] = terminate_process;
    syscalls[7] = kprintf;
    syscalls[8] = clearScreen;
    register_handler(SYSCALL_INT, syscall_dispatcher);
}