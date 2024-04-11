#include "vfs.h"


int file_id = 0;

FILE fs_files[MAX_FILES];

FILESYSTEM* file_systems[MAX_FILESYSTEMS] = {0};

int8_t get_file_descriptor(uint32_t id)
{
    for (int i = 0;i<MAX_FILES;i++)
        if (fs_files[i].file_id == id)
            return i;
    return -1;
}

uint8_t add_fs(FILESYSTEM* fs)
{
    for (int i = 0; i < MAX_FILESYSTEMS; i++)
    {
        if (file_systems[i] == 0)
        {
            file_systems[i] = fs;
            return 0;
        }
    }
    return 1;
}

uint8_t remove_fs(FILESYSTEM* fs)
{
    for (int i = 0; i < MAX_FILESYSTEMS; i++)
    {
        if (file_systems[i] == fs)
        {
            file_systems[i] = 0;
            return 0;
        }
    }
    return 1;
}

int vfs_open_file(const char* filename, int flags)
{
    int8_t fd = get_file_descriptor(0);
    if (fd == -1)
        return 1;
    FILE* file = &(fs_files[fd]);
    file->flags = flags;
    file->size = 0;
    file->fs_data = 0;
    file->file_id = file_id++;
    file->file_system_driver = 0;
    FILESYSTEM* fs = file_systems[file->file_system_driver];
    int result = fs->open(filename, file, flags);
    if (result == 0)
        return fd;

    //open error.
    file->file_id = 0;
    file->flags = 0;
    return -1;
}

int vfs_read_file(int fd, void* buffer, int count)
{
    if (fd < 0 || fd >= MAX_FILES)
        return -1;
    FILE* file = &(fs_files[fd]);
    FILESYSTEM* fs = file_systems[file->file_system_driver];
    return fs->read(file->fs_data, buffer, count);
}

int vfs_write_file(int fd, void* buffer, int count)
{
    if (fd < 0 || fd >= MAX_FILES)
        return -1;
    FILE* file = &(fs_files[fd]);
    FILESYSTEM* fs = file_systems[file->file_system_driver];
    int result = fs->write(file->fs_data, buffer, count);
    if (result != -1)
        file->size = result;
    return result;
}

int vfs_close_file(int fd)
{
    if (fd < 0 || fd >= MAX_FILES)
        return -1;
    FILE* file = &(fs_files[fd]);
    file->file_id = 0;
    file->size = 0;
    file->flags = 0;
    FILESYSTEM* fs = file_systems[file->file_system_driver];
    file->file_system_driver = 0;
    return fs->close(file->fs_data);
}

int vfs_mkdir(const char* path, int flags)
{
    uint32_t device_id = 0;
    FILESYSTEM* fs = file_systems[device_id];
    return fs->mkdir(path, flags);
}

int init_vfs()
{
    for (int i = 0;i<MAX_FILES;i++)
        memset(&fs_files[i], 0, sizeof(FILE));
    return 0;
}