#ifndef FS_H
#define FS_H

#include "utils.h"
#include "vmm.h"
#include "heap.h"
#include "ide.h"
#include "vfs.h"

#define MAX_FILENAME_LENGTH 28

#define FS_MAGIC_NUMBER 0xFACE

#define BITS_PER_BYTE 8
#define POINTERS_PER_INODE 3

#define SUPERBLOCK_SIZE sizeof(SuperBlock)
#define INODE_SIZE sizeof(Inode)
#define FS_BLOCK_SIZE sizeof(Block)

#define INODES_COUNT_DEFAULT 32
#define BLOCKS_COUNT_DEFAULT 128

#define FS_DONT_FORMAT_DISK 0
#define FS_FORMAT_DISK 1

typedef struct SuperBlock
{
	uint32_t magicNumber;
	uint32_t inodesCount;
	uint32_t blocksCount;
	uint32_t inodeBitmap;
	uint32_t blockBitmap;
	uint32_t inodesAddress;
	uint32_t blocksAddress;
} SuperBlock;

typedef struct Inode
{
	uint16_t fileSize;
	uint8_t isDir;
	uint8_t numOfBlocksUsed;
	uint32_t blocks[3];
} Inode;

typedef struct Block
{
	uint8_t block[1024];
} Block;

typedef struct MyFile
{
	uint32_t flags;
	uint32_t inodeNumber;
	uint32_t offset;
	uint32_t fileSize;
} MyFile;

typedef struct dirEntry //in a directory, each file in the directory will have a dirEntry.
{
	char filename[MAX_FILENAME_LENGTH];
	uint32_t inodeNumber;
} directoryEntry;


//helper functions.
int createFileOrDirectory(const char* filename, int isDir);
uint8_t* getInodeContent(Inode* inode);

//user functions
int createFile(const char* filename);
int createDirectory(const char* dirname);
int writeToFile(MyFile* fileToWrite, const char* data, uint32_t len);
int readFromFile(MyFile* fileToRead, uint8_t* buffer, uint32_t len);
MyFile* openFile(char* filename);
int closeFile(MyFile* file1);
int listDir(char* path);

//init functions
void init_fs(uint8_t format_disk);

#endif