#ifndef SYSCALL_H
#define SYSCALL_h

#include "utils.h"
#include "isr.h"
#include "fs.h"
#include "process.h"

#define SYSCALL_INT 0x80

#define FS_CREATE   0
#define FS_OPEN     1
#define FS_CLOSE    2
#define FS_READ     3
#define FS_WRITE    4
#define FS_MKDIR    5
#define FS_LIST     6
#define PROC_CREATE 7
#define PROC_EXIT   8
#define CLEAR_VGA   9

#define MAX_SYSCALLS 10

void syscall_init();

#endif