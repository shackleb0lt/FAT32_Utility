#ifndef FAT32_H
#define FAT32_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>

#define FAT12 0
#define FAT16 1
#define FAT32 2

/* boot sector constants */
#define BS_OEMName_LENGTH 8
#define BS_VolLab_LENGTH 11
#define BS_FilSysType_LENGTH 8

#define CMD_INFO "INFO"
#define CMD_DIR "DIR"
#define CMD_CD "CD"
#define CMD_GET "GET"
#define CMD_PUT "PUT"


//FAT constants
#define Partition_LBA_Begin 0 //first byte of the partition
#define ENTRIES_PER_SECTOR 16
#define DIR_SIZE 32 //in bytes
#define SECTOR_SIZE 64//in bytes
#define FAT_ENTRY_SIZE 4//in bytes
#define MAX_FILENAME_SIZE 8
#define MAX_EXTENTION_SIZE 3
//file table
#define TBL_OPEN_FILES 75 //Max size of open file table
#define TBL_DIR_STATS  75 //Max size of open directory table
//open file codes
#define MODE_READ 0
#define MODE_WRITE 1
#define MODE_BOTH 2
#define MODE_UNKNOWN 3 //when first created and directories

//generic
#define SUCCESS 0
#define ROOT  "/"
#define PARENT ".."
#define SELF "."
#define PATH_DELIMITER "/"
#define DEBUG_FLAG "-d"
bool DEBUG;

//Parsing
#define PERIOD 46
#define ALPHA_NUMBERS_LOWER_BOUND 48
#define ALPHA_NUMBERS_UPPER_BOUND 57
#define ALPHA_UPPERCASE_LOWER_BOUND 65
#define ALPHA_UPPERCASE_UPPER_BOUND 90
#define ALPHA_LOWERCASE_LOWER_BOUND 97
#define ALPHA_LOWERCASE_UPPER_BOUND 122

#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN  0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME 0x0F 


#define BUF_SIZE 256


/*
 * 
 * BEGIN STRUCTURE DEFINITIONS
 * 
 */ 

#pragma pack(push)
#pragma pack(1)
struct fat32BS_struct {
	char BS_jmpBoot[3];
	char BS_OEMName[BS_OEMName_LENGTH];
	uint16_t BPB_BytesPerSec;
	uint8_t BPB_SecPerClus;
	uint16_t BPB_RsvdSecCnt;
	uint8_t BPB_NumFATs;
	uint16_t BPB_RootEntCnt;
	uint16_t BPB_TotSec16;
	uint8_t BPB_Media;
	uint16_t BPB_FATSz16;
	uint16_t BPB_SecPerTrk;
	uint16_t BPB_NumHeads;
	uint32_t BPB_HiddSec;
	uint32_t BPB_TotSec32;
	uint32_t BPB_FATSz32;
	uint16_t BPB_ExtFlags;
	uint8_t BPB_FSVerLow;
	uint8_t BPB_FSVerHigh;
	uint32_t BPB_RootClus;
	uint16_t BPB_FSInfo;
	uint16_t BPB_BkBootSec;
	char BPB_reserved[12];
	uint8_t BS_DrvNum;
	uint8_t BS_Reserved1;
	uint8_t BS_BootSig;
	uint32_t BS_VolID;
	char BS_VolLab[BS_VolLab_LENGTH];
	char BS_FilSysType[BS_FilSysType_LENGTH];
	char BS_CodeReserved[420];
	uint8_t BS_SigA;
	uint8_t BS_SigB;
};
#pragma pack(pop)

typedef struct fat32BS_struct fat32BS;

