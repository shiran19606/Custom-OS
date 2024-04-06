#include "fs.h"

SuperBlock current_sb;
SuperBlock* sb;
uint8_t drive_num;

//read the inode bitmap from physical disk
uint32_t* get_inode_bitmap()
{
	uint32_t* bitmap = (uint32_t*)kmalloc((uint32_t)(sb->inodesCount / BITS_PER_BYTE));
	ide_access(drive_num, sb->inodeBitmap, (sb->inodesCount / BITS_PER_BYTE), (uint32_t)bitmap, ATA_READ);
	return bitmap;
}

uint32_t* get_block_bitmap()
{
	uint32_t* bitmap = (uint32_t*)kmalloc((uint32_t)(sb->blocksCount / BITS_PER_BYTE));
	ide_access(drive_num, sb->blockBitmap, (sb->blocksCount / BITS_PER_BYTE), (uint32_t)bitmap, ATA_READ);
	return bitmap;
}

void set_inode_bitmap(uint32_t* bitmap)
{
	ide_access(drive_num, sb->inodeBitmap, (sb->inodesCount / BITS_PER_BYTE), (uint32_t)bitmap, ATA_WRITE);
}

void set_blcok_bitmap(uint32_t* bitmap)
{
	ide_access(drive_num, sb->blockBitmap, (sb->blocksCount / BITS_PER_BYTE), (uint32_t)bitmap, ATA_WRITE);
}

void read_inode(uint32_t index, Inode* inode)
{
	ide_access(drive_num, sb->inodesAddress + (index * INODE_SIZE), INODE_SIZE, (uint32_t)inode, ATA_READ);
}

void write_inode(uint32_t index, Inode* inode)
{
	ide_access(drive_num, sb->inodesAddress + (index * INODE_SIZE), INODE_SIZE, (uint32_t)inode, ATA_WRITE);
}

// Function to find the first clear index in the bitmap
int findFirstClearIndex(uint32_t* bitmap, uint32_t size) {
	for (uint32_t i = 0; i < size; ++i) {
		uint32_t mask = 1 << (i % (sizeof(uint32_t) * 8)); // Calculate the bit position within an integer
		if ((bitmap[i / (sizeof(uint32_t) * 8)] & mask) == 0) {
			return i;
		}
	}
	return -1; // Return -1 if no clear bit found
}

// Function to set a bit at a specific index in the bitmap
void setBit(uint32_t* bitmap, uint32_t index) {
	uint32_t word = index / (sizeof(uint32_t) * 8); // Calculate which word (integer) in the bitmap
	uint32_t bit = index % (sizeof(uint32_t) * 8); // Calculate which bit within the word

	bitmap[word] |= 1 << bit; // Set the bit by ORing with a mask
}

// Function to clear a bit at a specific index in the bitmap
void clearBit(uint32_t* bitmap, uint32_t index) {
	uint32_t word = index / (sizeof(uint32_t) * 8); // Calculate which word (integer) in the bitmap
	uint32_t bit = index % (sizeof(uint32_t) * 8); // Calculate which bit within the word

	bitmap[word] &= ~(1 << bit); // Clear the bit by ANDing with the complement of a mask
}

uint32_t get_inode_from_name_and_dir(Inode* dirInode, char* name)
{
	//add to parent directory.
	uint8_t* ptr = getInodeContent(dirInode); 
	uint8_t* newEntry = ptr;
	uint32_t result = 0, flag = 1;
	while (ptr && ((directoryEntry*)ptr)->inodeNumber && flag)
	{
		if (strcmp(((directoryEntry*)ptr)->filename, name) == 0)
		{
			result = ((directoryEntry*)ptr)->inodeNumber;
			flag = 0;
		}
		ptr += sizeof(directoryEntry);
	}
	kfree((void*)newEntry);
	return result;
}


