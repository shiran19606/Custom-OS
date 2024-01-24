#include "fs.h"

SuperBlock* sb;

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

// Function to allocate an inode
Inode* allocateInode(SuperBlock* sb) {
	int index = findFirstClearIndex((int*)sb->inodeBitmap, (int)sb->inodesCount);
	if (index == -1) {
		kprintf("No kfree inodes available.\n");
		return NULL;
	}

	setBit((int*)sb->inodeBitmap, index);
	Inode* newInode = (sb->inodesAddress + sizeof(Inode) * index);
	newInode->fileSize = 0;
	newInode->isDir = 0;
	newInode->numOfBlocksUsed = 0;
	return newInode;
}

// Function to write data to inode blocks
void writeData(Inode* inode, const char* data, uint32_t dataSize) {
	// Clear existing blocks assigned to the inode
	for (int i = 0; i < inode->numOfBlocksUsed; ++i) {
		// Clear block by clearing the corresponding bit in the block bitmap
		int index = ((uint32_t)inode->blocks[i] - sb->blocksAddress) / BLOCK_SIZE;
		clearBit(sb->blockBitmap, index);
		inode->blocks[i] = 0; // Reset block reference in inode
	}

	// Calculate required blocks to store data
	int requiredBlocks = (dataSize + BLOCK_SIZE - 1) / BLOCK_SIZE;

	// Check if data exceeds the capacity of the inode's three blocks
	if (requiredBlocks > POINTERS_PER_INODE) {
		kprintf("Data too large to fit within the inode's capacity.\n");
		return;
	}

	// Allocate new blocks to the inode for data storage
	for (int i = 0; i < requiredBlocks; ++i) {
		int kfreeBlockIndex = findFirstClearIndex((int*)sb->blockBitmap, (int)sb->blocksCount);
		if (kfreeBlockIndex == -1) {
			kprintf("No kfree blocks available for writing data.\n");
			return;
		}
		setBit((int*)sb->blockBitmap, kfreeBlockIndex); // Set the bit in the block bitmap
		inode->blocks[i] = (uint32_t)sb->blocksAddress + kfreeBlockIndex * BLOCK_SIZE; // Assign block number to the inode
	}

	int blockIndex = 0;
	for (int i = 0; i < dataSize; i += BLOCK_SIZE) {
		memcpy(inode->blocks[blockIndex], data + i, BLOCK_SIZE);
		blockIndex++;
	}
	// Update inode details
	inode->fileSize = dataSize;
	inode->numOfBlocksUsed = requiredBlocks;
}

int doesFileExist(const char* filepath) {
	Inode* root = ROOT_INODE(); // Get the root inode
	char pathBuffer[256] = {0}; // Create a buffer to extract the path
	extractPath(filepath, pathBuffer); // Function that extracts the path before the last '/'
	if (strlen(pathBuffer) == 1 && pathBuffer[0] == '/')
		memcpy(pathBuffer, filepath, strlen(filepath));
	// Break down the path to reach the directory where the file is supposed to be
	Inode* directory = breakDownPath(root, pathBuffer);

	if (directory == NULL) {
		return 0; // Directory doesn't exist or is not a directory
	}
	if (directory->isDir == 0)
		return 1; //we found the file

	// Get the filename from the filepath
	const char* filename = filepath + strlen(pathBuffer) + 1;

	// Search for the file in the directory
	directoryEntry* entries = getInodeContent(directory);
	uint8_t* ptr = entries;
	while (ptr && ((directoryEntry*)ptr)->inodeNumber) {
		if (strcmp(((directoryEntry*)ptr)->filename, filename) == 0) {
			kfree(entries);
			return 1; // File exists in the directory
		}
		ptr += sizeof(directoryEntry);
	}
	kfree(entries);
	return 0; // File doesn't exist in the directory
}


//in this function, the caller is responsible to kfree the memory allocated by kmalloc.
uint8_t* getInodeContent(Inode* inode)
{
	uint8_t* blocks = kmalloc(sizeof(Block) * POINTERS_PER_INODE); //3 blocks in each inode
	memset(blocks, 0, sizeof(Block) * POINTERS_PER_INODE);
	int j = 0;
	for (int i = 0; i < POINTERS_PER_INODE; i++)
	{
		unsigned char* ptr = inode->blocks[i];
		if (ptr)
		{
			for (int i = 0; i < BLOCK_SIZE; i++)
			{
				blocks[j] = *ptr;
				j++;
				ptr++;
			}
		}
	}
	return blocks;
}

