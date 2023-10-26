#include "myfs.h"
#include <string.h>
#include <iostream>
#include <math.h>
#include <sstream>

#define START_OF_DATA_SECTION 65536 //size of 256 inodes and 5 bytes for the signature at the start.

std::vector<std::string> split_str(std::string cmd)
{
	std::stringstream ss(cmd);
	std::string part;
	std::vector<std::string> ans;

	while (std::getline(ss, part, ','))
		ans.push_back(part);

	return ans;
}

std::vector<std::string> splitString(const std::string& str, char del) {
    std::vector<std::string> result;
    std::istringstream ss(str);
    std::string token;

    while (std::getline(ss, token, del)) {
        result.push_back(token);
    }

    return result;
}

const char *MyFs::MYFS_MAGIC = "MYFS";

MyFs::MyFs(BlockDeviceSimulator *blkdevsim_):blkdevsim(blkdevsim_) {
	struct myfs_header header;
	blkdevsim->read(0, sizeof(header), (char *)&header);

	if (strncmp(header.magic, MYFS_MAGIC, sizeof(header.magic)) != 0 ||
	    (header.version != CURR_VERSION)) {
		std::cout << "Did not find myfs instance on blkdev" << std::endl;
		std::cout << "Creating..." << std::endl;
		format();
		std::cout << "Finished!" << std::endl;
	}
}

void MyFs::format() {

	// put the header in place
	struct myfs_header header;
	strncpy(header.magic, MYFS_MAGIC, sizeof(header.magic));
	header.version = CURR_VERSION;
	blkdevsim->write(0, sizeof(myfs_header), (const char*)&header);

	inode root; //create inode for root file. root file will have only one type of data: a table of file name and its inode number.
	root.file_size = 0;
	root.is_dir = true;
	root.numOfBlocksUsed = 0;

	int num = 1, num2 = 0;
	blkdevsim->write(START_OF_DATA_SECTION, 1, (const char*)&num); //write 1 at the first byte in the data section, to state that this block is already taken for the root folder data.
	root.blocks[root.numOfBlocksUsed] = START_OF_DATA_SECTION;
	root.blockSize[root.numOfBlocksUsed] = 0; //in the first block that was assigned to the root there is currently no data.
	
	for (int i = (sizeof(myfs_header) + sizeof(inode) + 1);i<START_OF_DATA_SECTION-256;i+=(2+sizeof(inode))) //create all inodes and put them free so that new files can take over an inode.
	{
		inode temp;
		temp.is_dir = false; //the inode is surely not a directory because directorys are not supported.
		temp.file_size = 0; 
		temp.numOfBlocksUsed = 0;
		blkdevsim->write(i, 1, (const char*)&num2); //write 0 to state that the current inode is free to use.
		blkdevsim->write(i+1, sizeof(inode), (const char*)&temp); //write the inode to the memory.
	}
	//now that all inodes are created, as well as the root folder node, we can start working.
	blkdevsim->write(sizeof(myfs_header), sizeof(inode), (const char*)&root); //put the root inode right after the signature. after creation root has no data so no need to write anything.
}

void MyFs::create_file(std::string path_str, bool directory) {
	//the next few lines are used to take the root inode from the file (first inode that is written, just after the signature).
	inode workingDir;
	int location = getWorkingDir(path_str, workingDir);
	
	std::string nameOfFile = splitString(path_str, '/').back();
	try
	{
		std::string nameAndInode = getInodeFromNameAndDir(nameOfFile, workingDir); //this call will throw an exception if the file doesnt exist already, which means we can create it. so catch the exception and create the file. if the call doesnt throw an exception, which means the file exists, we throw and excpetion because we cant create the same file twice.
	}
	catch(const std::exception& e)
	{
		//these next 15 lines are used to find a free inode for the file and take over it. the taking over is changing from 0 to 1 to state that the inode is taken, and adding the inode and the name to the inode-name list.
		int num2 = 1, inodeNumber = 1; //start from 1 because inode 0 is root folder
		bool foundInode = false;
		for (int i = (sizeof(myfs_header) + sizeof(inode) + 1); (i<START_OF_DATA_SECTION-256 && !foundInode);i+=(2+sizeof(inode)))
		{
			int num = 1;
			blkdevsim->read(i, 1, (char*)&num); 
			if (num == 0) //if num was changed to 0 then the inode is free.
			{
				blkdevsim->write(i, 1, (const char*)&num2);  //put 1 to state that this inode was just taken. no need to read the inode because it doesnt get modified.
				foundInode = true;
				if (directory)
				{
					inode temp;
					blkdevsim->read(i+1, sizeof(inode), (char*)&temp);
					temp.is_dir = true;
					blkdevsim->write(i+1, sizeof(inode), (const char*)&temp);
				}
			}
			else
				inodeNumber++;
		}
		if (!foundInode)
			throw std::runtime_error("no inodes free.");

		//write all the names as the root inode data
		std::vector<std::string> vec = splitString(path_str, '/'); //split the string by '/' to get all directories.
		std::string names = getAllInodeContent(workingDir) + vec.back() + ":" + std::to_string(inodeNumber) + ",", currentBlock = ""; //all the names is all the old names plus the new name.
		freeOldBlocksUsed(workingDir);
		writeMessage(names, workingDir, START_OF_DATA_SECTION);

		//update the root after modifying it:
		workingDir.file_size = names.length(); //the data of the root is the names and inodes, so the file size is the length of the names and inodes.
		blkdevsim->write(location, sizeof(inode), (const char*)&workingDir);
		return; //this return is used to not throw the exeption
	}
	throw std::runtime_error("File already exists in the folder");
}


