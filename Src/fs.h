#include "utils.h"
#include "heap.h"

#define MEMSTART 0x02FFFFFF

#define MAX_FILENAME_LENGTH 28

#define FS_MAGIC_NUMBER 0xFACE

#define BITS_PER_BYTE 8
#define POINTERS_PER_INODE 3

#define SUPERBLOCK_SIZE sizeof(SuperBlock)
#define INODE_SIZE sizeof(Inode)
#define FS_BLOCK_SIZE sizeof(Block)

#define INDEX_FROM_INODE(addr) (addr - sb->inodesAddress) / INODE_SIZE
#define INODE_FROM_INDEX(idx) (sb->inodesAddress + INODE_SIZE * (idx));
#define ROOT_INODE() INODE_FROM_INDEX(1)

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



void extractPath(const char* inputString, char* outputBuffer);
void createFileOrDirectory(const char* name, int isDir);
Inode* breakDownPath(Inode* parent, const char* path);
int checkPathLegit(Inode* parent, const char* path);
Inode* allocateInode(SuperBlock* sb);
void writeData(Inode* inode, const char* data, uint32_t dataSize);
int doesFileExist(const char* filepath);
uint8_t* getInodeContent(Inode* inode);
void createFile(const char* filename);
void createDirectory(const char* dirname);
Inode* getInodeFromName(Inode* current, const char* name);
void writeToFile(MyFile* fileToWrite, const char* data);
char* readFromFile(MyFile* fileToRead);
MyFile* openFile(const char* filepath);
void closeFile(MyFile* file);
void listDirectory(const char* path);
void init_fs(uint32_t num_of_inodes, uint32_t num_of_blocks);