#pragma pack(push)
#pragma pack(1)
struct DIR_ENTRY_struct{
    char filename[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
	uint8_t DIR_FstClusHI[2];
	uint16_t DIR_WrtTime;
	uint16_t DIR_WrtDate;
	uint8_t DIR_FstClusLO[2];
    uint32_t DIR_FileSize;
};
#pragma pack(pop)

typedef struct DIR_ENTRY_struct DIR_ENTRY;

#pragma pack(push)
#pragma pack(1)
struct FILEDESCRIPTOR {
    char filename[9];
    char extention[4];
    char parent[100];
    uint32_t firstCluster;
    int mode;
    uint32_t size;
    bool dir; //is it a directory
    bool isOpen;
    char fullFilename[13];
};
#pragma pack(pop)

typedef struct FILEDESCRIPTOR FILEDESCRIPTOR;

struct ENVIRONMENT {
    char pwd[100];			//current path
    char imageName[100]; //disk image name
    int pwd_cluster;		//current cluster
    uint32_t io_writeCluster; //< - deprecated
    int tbl_dirStatsIndex;
    int tbl_openFilesIndex;
    int tbl_dirStatsSize;
    int tbl_openFilesSize;
    char last_pwd[100];
    struct FILEDESCRIPTOR * openFilesTbl; //open file history table
    struct FILEDESCRIPTOR * dirStatsTbl; //directory history table
    
} environment;

int checkForFileError(int fd);

//-------------END BOOT SECTOR FUNCTIONS -----------------------
/* notes: the alias shown above the functions match up the variables from
 * the Microsoft FAT specificatiions
 */
  
//alias := rootDirSectors
int rootDirSectorCount(fat32BS * bs);

//alias := FirstDataSector
int firstDataSector(fat32BS * bs);

//alias := FirstSectorofCluster
uint32_t firstSectorofCluster(fat32BS * bs, uint32_t clusterNum);

/* decription: feed it a cluster and it returns the byte offset
 * of the beginning of that cluster's data
 * */
uint32_t byteOffsetOfCluster(fat32BS *bs, uint32_t clusterNum);

//alias := DataSec
int sectorsInDataRegion(fat32BS * bs);

//alias := CountofClusters
int countOfClusters(fat32BS * bs);

int getFatType(fat32BS * bs);

int firstFatTableSector(fat32BS *bs);

//the sector of the first clustor
//cluster_begin_lba
int firstClusterSector(fat32BS *bs);

 
//----------------OPEN FILE/DIRECTORY TABLES-----------------
/* create a new file to be put in the file descriptor table
 * */
FILEDESCRIPTOR * TBL_createFile(const char * filename, const char * extention, const char * parent, 
                                uint32_t firstCluster, int mode, uint32_t size, int dir,int isOpen);
/* adds a file object to either the fd table or the dir history table
 */ 
int TBL_addFileToTbl(FILEDESCRIPTOR * file, int isDir);
/* description: if <isDir> is set prints the content of the open files table
 * if not prints the open directory table
 */ 
int TBL_printFileTbl(int isDir);

/* desctipion searches the open directory table for entry if it finds it,
 * it returns the name of the parent directory, else it returns an empty string
*/
const char * TBL_getParent(const char * dirName);

/* description:  "index" is set to the index where the found element resides
 * returns true if file was found in the file table
 * 
 */ 
bool TBL_getFileDescriptor(int * index, const char * filename, bool isDir);
//-----------------------------------------------------------------------------
/*description: feed this a cluster and it returns an offset, in bytes, where the
 * fat entry for this cluster is locted
 * 
 * use: use the result to seek to the place in the image and read
 * the FAT entry
* */
uint32_t getFatAddressByCluster(fat32BS * bs, uint32_t clusterNum);

/*description: finds the location of this clusters FAT entry, reads
 * and returns the entry value
 * 
 * use: use the result to seek to the place in the image and read
 * the FAT entry
* */
uint32_t FAT_getFatEntry(fat32BS * bs, uint32_t clusterNum);
/* description: takes a value, newFatVal, and writes it to the destination
 * cluster, 
 */ 
int FAT_writeFatEntry(fat32BS * bs, uint32_t destinationCluster, uint32_t * newFatVal);

/* desctipion: NOT USED but might be in future to clear newly freed clusters
*/
// void clearCluster(fat32BS * bs, uint32_t targetCluster);

/*description:walks a FAT chain until 0x0fffffff, returns the number of iterations
 * which is the size of the cluster chiain
*/ 
int getFileSizeInClusters(fat32BS * bs, uint32_t firstClusterVal);

/*description: walks a FAT chain until returns the last cluster, where
* the current EOC is. returns the cluster number passed in if it's empty,
* , you should probly call getFirstFreeCluster if you intend to write to
* the FAT to keep things orderly
*/ 
uint32_t getLastClusterInChain(fat32BS * bs, uint32_t firstClusterVal);

/* description: traverses the FAT sequentially and find the first open cluster
 */ 
int FAT_findFirstFreeCluster(fat32BS * bs);

/* decription: returns the total number of open clusters
 */ 
int FAT_findTotalFreeCluster(fat32BS * bs);



/* description: pass in a entry and this properly formats the
 * "firstCluster" from the 2 byte segments in the file structure
 */
uint32_t buildClusterAddress(DIR_ENTRY * entry);

/* description: convert uint32_t to DIR_FstClusHI and DIR_FstClusLO byte array
 * and stores it into <entry>
 */ 
int deconstructClusterAddress(DIR_ENTRY * entry, uint32_t cluster);

/* description: takes a directory entry populates a file descriptor 
 * to be used in the file tables
 * */
FILEDESCRIPTOR * makeFileDecriptor(DIR_ENTRY * entry, FILEDESCRIPTOR * fd);
/* description: prints the contents of a directory entry
*/
// int showEntry(DIR_ENTRY * entry);

/* description: takes a cluster number where bunch of directories are and
 * the offst of the directory you want read and it will store that directory
 * info into the variable dir
 */ 
DIR_ENTRY * readEntry(fat32BS * bs, DIR_ENTRY * entry, uint32_t clusterNum, int offset);
/* description: takes a cluster number and offset where you want an entry written and write to that 
 * position
 */ 
DIR_ENTRY * writeEntry(fat32BS * bs, DIR_ENTRY * entry, uint32_t clusterNum, int offset);
/* description: takes a directory cluster and give you the byte address of the entry at
 * the offset provided. used to write dot entries. POSSIBLY REDUNDANT
 */ 
uint32_t byteOffsetofDirectoryEntry(fat32BS * bs, uint32_t clusterNum, int offset);

/* This is the workhorse. It is used for ls, cd, filesearching. 
 * Directory Functionality:
 * It is used to perform cd, which you use by setting <cd>. if <goingUp> is set it uses 
 * the open directory table to find the pwd's parent. if not it searches the pwd for
 * <directoryName>. If <cd> is not set this prints the contents of the pwd to the screen
 * 
 * Search Functionality
 * if <useAsSearch> is set this short circuits the cd functionality and returns true
 * if found
 * 
 * doDir(bs, environment.pwd_cluster, true, fileName, false, searchFile, true, searchForDirectory) 
 */ 

int doDir(fat32BS * bs, uint32_t clusterNum, bool cd, const char * directoryName, 
          bool goingUp, FILEDESCRIPTOR * searchFile, bool useAsSearch, bool searchForDir);

/* descriptioin: takes a FILEDESCRIPTOR and checks if the entry it was
 * created from is empty. Helper function
 */ 
bool isEntryEmpty(FILEDESCRIPTOR * fd);

/* description: walks a directory cluster chain looking for empty entries, if it finds one
 * it returns the byte offset of that entry. If it finds none then it returns -1;
 */ 
uint32_t FAT_findNextOpenEntry(fat32BS * bs, uint32_t pwdCluster);

/* description: wrapper for doDir. tries to change the pwd to <directoryName> 
 * This doesn't use the print to screen functinoality of doDir. if <goingUp> 
 * is set it uses the open directory table to find the pwd's parent
 */ 
int doCD(fat32BS * bs, const char * directoryName, int goingUp,  FILEDESCRIPTOR * searchFile);
/* desctipion: wrapper that uses doDir to search a pwd for a file and puts its info
 * into searchFile if found. if <useAsFileSearch> is set it disables the printing enabling
 * it to be used as a file search utility. if <searchForDirectory> is set it excludes
 * files from the search
 */ 
bool searchOrPrintFileSize(fat32BS * bs, const char * fileName, bool useAsFileSearch,
 						bool searchForDirectory, FILEDESCRIPTOR * searchFile);
/* desctipion: wrapper that uses doDir to search a pwd for a file and puts its info
 * into searchFile if found. Prints result for both file and directories
 */ 
bool printFileOrDirectorySize(fat32BS * bs, const char * fileName, FILEDESCRIPTOR * searchFile);

/* description: wrapper for searchOrPrintFileSize searchs pwd for filename. 
 * returns true if found, if found the resulting entry is put into searchFile
 * for use outside the function. if <searchForDirectory> is set it excludes
 * files from the search
 */ 
bool searchForFile(fat32BS * bs, const char * fileName, bool searchForDirectory, FILEDESCRIPTOR * searchFile);

/* decription: feed this a cluster number that's part of a chain and this
 * traverses it to find the last entry as well as finding the first available
 * free cluster and adds that free cluster to the chain
 */ 
uint32_t FAT_extendClusterChain(fat32BS * bs,  uint32_t clusterChainMember);

/* desctipion: pass in a free cluster and this marks it with EOC and sets 
 * the environment.io_writeCluster to the end of newly created cluster 
 * chain
*/
int FAT_allocateClusterChain(fat32BS * bs,  uint32_t clusterNum);

/* desctipion: pass in the 1st cluster of a chain and this traverses the 
 * chain and writes FAT_FREE_CLUSTER to every link, thereby freeing the 
 * chain for reallocation
*/
int FAT_freeClusterChain(fat32BS * bs,  uint32_t firstClusterOfChain);
/* description: feed this a cluster that's a member of a chain and this
 * sets your environment variable "io_writeCluster" to the last in that
 * chain. The thinking is this is where you'll write in order to grow
 * that file. before writing to cluster check with FAT_findNextOpenEntry()
 * to see if there's space or you need to extend the chain using FAT_extendClusterChain()
 */ 
int FAT_setIoWriteCluster(fat32BS * bs, uint32_t clusterChainMember) ;

/* description: this will write a new entry to <destinationCluster>. if that cluster
 * is full it grows the cluster chain and write the entry in the first spot
 * of the new cluster. if <isDotEntries> is set this automatically writes both
 * '.' and '..' to offsets 0 and 1 of <destinationCluster>
 * 
 */ 
int writeFileEntry(fat32BS * bs, DIR_ENTRY * entry, uint32_t destinationCluster, bool isDotEntries);


/* description: takes a directory entry and all the necesary info
	and populates the entry with the info in a correct format for
	insertion into a disk.
*/
int createEntry(DIR_ENTRY * entry, const char * filename, const char * ext,
			int isDir, uint32_t firstCluster, uint32_t filesize,
            bool emptyAfterThis, bool emptyDirectory);

/* description: take two entries and cluster info and populates	them with 
 * '.' and '..' entry information. The entries are	ready for writing to 
 * the disk after this. helper function for mkdir()
  */
int makeSpecialDirEntries(DIR_ENTRY * dot, DIR_ENTRY * dotDot,
						uint32_t newlyAllocatedCluster, uint32_t pwdCluster );

/* returns offset of the entry, <searchName> in the pwd 
 * if found return the offset, if not return -1 
 */ 
int getEntryOffset(fat32BS * bs, const char * searchName);

/* description: prints info about the image to the screen
 */ 
int printInfo(fat32BS * bs);

/* description: DOES NOT allocate a cluster chain. Writes a directory 
 * entry to <targetDirectoryCluster>. Error handling is done in the driver
 */ 
int touch(fat32BS * bs, const char * filename, const char * extention, uint32_t targetDirectoryCluster);

/* success code: 0 if fclose succeeded
 * error code 1: no filename given
 * error code 2: filename not found in pwd
 * error code 3: filename was never opened
 * error code 4: filename is currently closed 
 * 
 * description: attempts to change the status of <filename> in the open file table
 * from open to closed. see error codes above for assumptions
 * 
 */ 
int fClose(fat32BS * bs, int argC, const char * filename);
/* success code: 0 if fclose succeeded
 * error code 1: no filename given
 * error code 2: filename is a directory
 * error code 3: filename was not found
 * error code 4: filename is currently open 
 * error code 5: invalid option given
 * error code 6: file table was not updated
 * 
 * description: attempts to add <filename> to the open file table. if <filename> isn't
 * currently open and a valid file mode <option> is given the file is added to the file table.
 * <modeName> is passed in for the purose of displaying the selected mode to the user outside
 * of this function. See error codes above for other assumptions.
 * 
 * Assumptions: modeName must be at least 21 characters
 */ 
int fOpen(fat32BS * bs, int argC, const char * filename, const char * option);
/* description: takes a cluster number of the first cluster of a file and its size. 
 * this function will write <dataSize> bytes starting at position <pos>. 
 */ 
int FWRITE_writeData(fat32BS * bs, uint32_t firstCluster, uint32_t pos, const char * data, uint32_t dataSize);
/* description: takes a cluster number of a file and its size. this function will read <dataSize>
 * bytes starting at position <pos>. It will print <dataSize> bytes to the screen until the end of
 * the file is reached, as determined by <fileSize>
 */ 
int FREAD_readData(fat32BS * bs, uint32_t firstCluster, uint32_t pos, uint32_t dataSize, uint32_t fileSize, const char *filename );


/* success code: 0 if fwrite succeeded
 * error code 1: filename is a directory
 * error code 2: filename was never opened
 * error code 3: filename is not currently open
 * error code 4: no data to be written
 * error code 5: not open for writing
 * error code 6: filename does not exist in pwd
 * error code 7: empty file entry could allocated space
 * error code 8: file entry could be updated w/ new file size
 */ 
 /* description: Takes a filename, checks if file has been opened for writing. If so
  * it uses FWRITE_writeData() to write <data> to <filename>. If <position> is larger 
  * than the current filesize this grows it to the size required and fills the gap created with 0x20 (SPACE).
  * After writing if the filesize has changed this updates the directory entry of <filename> the new size
  */ 
int fWrite(fat32BS * bs, const char * filename, uint32_t pos, const char * data,uint32_t dataSize);

/* success code: 0 if fwrite succeeded
 * error code 1: filename is a directory
 * error code 2: filename was never opened
 * error code 3: filename is not currently open
 * error code 4: position is greater than DIR_FileSize
 * error code 5: not open for reading
 * error code 6: filename does not exist in pwd
 */ 
 /* description: Takes a filename, checks if file has been opened for reading. If so
  * it uses FREAD_readData() to print <numberOfBytes> to the screen, starting at <position>
  * 'th byte. <actualBytesRead> is to be passed back to the caller
  */ 
int fRead(fat32BS * bs, const char * filename, const char * position, uint32_t * actualBytesRead);

//------ARGUMENT PARSING UTILITIES --------------
//puts tokens into argV[x], returns number of tokens in argC
int tokenize(char * argString, char ** argV, const char * delimiter);

/*
* error code 1: too many invalid characters
* error code 2: extention given but no filename
* error code 3: filename longer than 8 characters
* error code 4: extension longer than 3 characters
* error code 5: cannot touch or mkdir special directories
* success code 0: filename is valid
*/

int filterFilename(const char * argument, bool isPut, bool isCdOrLs);

//this does just what it sounds like
void printFilterError(const char * commandName, const char * filename, bool isDir, int exitCode);

//------------ INITIALIZATION ------------------------------
/* loads up boot sector and initalizes file and directory tables

*/
void allocateFileTable();


int initEnvironment(const char * imageName, fat32BS * boot_sector);

uint32_t getFileSize(char * filename);

#endif