uint32_t get_working_dir(Inode* root, char* path)
{
	uint32_t index = 1; //start from root.
	Inode* tmp = root;
	while (path && *path)
	{
		uint8_t arr[MAX_FILENAME_LENGTH] = {0};
		uint32_t i = 0;
		while (*path && *path != '/')
		{
			arr[i] = *path;
			i++;
			path++;
		}
		if (*path == '/') //if we reached '/'
		{
			index = get_inode_from_name_and_dir(tmp, arr);
			if (!index) //if we got index 0, it means the current dir path is not valid.
				return index;
			read_inode(index, tmp);
			path++;
		} 
	}
	return index;
}

uint32_t allocateInode()
{
	uint32_t* inode_bitmap = get_inode_bitmap();
	int index = findFirstClearIndex((uint32_t*)inode_bitmap, (int)sb->inodesCount);
	if (index == -1)
	{
		kprintf("Error: No free inodes available.\n");
		return NULL;
	}
	setBit((uint32_t*)inode_bitmap, index);
	set_inode_bitmap(inode_bitmap);
	kfree((void*)inode_bitmap);
	Inode inode1;
	read_inode(index, &inode1);
	inode1.fileSize = 0;
	inode1.isDir = 0;
	inode1.numOfBlocksUsed = 0;
	write_inode(index, &inode1);
	return index;
}

