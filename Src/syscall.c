#include "syscall.h"

void* syscalls[MAX_SYSCALLS] = {0};

//TODO: test dispatcher with functions that take parameters.
static void syscall_dispatcher(registers_t* regs)
{
    if (regs->eax >= MAX_SYSCALLS)
        asm volatile("cli;hlt");
    
    void* func = syscalls[regs->eax];

    asm volatile(
        "push %0;"
        "cld;"
        "call %1;"
        "add $4, %%esp"
        : : "r"(regs->useresp), "r"(func) : "%esp"
    );
    asm volatile("mov %%eax, %0" : "=r"(regs->eax));
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