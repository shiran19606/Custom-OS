#include "syscall.h"

void* syscalls[MAX_SYSCALLS] = {0};

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

uint32_t proc_stop(void* params)
{
    terminate_process();
    return 0; //shouldnt be reached, but just in case.
}

uint32_t clear_screen(void* params)
{
    clearScreen();
    return 0;
}


void syscall_init()
{
    syscalls[FS_CREATE      ] = create_file;
    syscalls[FS_OPEN        ] = open_file;
    syscalls[FS_CLOSE       ] = close_file;
    syscalls[FS_READ        ] = read_file;
    syscalls[FS_WRITE       ] = write_file;
    syscalls[FS_MKDIR       ] = create_dir;
    syscalls[FS_LIST        ] = list_dir;
    syscalls[PROC_CREATE    ] = proc_create;
    syscalls[PROC_EXIT      ] = proc_stop;
    syscalls[CLEAR_VGA      ] = clear_screen;
    register_handler(SYSCALL_INT, syscall_dispatcher);
}