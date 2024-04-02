#ifndef SYSCALL_H
#define SYSCALL_h

#include "utils.h"
#include "isr.h"
#include "fs.h"
#include "process.h"

#define SYSCALL_INT 0x80

#define FS_OPEN     0
#define FS_CLOSE    1
#define FS_READ     2
#define FS_write    3
#define FS_LIST     4
#define PROC_CREATE 5
#define PROC_EXIT   6
#define PRINT       7
#define CLEAR_VGA   8

#define MAX_SYSCALLS 9

void syscall_init();

#endif