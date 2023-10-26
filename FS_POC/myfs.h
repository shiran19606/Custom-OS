#ifndef __MYFS_H__
#define __MYFS_H__

#include <memory>
#include <vector>
#include <stdint.h>
#include "blkdev.h"

#define END_OF_INODES_SECTION 65536

class MyFs {
public:
	MyFs(BlockDeviceSimulator *blkdevsim_);

	/**
	 * dir_list_entry struct
	 * This struct is used by list_dir method to return directory entry
	 * information.
	 */

	typedef struct inode
	{
		int file_size; 
		int blocks[30] = {0}; //this array will store the location of all blocks a file is using to store its data, each block of max size 256 bytes, when the first byte is used to determine if the block is taken or not.
		int blockSize[30] = {0}; //this array will store the amount of bytes that are actually written in each block stored in the blocks array. for example, blockSize[0] will store the amount of blocks that are written in the block in blocks[0]
		int numOfBlocksUsed; //this variable will store the amount of blocks that a file is using.
		bool is_dir;
	} inode;

	struct dir_list_entry {
		/**
		 * The directory entry name
		 */
		std::string name;

		/**
		 * whether the entry is a file or a directory
		 */
		bool is_dir;

		/**
		 * File size
		 */
		int file_size;
	};
	typedef std::vector<struct dir_list_entry> dir_list;

	/**
	 * format method
	 * This function discards the current content in the blockdevice and
	 * create a fresh new MYFS instance in the blockdevice.
	 */
	void format();

	/**
	 * create_file method
	 * Creates a new file in the required path.
	 * @param path_str the file path (e.g. "/newfile")
	 * @param directory boolean indicating whether this is a file or directory
	 */
	void create_file(std::string path_str, bool directory);

	/**
	 * get_content method
	 * Returns the whole content of the file indicated by path_str param.
	 * Note: this method assumes path_str refers to a file and not a
	 * directory.
	 * @param path_str the file path (e.g. "/somefile")
	 * @return the content of the file
	 */
	std::string get_content(std::string path_str);

	/**
	 * set_content method
	 * Sets the whole content of the file indicated by path_str param.
	 * Note: this method assumes path_str refers to a file and not a
	 * directory.
	 * @param path_str the file path (e.g. "/somefile")
	 * @param content the file content string
	 */
	void set_content(std::string path_str, std::string content);

	/**
	 * list_dir method
	 * Returns a list of a files in a directory.
	 * Note: this method assumes path_str refers to a directory and not a
	 * file.
	 * @param path_str the file path (e.g. "/somedir")
	 * @return a vector of dir_list_entry structures, one for each file in
	 *	the directory.
	 */
	dir_list list_dir(std::string path_str);

private:

	/**
	 * This struct represents the first bytes of a myfs filesystem.
	 * It holds some magic characters and a number indicating the version.
	 * Upon class construction, the magic and the header are tested - if
	 * they both exist than the file is assumed to contain a valid myfs
	 * instance. Otherwise, the blockdevice is formated and a new instance is
	 * created.
	 */
	struct myfs_header {
		char magic[4];
		uint8_t version;
	};

	BlockDeviceSimulator *blkdevsim;

	static const uint8_t CURR_VERSION = 0x03;
	static const char *MYFS_MAGIC;

	//private help functions.
	void writeMessage(std::string strToWrite, inode& inodeOfFile, int whereToStart); //this function will perform the writing of the file's content. it will do it recursivly because I use blocks of 256 so if I want to write a file bigger than 256 bytes, we write 256 of it and then call the function again with the rest of the content.
	void freeOldBlocksUsed(inode& inodeOfFile); //writing a message will override the older one, so if new message is smaller then the older message, we need to free the extra blocks
	std::string getFileNamesAndInodes(); //the function will return a string of all file names and inodes in this format: "filename:inodeNumber,filename:inodeNumber....."
	std::string getInodeFromName(std::string name); //the function takes a file name and returns a string with the name and the inode if the file.
	std::string getAllInodeContent(inode& inodeOfFile); //the function runs on an inode and returns all its content.
	bool checkIsValidName(std::string name); //this function checks that the names of the file doesnt exist
	int getWorkingDir(std::string path_str, inode& inode);
	std::string getInodeFromNameAndDir(std::string name, inode& dir); //this function takes a name as parameter and returns a string of the name and the inode number of the file.

};

#endif // __MYFS_H__
