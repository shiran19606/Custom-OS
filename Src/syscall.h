#ifndef SYSCALL_H
#define SYSCALL_h

#include "utils.h"
#include "isr.h"
#include "vfs.h"
#include "process.h"

#define SYSCALL_INT 0x80

#define FS_CREATE   0
#define FS_OPEN     1
#define FS_CLOSE    2
#define FS_SEEK     3
#define FS_READ     4
#define FS_WRITE    5
#define FS_MKDIR    6
#define FS_LIST     7
#define PROC_CREATE 8
#define PROC_EXIT   9
#define CLEAR_VGA   10

#define MAX_SYSCALLS 11

void syscall_init();

#endif