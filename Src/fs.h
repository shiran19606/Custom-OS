#ifndef FS_H
#define FS_H

#include "utils.h"
#include "vmm.h"
#include "heap.h"
#include "ide.h"

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
	uint32_t inodeNumber;
} MyFile;

typedef struct dirEntry //in a directory, each file in the directory will have a dirEntry.
{
	char filename[MAX_FILENAME_LENGTH];
	uint32_t inodeNumber;
} directoryEntry;


//helper functions.
void createFileOrDirectory(const char* filename, int isDir);
uint8_t* getInodeContent(Inode* inode);

//user functions
void createFile(const char* filename);
void createDirectory(const char* dirname);
void writeToFile(MyFile* fileToWrite, const char* data);
char* readFromFile(MyFile* fileToRead);
MyFile* openFile(char* filename);
void closeFile(MyFile* file1);
void listDir(char* path);

//init functions
void init_fs(uint8_t format_disk);

#endif