void createFile(const char* filename)
{
	createFileOrDirectory(filename, 0);
}

void createDirectory(const char* dirname)
{
	createFileOrDirectory(dirname, 1);
}

void createFileOrDirectory(const char* filename, int isDir)
{
	if (filename[0] == '/') //if the path starts with / means the path starts from root, which is automatically so we dont need that char.
		filename++;
	char buff[MAX_FILENAME_LENGTH] = { 0 };
	//get the parent directory of the new file.
	Inode* root = ROOT_INODE();
	char* path = filename;
	char* tmp = filename;
	while (filename && *filename)
	{
		if (*filename == '/')
			tmp = filename + 1;
		filename++;
	}
	int j = 0;
	Inode* parent = root;
	for (int i = path; i < tmp - 1; i++)
	{
		buff[j++] = *((int*)i);
	}
	if (!checkPathLegit(parent, buff)) {
		kprintf("Path To create file %s in is not valid\n", path);
		return;
	}
	if (doesFileExist(path))
	{
		kprintf("File with path %s already exists\n", path);
		return;
	}
	if (*buff)
		parent = breakDownPath(root, buff);

	//get inode for new file and choose file or directory.
	Inode* inodeForNewFile = allocateInode(sb);
	inodeForNewFile->isDir = isDir;

	//add to parent directory.
	uint32_t inodeIndex = INDEX_FROM_INODE((uint32_t)inodeForNewFile);
	directoryEntry* newEntry = getInodeContent(parent);
	uint8_t* ptr = newEntry;
	uint32_t numEntries = 0;
	while (ptr && ((directoryEntry*)ptr)->inodeNumber)
	{
		ptr += sizeof(directoryEntry);
		numEntries++;
	}
	uint32_t i = 0;
	for (i = 0; i < MAX_FILENAME_LENGTH && tmp[i]; i++, ptr++)
		*ptr = tmp[i];
	for (i; i < MAX_FILENAME_LENGTH; i++, ptr++) //pad filename with nulls
		*ptr = 0;
	*ptr = inodeIndex;
	writeData(parent, (const char*)newEntry, (numEntries+1)*(sizeof(directoryEntry)));
	kfree((void*)newEntry);
}

Inode* getInodeFromName(Inode* current, const char* name)
{
	directoryEntry* entries = getInodeContent(current);
	uint8_t* ptr = entries;
	while (ptr && ((directoryEntry*)ptr)->inodeNumber)
	{
		Inode* entry = INODE_FROM_INDEX((((directoryEntry*)ptr))->inodeNumber)
		if (strcmp(((directoryEntry*)ptr)->filename, name) == 0)
		{
			return INODE_FROM_INDEX(((directoryEntry*)ptr)->inodeNumber);
		}
		ptr += sizeof(directoryEntry);
	}
	return NULL;
}

Inode* breakDownPath(Inode* parent, const char* path)
{
	char buffer[MAX_FILENAME_LENGTH] = { 0 };
	uint32_t j = 0;
	while (path)
	{
		if (!(*path))
			return getInodeFromName(parent, buffer);
		if (*path == '/')
		{
			Inode* tmp = getInodeFromName(parent, buffer);
			if (tmp)
				return breakDownPath(tmp, (path + 1));
			else
				return NULL;
		}
		else
		{
			buffer[j] = *path;
			j++;
			path++;
		}
	}
	return NULL;
}

// Function to check if a path is valid up to a certain point
int checkPathLegit(Inode* parent, const char* path) {
	if (!parent || !parent->isDir) {
		return 0;
	}
	int flag = 1;
	char* temp = path;
	while (*temp)
	{
		if (*temp == '/')
			flag = 0;
		temp++;
	}
	if (flag) return 1; //if there is no '/' in the name means there is no path to check if its legit, only a file to check if already exists in the root.
	Inode* currentDir = breakDownPath(parent, path);
	if (currentDir == NULL) {
		return 0;
	}

	return 1;
}

