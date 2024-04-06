#include "syscall.h"

void* syscalls[MAX_SYSCALLS] = {0};

//TODO: test dispatcher with functions that take parameters.
static uint32_t syscall_dispatcher(registers_t* regs)
{
    if (!regs || regs->eax >= MAX_SYSCALLS)
        asm volatile("cli;hlt");
    
    void* func = syscalls[regs->eax];

    asm volatile(
        "push %0;"
        "cld;"
        "call %1;"
        "add $4, %%esp"
        : : "r"(*(uint32_t*)(regs->useresp+8)), "r"(func) : "%esp"
    );
    
    uint32_t result;
    asm volatile("movl %%eax, %0" : "=r" (result));
    return result;
}

void syscall_init()
{
    syscalls[0] = createFile;
    syscalls[1] = openFile;
    syscalls[2] = closeFile;
    syscalls[3] = readFromFile;
    syscalls[4] = writeToFile;
    syscalls[5] = createDirectory;
    syscalls[6] = listDir;
    syscalls[7] = create_process;
    syscalls[8] = terminate_process;
    syscalls[9] = kprintf;
    syscalls[10] = clearScreen;
    register_handler(SYSCALL_INT, syscall_dispatcher);
}