std::string MyFs::get_content(std::string path_str) {
	inode workingDir;
	int location = getWorkingDir(path_str, workingDir);

	std::string nameOfFile = splitString(path_str, '/').back();
	std::string nameAndInode = getInodeFromNameAndDir(nameOfFile, workingDir);
	int inodeNum = std::stoi(nameAndInode.substr(nameAndInode.find(':')+1));
	location = (sizeof(myfs_header)) + inodeNum * (2+sizeof(inode));
	inode fileInode;
	blkdevsim->read(location, sizeof(inode), (char*)&fileInode);
	if (fileInode.is_dir)
		throw std::runtime_error("Cant print a directory");
	return getAllInodeContent(fileInode);
}

void MyFs::set_content(std::string path_str, std::string content)
{
	inode workingDir;
	getWorkingDir(path_str, workingDir);
	std::string nameOfFile = splitString(path_str, '/').back();

	std::string nameAndInode = getInodeFromNameAndDir(nameOfFile, workingDir);
	int inodeNum = std::stoi(nameAndInode.substr(nameAndInode.find(':')+1));
	int location = (sizeof(myfs_header)) + inodeNum * (2+sizeof(inode));
	inode fileInode;
	blkdevsim->read(location, sizeof(inode), (char*)&fileInode);
	if (fileInode.is_dir)
		throw std::runtime_error("Cant edit a directory");
	
	freeOldBlocksUsed(fileInode);
	writeMessage(content, fileInode, START_OF_DATA_SECTION);

	//now that the file is in the memory, just need to update the new inode of the file, and file writing is good.
	fileInode.file_size = content.length();
	blkdevsim->write(location, sizeof(inode), (const char*)&fileInode);
}

MyFs::dir_list MyFs::list_dir(std::string path_str) {
	inode workingDir;
	getWorkingDir(path_str, workingDir);
	std::string nameOfDirToList = splitString(path_str, '/').back();
	if (nameOfDirToList != "")
	{
		std::string nameAndInode = getInodeFromNameAndDir(nameOfDirToList, workingDir);
		int inodeNum = std::stoi(nameAndInode.substr(nameAndInode.find(':')+1));
		int location = (sizeof(myfs_header)) + inodeNum * (2+sizeof(inode));
		blkdevsim->read(location, sizeof(inode), (char*)&workingDir);
	}
	if (workingDir.is_dir == false)
		throw std::runtime_error("Cant list a file, only directories.");
	dir_list ans;
	std::vector<std::string> splitted = split_str(getAllInodeContent(workingDir));
	for (int i = 0;i<(int)splitted.size();i++)
	{
		std::string name = splitted[i].substr(0, splitted[i].find(':'));
		int inodeNum = std::stoi(splitted[i].substr(splitted[i].find(':')+1));
		int location = (sizeof(myfs_header)) + inodeNum * (2+sizeof(inode));
		inode temp;
		blkdevsim->read(location, sizeof(inode), (char*)&temp);
		dir_list_entry temp2;
		temp2.is_dir = temp.is_dir;
		temp2.file_size = temp.file_size;
		temp2.name = name;
		ans.push_back(temp2);
	}
	return ans;
}

//explaination about third parameter in text file.
void MyFs::writeMessage(std::string strToWrite, inode& inodeOfFile, int whereToStart) //this function will perform the writing of the file's content. it will do it recursivly because I use blocks of 256 so if I want to write a file bigger than 256 bytes, we write 256 of it and then call the function again with the rest of the content.
{
	if (strToWrite.length() > 0)
	{
		int num = 1, num2 = 1, i = 0;
		bool blockFound = false;
		for (i = whereToStart;i<(1024*1024) && !blockFound;i+=256) //in each block of 256 we only write 255 bytes, the first one is used to determine if the block is taken or not.
			{
				blkdevsim->read(i, 1, (char*)&num);
				std::string oneBlock = strToWrite.substr(0, 255);
				if (num == 0) //if num is 0 then the block is free.
				{
					blkdevsim->write(i, 1, (const char*)&num2);  //put 1 to state that this block is now taken
					blkdevsim->write(i+1, oneBlock.length(), oneBlock.c_str()); //write the data in the new memory block.
					inodeOfFile.blocks[inodeOfFile.numOfBlocksUsed] = i+1; //save the start of the new block we just got.
					inodeOfFile.blockSize[inodeOfFile.numOfBlocksUsed] = oneBlock.length(); //also save the amount of bytes we wrote to the new block.
					blockFound = true;
					inodeOfFile.numOfBlocksUsed++;
					if (inodeOfFile.numOfBlocksUsed == 30) //if numOfBlocksUsed reached 30 it means there is no space left in the inode to store the blocks
						throw std::runtime_error("No enough space to write");
					if (strToWrite.length() > 255) //if the string is too big for one block, we wrote one block and then we call the function again but with the rest of the file.
					{
						std::string theRest = strToWrite.substr(255); //take the file from the 255 bytes to the end
						writeMessage(theRest, inodeOfFile, i + 256);
					}
				}
			}
		if (!(i<(1024*1024))) //if the loop finished without writing
			throw std::runtime_error("No enough space to write");
	}
}