void writeToFile(MyFile* fileToWrite, const char* data)
{
	Inode* inode = INODE_FROM_INDEX((uint32_t)fileToWrite->inodeNumber);
	writeData(inode, data, strlen(data));
}

char* readFromFile(MyFile* fileToRead)
{
	Inode* inode = INODE_FROM_INDEX((uint32_t)fileToRead->inodeNumber);
	return getInodeContent(inode);
}

MyFile* openFile(const char* filepath)
{
	if (filepath[0] == '/') //if the path starts with / means the path starts from root, which is automatically so we dont need that char.
		filepath++;
	Inode* root = ROOT_INODE();
	if (!checkPathLegit(root, filepath) || !doesFileExist(filepath)) {
		kprintf("Cant open file %s\n");
		return NULL;
	}
	Inode* fileToRead = breakDownPath(root, filepath);
	MyFile* file = kmalloc(sizeof(MyFile));
	if (file)
		file->inodeNumber = INDEX_FROM_INODE((uint32_t)fileToRead);
	return file;
}

void closeFile(MyFile* file)
{
	if (file)
		kfree(file);
}

// Function to list directory entries for a given path
void listDirectory(const char* path) {
	if (path[0] == '/') // If the path starts with '/', skip it
		path++;


	Inode* root = ROOT_INODE();
	if (!checkPathLegit(root, path)) {
		kprintf("The directory %s cant be listed\n", path);
		return;
	}

	Inode* currentDir = root;
	if (*path) //if we ls the root, the path would now point to null, because the path was only "/" and we did path++ at the start. if *path it means path is not null
		currentDir = breakDownPath(root, path);
	// Check if the given path exists and points to a directory
	if (!currentDir || !currentDir->isDir) {
		kprintf("Path doesn't exist or is not a directory.\n");
		return NULL;
	}

	// Get the content of the directory (assuming it's an array of directoryEntry structs)
	directoryEntry* dirContent = (directoryEntry*)getInodeContent(currentDir);
	if (!dirContent) {
		kprintf("Failed to retrieve directory content.\n");
		return NULL;
	}
	uint8_t* ptr = dirContent;
	while (ptr && ((directoryEntry*)ptr)->inodeNumber)
	{
		kprintf("%s ", ((directoryEntry*)ptr)->filename);
		ptr += sizeof(directoryEntry);
	}
	kfree(dirContent);
}

void extractPath(const char* inputString, char* outputBuffer) {
	int i = 0, lastSlashIndex = -1;

	// Find the index of the last '/'
	while (inputString[i] != '\0') {
		if (inputString[i] == '/')
			lastSlashIndex = i;
		i++;
	}

	// If no '/' was found or the string starts with '/', set the output buffer to '/'
	if (lastSlashIndex == -1 || lastSlashIndex == 0) {
		outputBuffer[0] = '/';
		outputBuffer[1] = '\0';
		return;
	}

	// Copy characters up to the last '/'
	for (i = 0; i < lastSlashIndex; i++) {
		outputBuffer[i] = inputString[i];
	}
	outputBuffer[lastSlashIndex] = '\0';
}

void init_fs(uint32_t num_of_inodes, uint32_t num_of_blocks)
{
	sb = MEMSTART;
	sb->inodesCount = num_of_inodes;
	sb->blocksCount = num_of_blocks;
	sb->magicNumber = FS_MAGIC_NUMBER;
	sb->inodeBitmap = ((uint32_t)sb + SUPERBLOCK_SIZE);
	sb->blockBitmap = ((uint32_t)sb->inodeBitmap + num_of_inodes / BITS_PER_BYTE);
	sb->inodesAddress = ((uint32_t)sb->blockBitmap + num_of_blocks / BITS_PER_BYTE);;
	sb->blocksAddress = ((uint32_t)sb->inodesAddress + num_of_inodes * INODE_SIZE);
	allocateInode(sb); //allocate a dummy inode, as the first inode is reserved
	allocateInode(sb)->isDir = 1; //this is the inode for the root directory, so put isDir as true.
}