#ifndef VFS_H
#define VFS_H

#include "utils.h"
#include "heap.h"

#define MAX_FILES 16
#define MAX_FILESYSTEMS 4

#define O_RDONLY 00000001
#define O_WRONLY 00000002
#define O_RDWR   00000003
#define O_CREATE 00000100
#define O_APPEND 00001000

#define FS_DIR   1
#define FS_FILE  0

typedef struct FILE
{
    uint32_t file_id;
    uint32_t size;
    uint32_t flags;
    uint32_t file_system_driver;
    void* fs_data;
} FILE;

typedef int (*open)(const char* filename, FILE* file_descriptor, int flags);
typedef int (*read)(void* file, void* buffer, int count);
typedef int (*write)(void* file, void* buffer, int count);
typedef int (*close)(void* file);
typedef int (*mkdir)(const char* path, int flags);

typedef struct FILESYSTEM
{
    uint8_t name[8];
    open open;
    read read;
    write write;
    close close;
    mkdir mkdir;
} FILESYSTEM;

int vfs_open_file(const char* filename, int flags);
int vfs_read_file(int fd, void* buffer, int count);
int vfs_write_file(int fd, void* buffer, int count);
int vfs_close_file(int fd);
int vfs_mkdir(const char* path, int flags);

uint8_t add_fs(FILESYSTEM* fs);
uint8_t remove_fs(FILESYSTEM* fs);

int init_vfs();

#endif