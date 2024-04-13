#include "fs.h"

SuperBlock current_sb;
SuperBlock* sb;
uint8_t drive_num;

FILESYSTEM my_fs;

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
int writeData(Inode* inode, const char* data, int dataSize) {
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
		return -1;
	}

	// Allocate new blocks to the inode for data storage
	for (int i = 0; i < requiredBlocks; ++i) {
		int kfreeBlockIndex = findFirstClearIndex((uint32_t*)block_bitmap, (int)sb->blocksCount);
		if (kfreeBlockIndex == -1) 
		{
			kprintf("Error: No free blocks available for writing data.\n");
			return -1;
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
	return dataSize;
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

int createFile(const char* filename)
{
	createFileOrDirectory(filename, 0);
}

int createDirectory(const char* dirname)
{
	createFileOrDirectory(dirname, 1);
}

int createFileOrDirectory(const char* filename, int isDir)
{
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

int writeToFile(MyFile* fileToWrite, const char* data, uint32_t len)
{
	int size_read = -1;
	Inode inode;
	read_inode(fileToWrite->inodeNumber, &inode);
	if (inode.isDir)
	{
		kprintf("Cant Write to a directory\n");
		return -1;
	}
	if (fileToWrite->offset + len > POINTERS_PER_INODE * FS_BLOCK_SIZE)
	{
		kprintf("CONTENT TOO BIG\n");
		return -1;
	}
	if (fileToWrite->offset + len <= fileToWrite->fileSize)
	{
		uint32_t block = fileToWrite->offset / FS_BLOCK_SIZE;
		uint32_t offset_in_block = fileToWrite->offset % FS_BLOCK_SIZE;
		uint32_t location_to_write = inode.blocks[block] + offset_in_block;
		ide_access(drive_num, location_to_write, len, data, ATA_WRITE);
		fileToWrite->offset += len;
		size_read = fileToWrite->fileSize;
	}
	else
	{
		uint8_t* data2 = getInodeContent(&inode);
		if (fileToWrite->fileSize == 0) //if the file was truncated.
			memset(data2, 0, sizeof(Block) * POINTERS_PER_INODE);
		memcpy(data2 + fileToWrite->offset, data, len);
		size_read = writeData(&inode, data2, inode.fileSize + len);
		fileToWrite->offset = inode.fileSize;
		fileToWrite->fileSize = inode.fileSize;
		write_inode(fileToWrite->inodeNumber, &inode);
		kfree((void*)data2);
	}
	return size_read;
}

int readFromFile(MyFile* fileToRead, uint8_t* buffer, uint32_t len)
{
	Inode inode;
	read_inode(fileToRead->inodeNumber, &inode);
	if (inode.isDir)
	{
		kprintf("Cant read from a directory\n");
		return NULL;
	}
	uint8_t* buffer2 = getInodeContent(&inode);
	if (fileToRead->offset == fileToRead->fileSize)
		return -1; //EOF
	if (len > (fileToRead->fileSize - fileToRead->offset))
		len = fileToRead->fileSize - fileToRead->offset;
	memcpy(buffer, buffer2 + fileToRead->offset, len);
	kfree((void*)buffer2);
	return len;
}

MyFile* openFile(char* filename)
{
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
		return NULL;

	Inode inode_tmp;
	read_inode(indexOfFileInode, &inode_tmp);
	if (inode_tmp.isDir)
		return FS_DIR;

	MyFile* file = (MyFile*)kmalloc(sizeof(MyFile));
	file->inodeNumber = indexOfFileInode;
	file->offset = 0;
	file->fileSize = inode_tmp.fileSize;
	return file;
}

int closeFile(MyFile* file1)
{
	if (file1)
		kfree(file1);
	return 0;
}

int listDir(char* path)
{
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
		return -1;
	}
	read_inode(working_dir, &inode2);

	if (!inode2.isDir)
	{
		kprintf("cant ls a file\n");
		return -1;
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

int Open(const char* filename, FILE* file_descriptor, int flags)
{
	MyFile* file1 = openFile(filename);
	if (file1 == FS_DIR)
	{
		kprintf("Cant open a directory\n");
		return -1;
	}
	if (file1 == NULL && (file_descriptor->flags & O_CREATE) == O_CREATE)
	{
		createFile(filename);
		file1 = openFile(filename);
	}
	else if (file1 == NULL)
	{
		kprintf("%s does not exist\n", filename);
		return -1;
	}
	
	//if we reached here, it means the file was opened successfuly.
	if ((file_descriptor->flags & O_APPEND) != O_APPEND) //if the append flag is not present, truncated the contents of the file.
		file1->fileSize = 0;
	file1->offset = file1->fileSize;  //we have two cases. we either truncated the file (so no contents) or we opened with append flag and then the offset is the size cause writing to the end of the file.

	file_descriptor->flags = flags;
	file_descriptor->fs_data = (void*)file1;
	file1->flags = flags;
	file_descriptor->size = file1->fileSize;
	return 0;
}

int Read(void* file, void* buffer, int count)
{
	MyFile* file1 = (MyFile*)file;
	if (file1->flags & (O_RDONLY | O_RDWR))
		return readFromFile(file1, buffer, count);
	return -1;
}

int Write(void* file, void* buffer, int count)
{
	MyFile* file1 = (MyFile*)file;
	if (file1->flags & (O_WRONLY | O_RDWR))
		return writeToFile(file1, buffer, count);
	return -1;
}

int Close(void* file)
{
	MyFile* file1 = (MyFile*)file;
	return closeFile(file1);
}

int Seek(void* file, int offset, int whence)
{
	MyFile* file1 = (MyFile*)file;
	switch (whence)
	{
	case SEEK_SET:
		file1->offset = offset;
		break;
	case SEEK_CUR:
		file1->offset += offset;
		break;
	case SEEK_END:
		file1->offset = file1->fileSize + offset;
		break;
	default:
		return -1;
		break;
	}
	if (file1->offset >= file1->fileSize)
	{
		void* new_content = kmalloc(file1->offset);
		memset(new_content, 0, file1->offset);
		Inode inode;
		read_inode(file1->inodeNumber, &inode);
		void* old_content = getInodeContent(&inode);
		memcpy(new_content, old_content, file1->fileSize);
		memset(new_content + file1->fileSize, 0, file1->offset + 1 - file1->fileSize);
		writeData(&inode, new_content, file1->offset);
		file1->fileSize = file1->offset;
		write_inode(file1->inodeNumber, &inode);
		kfree(new_content);
		kfree(old_content);	
	}
	return 0;
}

int Mkdir(const char* path, int flags)
{
	return createDirectory(path);
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
	my_fs.open = Open;
	my_fs.close = Close;
	my_fs.seek = Seek;
	my_fs.read = Read;
	my_fs.write = Write;
	my_fs.mkdir = Mkdir;

	drive_num = PRIMARY_SLAVE;
	sb = &current_sb;
	ide_access(drive_num, 0, SUPERBLOCK_SIZE, sb, ATA_READ); //read superblock information from disk

	if (sb->magicNumber != FS_MAGIC_NUMBER || format_disk)
		format_fs();

	add_fs(&my_fs);
}