// Function to write data to inode blocks
void writeData(Inode* inode, const char* data, uint32_t dataSize) {
	// Clear existing blocks assigned to the inode
	uint32_t* block_bitmap = get_block_bitmap();
	for (int i = 0; i < inode->numOfBlocksUsed; ++i) {
		// Clear block by clearing the corresponding bit in the block bitmap
		int index = ((uint32_t)inode->blocks[i] - sb->blocksAddress) / FS_BLOCK_SIZE;
		clearBit((uint32_t*)block_bitmap, index);
		inode->blocks[i] = 0; // Reset block reference in inode
	}

	// Calculate required blocks to store data
	int requiredBlocks = (dataSize + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;

	// Check if data exceeds the capacity of the inode's three blocks
	if (requiredBlocks > POINTERS_PER_INODE)
	{
		kprintf("Error: Data too large to fit within the inode's capacity.\n");
		return;
	}

	// Allocate new blocks to the inode for data storage
	for (int i = 0; i < requiredBlocks; ++i) {
		int kfreeBlockIndex = findFirstClearIndex((uint32_t*)block_bitmap, (int)sb->blocksCount);
		if (kfreeBlockIndex == -1) 
		{
			kprintf("Error: No free blocks available for writing data.\n");
			return;
		}
		setBit((uint32_t*)block_bitmap, (uint32_t)kfreeBlockIndex); // Set the bit in the block bitmap
		inode->blocks[i] = (uint32_t)sb->blocksAddress + kfreeBlockIndex * FS_BLOCK_SIZE; // Assign block number to the inode
	}
	set_blcok_bitmap(block_bitmap);
	kfree((void*)block_bitmap);

	int blockIndex = 0;
	uint32_t size = dataSize;
	for (uint32_t i = 0; i < dataSize; i += FS_BLOCK_SIZE) 
	{
		uint32_t size_to_write = (size > FS_BLOCK_SIZE) ? FS_BLOCK_SIZE : size;
		ide_access(drive_num, inode->blocks[blockIndex], size_to_write, (data + i), ATA_WRITE);
		size -= size_to_write;
		blockIndex++;
	}
	// Update inode details
	inode->fileSize = dataSize;
	inode->numOfBlocksUsed = requiredBlocks;
}


//in this function, the caller is responsible to kfree the memory allocated by kmalloc.
uint8_t* getInodeContent(Inode* inode)
{
	uint8_t* blocks = (uint8_t*)kmalloc(sizeof(Block) * POINTERS_PER_INODE); //3 blocks in each inode
	memset((void*)blocks, 0, sizeof(Block) * POINTERS_PER_INODE);
	int j = 0;
	uint32_t size = inode->fileSize;
	for (int i = 0; i < inode->numOfBlocksUsed; i++)
	{
		uint32_t size_to_read = (size > FS_BLOCK_SIZE) ? FS_BLOCK_SIZE : size;
		ide_access(drive_num, inode->blocks[i], size_to_read, blocks + (i * FS_BLOCK_SIZE), ATA_READ);
		size_to_read -= FS_BLOCK_SIZE;
	}
	return blocks;
}

uint32_t createFile(const char* filename)
{
	createFileOrDirectory(filename, 0);
}

uint32_t createDirectory(const char* dirname)
{
	createFileOrDirectory(dirname, 1);
}

uint32_t createFileOrDirectory(const char* filename, int isDir)
{
	asm volatile("cli");
	if (filename[0] == '/') //if the path starts with / means the path starts from root, which is automatically so we dont need that char.
		filename++;
	char buff[MAX_FILENAME_LENGTH] = { 0 };
	//get the parent directory of the new file.
	Inode inode1;
	Inode* root = &inode1;

	char* path = filename;
	char* tmp = filename;
	while (filename && *filename)
	{
		if (*filename == '/')
			tmp = filename + 1;
		filename++;
	}
	int j = 0;

	//get inode for new file and choose file or directory.
	uint32_t index1 = allocateInode();
	Inode inodeForNewFile;
	read_inode(index1, &inodeForNewFile);
	inodeForNewFile.isDir = isDir;
	write_inode(index1, &inodeForNewFile);

	Inode inode2;
	read_inode(1, &inode2);
	uint32_t working_dir = get_working_dir(&inode2, path);
	if (working_dir == 0)
	{
		kprintf("Error: Path to create the file in is not valid\n");
		return 1;
	}
	read_inode(working_dir, root);

	//add to parent directory.
	uint8_t* ptr = getInodeContent(root); 
	directoryEntry* newEntry = (directoryEntry*)ptr;
	uint32_t numEntries = 0;
	uint32_t flag = 1;
	while (ptr && ((directoryEntry*)ptr)->inodeNumber)
	{
		if (strcmp(((directoryEntry*)ptr)->filename, tmp) == 0)
		{
			kprintf("Error: File with name %s already exists in directory\n", tmp);
			flag = 0;
		}
		ptr += sizeof(directoryEntry);
		numEntries++;
	}
	if (flag)
	{
		uint32_t i = 0;
		for (i = 0; i < MAX_FILENAME_LENGTH && tmp[i]; i++, ptr++)
			*ptr = tmp[i];
		for (i; i < MAX_FILENAME_LENGTH; i++, ptr++) //pad filename with nulls
			*ptr = 0;
		*ptr = index1;
	}
	writeData(root, (const char*)newEntry, (numEntries+1)*(sizeof(directoryEntry)));
	write_inode(working_dir, root);
	kfree((void*)newEntry);
	return 0;
}

uint32_t writeToFile(MyFile* fileToWrite, const char* data)
{
	asm volatile("cli");
	Inode inode;
	read_inode(fileToWrite->inodeNumber, &inode);
	if (inode.isDir)
	{
		kprintf("Cant Write to a directory\n");
		return 1;
	}
	writeData(&inode, data, strlen(data));
	write_inode(fileToWrite->inodeNumber, &inode);
	return 0;
}

uint8_t* readFromFile(MyFile* fileToRead)
{
	asm volatile("cli");
	Inode inode;
	read_inode(fileToRead->inodeNumber, &inode);
	if (inode.isDir)
	{
		kprintf("Cant read from a directory\n");
		return NULL;
	}
	return getInodeContent(&inode);
}

MyFile* openFile(char* filename)
{
	asm volatile("cli");
	if (filename[0] == '/') //if the path starts with / means the path starts from root, which is automatically so we dont need that char.
		filename++;
	
	char* path = filename;
	char* tmp = filename;
	while (filename && *filename)
	{
		if (*filename == '/')
			tmp = filename + 1;
		filename++;
	}

	Inode inode2;
	read_inode(1, &inode2);
	uint32_t working_dir = get_working_dir(&inode2, path);
	read_inode(working_dir, &inode2);
	uint32_t indexOfFileInode = get_inode_from_name_and_dir(&inode2, tmp);
	if (indexOfFileInode == 0)
	{
		kprintf("Error: cant open file %s\n", tmp);
		return NULL;
	}

	Inode inode_tmp;
	read_inode(indexOfFileInode, &inode_tmp);
	if (inode_tmp.isDir)
	{
		kprintf("Cant open a directory\n");
		return NULL;
	}

	MyFile* file = (MyFile*)kmalloc(sizeof(MyFile));
	file->inodeNumber = indexOfFileInode;
	return file;
}

uint32_t closeFile(MyFile* file1)
{
	asm volatile("cli");
	if (file1)
		kfree(file1);
	return 0;
}

uint32_t listDir(char* path)
{
	asm volatile("cli");
	if (path[0] == '/') //if the path starts with / means the path starts from root, which is automatically so we dont need that char.
		path++;

	Inode inode2;
	read_inode(1, &inode2);

	//this code here is used to create a string to call get_working_dir with. if the user entered Dir1 to list the directory Dir1, the get_working_dir terminates the string by '/' so it expects to see '/' at the end, so we add a '/' to a tmp string.
	uint32_t size_path = strlen(path);
	char* str = (char*)kmalloc(size_path + 2);
	memcpy(str, path, size_path);
	str[size_path] = '/';
	str[size_path + 1] = 0;

	uint32_t working_dir = 1;
	if (strcmp(str, "/") != 0)
		working_dir = get_working_dir(&inode2, str);
	kfree((void*)str);

	if (working_dir == 0)
	{
		kprintf("Error: cant list directory\n");
		return 1;
	}
	read_inode(working_dir, &inode2);

	if (!inode2.isDir)
	{
		kprintf("cant ls a file\n");
		return 1;
	}

	//add to parent directory.
	uint8_t* ptr = getInodeContent(&inode2); 
	uint8_t* newEntry = ptr;

	kprintf("Listing directory at path: /%s\n", path);
	
	while (ptr && ((directoryEntry*)ptr)->inodeNumber)
	{
		kprintf("%s ", ((directoryEntry*)ptr)->filename);
		ptr += sizeof(directoryEntry);
	}
	kprintf("\n");
	kfree((void*)newEntry);
	return 0;
}


void format_fs()
{
	kprintf("Formating disk\n");
	sb->inodesCount = INODES_COUNT_DEFAULT;
	sb->blocksCount = BLOCKS_COUNT_DEFAULT;
	sb->inodeBitmap = SUPERBLOCK_SIZE;
	sb->blockBitmap = sb->inodeBitmap + (sb->inodesCount / BITS_PER_BYTE);
	sb->inodesAddress = sb->blockBitmap + (sb->blocksCount / BITS_PER_BYTE);
	sb->blocksAddress = sb->inodesAddress + (sb->inodesCount * INODE_SIZE);
	sb->magicNumber = FS_MAGIC_NUMBER;
	uint8_t buff[SECTOR_SIZE] = {0};
	ide_access(drive_num, 0, SUPERBLOCK_SIZE, sb, ATA_WRITE);
	ide_access(drive_num, sb->inodeBitmap, (sb->blockBitmap - sb->inodeBitmap), buff, ATA_WRITE);
	ide_access(drive_num, sb->blockBitmap, (sb->inodesAddress - sb->blockBitmap), buff, ATA_WRITE);

	uint32_t index = allocateInode(); //allocate a dummy inode, as the first inode is reserved
	index = allocateInode(); //this is the inode for the root directory, so put isDir as true.
	Inode inode1;
	read_inode(index, &inode1);
	inode1.isDir = 1;
	write_inode(index, &inode1);
}

void init_fs(uint8_t format_disk)
{
	drive_num = PRIMARY_SLAVE;
	sb = &current_sb;
	ide_access(drive_num, 0, SUPERBLOCK_SIZE, sb, ATA_READ); //read superblock information from disk

	if (sb->magicNumber != FS_MAGIC_NUMBER || format_disk)
		format_fs();
}