void MyFs::freeOldBlocksUsed(inode& inodeOfFile) //writing a message will override the older one, so if new message is smaller then the older message, we need to free the extra blocks
{
	int num = 0;
	for (int i = 0;i<=inodeOfFile.numOfBlocksUsed;i++)
	{
		if (inodeOfFile.blocks[i] != 0) //means there is a block that is currently in use by the file
		{
			blkdevsim->write(inodeOfFile.blocks[i]-1, 1, (const char*)&num); //write 0 to state that the current block is free to use. i-1 is because in writeMessage, we save the start of the data and not the start of the block. each block is 1 byte that says if its free or not, and then the data. if we want to get the one byte, we need to take the startOfData-1
			inodeOfFile.blocks[i] = 0;
			inodeOfFile.blockSize[i] = 0;
		}
	}
	inodeOfFile.numOfBlocksUsed = 0;
}

std::string MyFs::getFileNamesAndInodes() //the function will return a string of all file names and inodes in this format: "filename:inodeNumber,filename:inodeNumber....."
{
	inode root;
	blkdevsim->read(sizeof(myfs_header), sizeof(inode), (char*)&root);
	return getAllInodeContent(root); //get all the cintent on the root inode.
}

std::string MyFs::getInodeFromName(std::string name) //this function takes a name as parameter and returns a string of the name and the inode number of the file.
{
	std::vector<std::string> splitted = split_str(getFileNamesAndInodes());
	for (int i = 0;i<(int)splitted.size();i++)
		{
			std::string nameOnly = splitted[i].substr(0, splitted[i].find(':'));
			if (nameOnly == name)
				return splitted[i];
		}
	throw std::runtime_error("File not in system"); //if we made it here it means the name was not found and that the file is not in the system
}

std::string MyFs::getInodeFromNameAndDir(std::string name, inode& dir) //this function takes a name as parameter and returns a string of the name and the inode number of the file.
{
	std::vector<std::string> splitted = split_str(getAllInodeContent(dir));
	for (int i = 0;i<(int)splitted.size();i++)
		{
			std::string nameOnly = splitted[i].substr(0, splitted[i].find(':'));
			if (nameOnly == name)
				return splitted[i];
		}
	throw std::runtime_error("File not in system"); //if we made it here it means the name was not found and that the file is not in the system
}

std::string MyFs::getAllInodeContent(inode& inodeOfFile) //the function runs on an inode and returns all its content.
{
	std::string data = "";
	for (int i = 0;i <= inodeOfFile.numOfBlocksUsed;i++) //add together all the data of the file. the adding uses all the blocks, which are stored in the inode, and the size of each block, which is also stored in the inode.
	{
		if (inodeOfFile.blocks[i] != 0 && inodeOfFile.blockSize[i] > 0) //if the block has data in it
		{
			char* temp = new char[inodeOfFile.blockSize[i] + 1];
			blkdevsim->read(inodeOfFile.blocks[i], inodeOfFile.blockSize[i], temp);
			temp[inodeOfFile.blockSize[i]] = '\0';
			data += std::string(temp);
			delete temp;
		}
	}
	return data;
}

bool MyFs::checkIsValidName(std::string name) //this function will check if a file already exist with the given name.
{
	std::vector<std::string> splitted = split_str(getFileNamesAndInodes());
	for (int i = 0;i<(int)splitted.size();i++)
		if (splitted[i].substr(0, splitted[i].find(':')) == name)
			return false;
	return true;
}

int MyFs::getWorkingDir(std::string path_str, inode& inode1)
{
	int inodeNum = 0, location = sizeof(myfs_header); //set start location as root inode location.
	std::vector<std::string> vec = splitString(path_str, '/'); //split the string by '/' to get all directories.
	blkdevsim->read(sizeof(myfs_header), sizeof(inode), (char*)&inode1); //read the root to the inode.
	while (vec.size() > 1)
	{
		std::string nameAndInode = getInodeFromNameAndDir(vec[0], inode1);
		inodeNum = std::stoi(nameAndInode.substr(nameAndInode.find(':')+1));
		location = (sizeof(myfs_header)) + inodeNum * (2+sizeof(inode));
		blkdevsim->read(location, sizeof(inode), (char*)&inode1);
		vec.erase(vec.begin(), vec.begin()+1); //remove first element
	}
	return location;
}