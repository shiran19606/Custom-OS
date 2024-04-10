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

int create_file(void* params)
{
    char* filename = *(char**)params;
    return vfs_open_file(filename, (O_RDONLY | O_CREATE));
}

int open_file(void* params)
{
    char* filename = *(char**)params;
    params += sizeof(char**);
    int flags = *(int*)params;
    return vfs_open_file(filename, flags);
}

int close_file(void* params)
{
    int fd = *(int*)params;
    return vfs_close_file(fd);
}

int read_file(void* params)
{
    int fd = *(int*)params;
    params += sizeof(int*);
    char* buffer = *(char**)params;
    params += sizeof(char**);
    int count = *(int*)params;
    return vfs_read_file(fd, buffer, count);
}

int write_file(void* params)
{
    int fd = *(int*)params;
    params += sizeof(int*);
    char* buffer = *(char**)params;
    params += sizeof(char**);
    int count = *(int*)params;
    return vfs_write_file(fd, buffer, count);
}

int create_dir(void* params)
{
    char* dirname = *(char**)params;
    return vfs_mkdir(dirname, 0);
}

int list_dir(void* params)
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
    uint32_t exit_code = *(uint32_t*)params;
    terminate_process(exit_code);
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