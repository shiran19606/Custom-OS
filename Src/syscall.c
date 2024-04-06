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
        : : "r"((uint32_t*)(regs->useresp+8)), "r"(func) : "%esp"
    );
    
    uint32_t result;
    asm volatile("movl %%eax, %0" : "=r" (result));
    return result;
}

uint32_t create_file(void* params)
{
    char* filename = *(char**)params;
    return createFile(filename);
}

uint32_t open_file(void* params)
{
    char* filename = *(char**)params;
    return openFile(filename);
}

uint32_t close_file(void* params)
{
    MyFile* file_descriptor = *(MyFile**)params;
    return closeFile(file_descriptor);
}

uint32_t read_file(void* params)
{
    MyFile* file_descriptor = *(MyFile**)params;
    return readFromFile(file_descriptor);
}

uint32_t write_file(void* params)
{
    MyFile* file_descriptor = *(MyFile**)params;
    params += sizeof(MyFile*);
    const char* input = *(char**)params;
    return writeToFile(file_descriptor, input);
}

uint32_t create_dir(void* params)
{
    char* dirname = *(char**)params;
    return createDirectory(dirname);
}

uint32_t list_dir(void* params)
{
    char* dirname = *(char**)params;
    return listDir(dirname);
}

uint32_t proc_create(void* params)
{
    void* ent = *(void**)params;
    params += sizeof(void*);
    uint32_t ring = *(uint32_t*)params;
    params += sizeof(ring);
    int argc = *(int*)params;
    params += sizeof(int);
    char** argv = *(char***)params;
    return create_process(ent, ring, argc, argv);
}

uint32_t write_screen(void* params)
{
    char* format = *(char**)params;
    kprintf(format);
}

uint32_t proc_stop(void* params)
{
    terminate_process();
    return 0;
}

uint32_t clear_screen(void* params)
{
    clearScreen();
    return 0;
}


void syscall_init()
{
    syscalls[0] = create_file;
    syscalls[1] = open_file;
    syscalls[2] = close_file;
    syscalls[3] = read_file;
    syscalls[4] = write_file;
    syscalls[5] = create_dir;
    syscalls[6] = list_dir;
    syscalls[7] = proc_create;
    syscalls[8] = proc_stop;
    syscalls[9] = write_screen;
    syscalls[10] = clear_screen;
    register_handler(SYSCALL_INT, syscall_dispatcher);
}