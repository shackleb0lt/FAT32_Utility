
#include "fat32.h"

//Clusters  
uint32_t FAT_FREE_CLUSTER = 0x00000000;
uint32_t FAT_EOC = 0x0FFFFFF8;
uint32_t MASK_IGNORE_MSB = 0x0FFFFFFF;

int checkForFileError(int fd)
{
    if(fd == -1) {
        printf("FATAL ERROR: Problem openning image -- EXITTING!\n");
        exit(EXIT_FAILURE);
    }
    return 1;
} 

//-------------END BOOT SECTOR FUNCTIONS -----------------------
/* notes: the alias shown above the functions match up the variables from
 * the Microsoft FAT specificatiions
 */
  
//alias := rootDirSectors
int rootDirSectorCount(fat32BS * bs)
{
	return (bs->BPB_RootEntCnt * 32) + (bs->BPB_BytesPerSec - 1) / bs->BPB_BytesPerSec ;	
}
//alias := FirstDataSector
int firstDataSector(fat32BS * bs) 
{
	int FATSz;
	if(bs->BPB_FATSz16 != 0)
		FATSz = bs->BPB_FATSz16;
	else
		FATSz = bs->BPB_FATSz32;
	return bs->BPB_RsvdSecCnt + (bs->BPB_NumFATs * FATSz) + rootDirSectorCount(bs);
}
//alias := FirstSectorofCluster
uint32_t firstSectorofCluster(fat32BS * bs, uint32_t clusterNum)
{
	return (clusterNum - 2) * (bs->BPB_SecPerClus) + firstDataSector(bs);
}

/* decription: feed it a cluster and it returns the byte offset
 * of the beginning of that cluster's data
 * */
uint32_t byteOffsetOfCluster(fat32BS *bs, uint32_t clusterNum)
{
    return firstSectorofCluster(bs, clusterNum) * bs->BPB_BytesPerSec; 
}

//alias := DataSec
int sectorsInDataRegion(fat32BS * bs)
{
	int FATSz;
	int TotSec;
	if(bs->BPB_FATSz16 != 0)
		FATSz = bs->BPB_FATSz16;
	else
		FATSz = bs->BPB_FATSz32;
	if(bs->BPB_TotSec16 != 0)
		TotSec = bs->BPB_TotSec16;
	else
		TotSec = bs->BPB_TotSec32;
	return TotSec - (bs->BPB_RsvdSecCnt + (bs->BPB_NumFATs * FATSz) + rootDirSectorCount(bs));

}

//alias := CountofClusters
int countOfClusters(fat32BS * bs) 
{
	return sectorsInDataRegion(bs) / bs->BPB_SecPerClus;
}

int getFatType(fat32BS * bs)
{
	int clusterCount =  countOfClusters(bs);
	if(clusterCount < 4085)
		return FAT12;
	else if(clusterCount < 65525)	
		return FAT16;
	else
		return FAT32;
}

int firstFatTableSector(fat32BS *bs)
{
    return Partition_LBA_Begin + bs->BPB_RsvdSecCnt;
}
//the sector of the first clustor
//cluster_begin_lba
int firstClusterSector(fat32BS *bs)
{
    return Partition_LBA_Begin + bs->BPB_RsvdSecCnt + (bs->BPB_NumFATs * bs->BPB_SecPerClus);
}

 
//----------------OPEN FILE/DIRECTORY TABLES-----------------
/* create a new file to be put in the file descriptor table
 * */
FILEDESCRIPTOR * TBL_createFile(const char * filename, const char * extention, const char * parent, 
                                uint32_t firstCluster, int mode, uint32_t size, int dir,int isOpen) 
{
    FILEDESCRIPTOR * newFile = (FILEDESCRIPTOR *) malloc(sizeof(FILEDESCRIPTOR));
    strcpy(newFile->filename, filename);
    strcpy(newFile->extention, extention);
    strcpy(newFile->parent, parent);
    
    if(strlen(newFile->extention) > 0) {
        strcpy(newFile->fullFilename, newFile->filename);
        strcat(newFile->fullFilename, ".");
        strcat(newFile->fullFilename, newFile->extention);
    }
    else
    {
        strcpy(newFile->fullFilename, newFile->filename);
    }
    
    newFile->firstCluster = firstCluster;
    newFile->mode = mode;
    newFile->size = size;
    newFile->dir = dir;
	newFile->isOpen = isOpen;
    return newFile;
}
/* adds a file object to either the fd table or the dir history table
 */ 
int TBL_addFileToTbl(FILEDESCRIPTOR * file, int isDir)
{
    if(isDir == true) {
        if(environment.tbl_dirStatsSize < TBL_DIR_STATS) { 
            environment.dirStatsTbl[environment.tbl_dirStatsIndex % TBL_DIR_STATS] = *file;
            environment.tbl_dirStatsSize++;
			environment.tbl_dirStatsIndex = (environment.tbl_dirStatsIndex+1) % TBL_DIR_STATS;
			return 0;
        }
        else {
			environment.dirStatsTbl[environment.tbl_dirStatsIndex % TBL_DIR_STATS] = *file;
			environment.tbl_dirStatsIndex = (environment.tbl_dirStatsIndex+1) % TBL_DIR_STATS;
			return 0;
		}		
        
        
    } else {
        if(environment.tbl_openFilesSize < TBL_OPEN_FILES) {
            environment.openFilesTbl[environment.tbl_openFilesIndex % TBL_OPEN_FILES] = *file;
            environment.tbl_openFilesSize++;
			environment.tbl_openFilesIndex = (environment.tbl_openFilesIndex+1) % TBL_OPEN_FILES;
			return 0;
        }
        else {
            environment.openFilesTbl[environment.tbl_openFilesIndex % TBL_OPEN_FILES] = *file;
			environment.tbl_openFilesIndex = (environment.tbl_openFilesIndex+1) % TBL_OPEN_FILES;
			return 0;
        }
        
    }
}

/* description: if <isDir> is set prints the content of the open files table
 * if not prints the open directory table
 */ 
int TBL_printFileTbl(int isDir)
{
	if(isDir == true) {
		puts("\nDirectory Table\n");
		if(environment.tbl_dirStatsSize > 0) {
			int x;
			for(x = 0; x < environment.tbl_dirStatsSize; x++) {
				
				printf("[%d] filename: %s, parent: %s, isOpen: %d, Size: %d, mode: %d\n", 
					x,environment.dirStatsTbl[x % TBL_DIR_STATS].fullFilename,
					environment.dirStatsTbl[x % TBL_DIR_STATS].parent,
					environment.dirStatsTbl[x % TBL_DIR_STATS].isOpen,
					environment.dirStatsTbl[x % TBL_DIR_STATS].size,
					environment.dirStatsTbl[x % TBL_DIR_STATS].mode);
			}
			return 0;
		} 
        else return 1;

	} 
    else {
		puts("\nOpen File Table\n");
		if(environment.tbl_openFilesSize > 0) {
			int x;
			for(x = 0; x < environment.tbl_openFilesSize; x++) {
			
				printf("[%d] filename: %s, parent:%s, isOpen: %d, Size: %d, mode: %d\n", 
					x,environment.openFilesTbl[x % TBL_OPEN_FILES].fullFilename,
					environment.openFilesTbl[x % TBL_OPEN_FILES].parent,
					environment.openFilesTbl[x % TBL_OPEN_FILES].isOpen,
					environment.openFilesTbl[x % TBL_OPEN_FILES].size,
					environment.openFilesTbl[x % TBL_OPEN_FILES].mode);
					
			} 
			return 0;
		}
        else return 1;
	}
}

/* desctipion searches the open directory table for entry if it finds it,
 * it returns the name of the parent directory, else it returns an empty string
*/
const char * TBL_getParent(const char * dirName) {
	if(environment.tbl_dirStatsSize > 0) {
        int x;
        for(x = 0; x < environment.tbl_dirStatsSize; x++) {
            if(strcmp(environment.dirStatsTbl[x % TBL_DIR_STATS].filename, dirName) == 0) return environment.dirStatsTbl[x % TBL_DIR_STATS].parent;
        }
        return "";
	}
    else
    {
        return "";
    }
}

/* description:  "index" is set to the index where the found element resides
 * returns true if file was found in the file table
 * 
 */ 
bool TBL_getFileDescriptor(int * index, const char * filename, bool isDir){
    // FILEDESCRIPTOR  * b;
    if(isDir == true) {
        if(environment.tbl_dirStatsSize > 0) {

			int x;
			for(x = 0; x < environment.tbl_dirStatsSize; x++) {
                if(strcmp(environment.dirStatsTbl[x % TBL_DIR_STATS].fullFilename, filename) == 0) {
                    *index = x;
                    return true;
                }
            }
            
        } 
        else return false;
    }
    else {

        if(environment.tbl_openFilesSize > 0) {

			int x;
			for(x = 0; x < environment.tbl_openFilesSize; x++) {
              
					if(strcmp(environment.openFilesTbl[x % TBL_OPEN_FILES].fullFilename, filename) == 0) {
                        *index = x;
                        return true;
                    }
                }
            
        }
         else return false;
    }

    return false;
}
//-----------------------------------------------------------------------------
/*description: feed this a cluster and it returns an offset, in bytes, where the
 * fat entry for this cluster is locted
 * 
 * use: use the result to seek to the place in the image and read
 * the FAT entry
* */
uint32_t getFatAddressByCluster(fat32BS * bs, uint32_t clusterNum) {
    uint32_t FATOffset = clusterNum * 4;
    uint32_t ThisFATSecNum = bs->BPB_RsvdSecCnt + (FATOffset / bs->BPB_BytesPerSec);
    uint32_t ThisFATEntOffset = FATOffset % bs->BPB_BytesPerSec;
    return (ThisFATSecNum * bs->BPB_BytesPerSec + ThisFATEntOffset); 
}

/*description: finds the location of this clusters FAT entry, reads
 * and returns the entry value
 * 
 * use: use the result to seek to the place in the image and read
 * the FAT entry
* */
uint32_t FAT_getFatEntry(fat32BS * bs, uint32_t clusterNum) {
    
    int fd = open(environment.imageName, O_RDWR);
    checkForFileError(fd);
    uint8_t aFatEntry[FAT_ENTRY_SIZE];
    uint32_t FATOffset = clusterNum * 4;

    lseek(fd, getFatAddressByCluster(bs, clusterNum), SEEK_SET);
    read(fd,aFatEntry, sizeof(aFatEntry));
    close(fd);
    
    uint32_t fatEntry = 0x00000000;

    int x;
    for(x = 0; x < 4; x++) {
        fatEntry |= aFatEntry[(FATOffset % FAT_ENTRY_SIZE ) + x] << 8 * x;
    }

    return fatEntry;
}
/* description: takes a value, newFatVal, and writes it to the destination
 * cluster, 
 */ 
int FAT_writeFatEntry(fat32BS * bs, uint32_t destinationCluster, uint32_t * newFatVal) {
    int fd = open(environment.imageName, O_RDWR);
    checkForFileError(fd);
    lseek(fd, getFatAddressByCluster(bs, destinationCluster), SEEK_SET);
    write(fd,newFatVal,4);
    close(fd);
    return 0;
}


/*description:walks a FAT chain until 0x0fffffff, returns the number of iterations
 * which is the size of the cluster chiain
*/ 
int getFileSizeInClusters(fat32BS * bs, uint32_t firstClusterVal) {
    int size = 1;
    firstClusterVal = (int) FAT_getFatEntry(bs, firstClusterVal);

    while((firstClusterVal = (firstClusterVal & MASK_IGNORE_MSB)) < FAT_EOC) {
       
        size++;
        firstClusterVal = FAT_getFatEntry(bs, firstClusterVal);
    }
    return size;
        
}
/*description: walks a FAT chain until returns the last cluster, where
* the current EOC is. returns the cluster number passed in if it's empty,
* , you should probly call getFirstFreeCluster if you intend to write to
* the FAT to keep things orderly
*/ 
uint32_t getLastClusterInChain(fat32BS * bs, uint32_t firstClusterVal) {
    
    uint32_t lastCluster = firstClusterVal;
    firstClusterVal = (int) FAT_getFatEntry(bs, firstClusterVal);
    //if cluster is empty return cluster number passed in
    if((((firstClusterVal & MASK_IGNORE_MSB) | FAT_FREE_CLUSTER) == FAT_FREE_CLUSTER) )
        return lastCluster;
    //mask the 1st 4 msb, they are special and don't count    
    while((firstClusterVal = (firstClusterVal & MASK_IGNORE_MSB)) < FAT_EOC) {
        lastCluster = firstClusterVal;
        firstClusterVal = FAT_getFatEntry(bs, firstClusterVal);
    }
    return lastCluster;
        
}
/* description: traverses the FAT sequentially and find the first open cluster
 */ 
int FAT_findFirstFreeCluster(fat32BS * bs) {
    int i = 0;
    int totalClusters = countOfClusters(bs);
    while(i < totalClusters) {
        if(((FAT_getFatEntry(bs, i) & MASK_IGNORE_MSB) | FAT_FREE_CLUSTER) == FAT_FREE_CLUSTER)
            break;
        i++;
    }
    return i;
}
/* decription: returns the total number of open clusters
 */ 
int FAT_findTotalFreeCluster(fat32BS * bs) {
    int i = 0;
    int fatIndex = 0;
    int totalClusters = countOfClusters(bs);
    while(fatIndex < totalClusters) {
        if((((FAT_getFatEntry(bs, fatIndex) & MASK_IGNORE_MSB) | FAT_FREE_CLUSTER) == FAT_FREE_CLUSTER))
            i++;
        fatIndex++;
    }
    return i;
}



/* description: pass in a entry and this properly formats the
 * "firstCluster" from the 2 byte segments in the file structure
 */
uint32_t buildClusterAddress(DIR_ENTRY * entry) {
    uint32_t firstCluster = 0x00000000;
    firstCluster |=  entry->DIR_FstClusHI[1] << 24;
    firstCluster |=  entry->DIR_FstClusHI[0] << 16;
    firstCluster |=  entry->DIR_FstClusLO[1] << 8;
    firstCluster |=  entry->DIR_FstClusLO[0];
    return firstCluster;
}

/* description: convert uint32_t to DIR_FstClusHI and DIR_FstClusLO byte array
 * and stores it into <entry>
 */ 
int deconstructClusterAddress(DIR_ENTRY * entry, uint32_t cluster) {
    entry->DIR_FstClusLO[0] = cluster;
    entry->DIR_FstClusLO[1] = cluster >> 8;
    entry->DIR_FstClusHI[0] = cluster >> 16;
    entry->DIR_FstClusHI[1] = cluster >> 24;
    return 0;
}
/* description: takes a directory entry populates a file descriptor 
 * to be used in the file tables
 * */
FILEDESCRIPTOR * makeFileDecriptor(DIR_ENTRY * entry, FILEDESCRIPTOR * fd) {
    char newFilename[12];
    bzero(fd->filename, 9);
    bzero(fd->extention, 4);
    memcpy(newFilename, entry->filename, 11);
    newFilename[11] = '\0';
    int x;
    for(x = 0; x < 8; x++) {
        if(newFilename[x] == ' ')
            break;
        fd->filename[x] = newFilename[x];
    }
    fd->filename[++x] = '\0';
    for(x = 8; x < 11; x++) {
        if(newFilename[x] == ' ')
            break;
        fd->extention[x - 8] = newFilename[x];
    }
    fd->extention[++x - 8] = '\0';
    if(strlen(fd->extention) > 0) {
        strcpy(fd->fullFilename, fd->filename);
        strcat(fd->fullFilename, ".");
        strcat(fd->fullFilename, fd->extention);
    }
    else {
        strcpy(fd->fullFilename, fd->filename);
    }
    fd->firstCluster = buildClusterAddress(entry);
	fd->size = entry->DIR_FileSize;
	fd->mode = MODE_UNKNOWN;
    if((entry->DIR_Attr & ATTR_DIRECTORY) == ATTR_DIRECTORY)
        fd->dir = true;
    else
        fd->dir = false;
    return fd;
}



/* description: takes a cluster number where bunch of directories are and
 * the offst of the directory you want read and it will store that directory
 * info into the variable dir
 */ 
DIR_ENTRY * readEntry(fat32BS * bs, DIR_ENTRY * entry, uint32_t clusterNum, int offset){
    offset *= 32;
    uint32_t dataAddress = byteOffsetOfCluster(bs, clusterNum);
    
    int fd = open(environment.imageName, O_RDWR);
    checkForFileError(fd);
    lseek(fd, dataAddress + offset, SEEK_SET);
    read(fd, entry,sizeof(DIR_ENTRY));    
    close(fd);
    return entry;
}
/* description: takes a cluster number and offset where you want an entry written and write to that 
 * position
 */ 
DIR_ENTRY * writeEntry(fat32BS * bs, DIR_ENTRY * entry, uint32_t clusterNum, int offset){
    offset *= 32;
    uint32_t dataAddress = byteOffsetOfCluster(bs, clusterNum);
    int fd = open(environment.imageName, O_RDWR);
    checkForFileError(fd);

    lseek(fd, dataAddress + offset, SEEK_SET);
    write(fd,entry,sizeof(DIR_ENTRY));
    
    close(fd);
    return entry;
}
/* description: takes a directory cluster and give you the byte address of the entry at
 * the offset provided. used to write dot entries. POSSIBLY REDUNDANT
 */ 
uint32_t byteOffsetofDirectoryEntry(fat32BS * bs, uint32_t clusterNum, int offset) {
    offset *= 32;
    uint32_t dataAddress = byteOffsetOfCluster(bs, clusterNum);
    return (dataAddress + offset);
}

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
          bool goingUp, FILEDESCRIPTOR * searchFile, bool useAsSearch, bool searchForDir)
 {

    DIR_ENTRY dir;
	int dirSizeInCluster = getFileSizeInClusters(bs, clusterNum);
    int clusterCount;
    int offset;
    int increment;
    
    for(clusterCount = 0; clusterCount < dirSizeInCluster; clusterCount++) {
        //root doesn't have . and .., so we traverse it differently
        // we only increment 1 to get . and .. then we go in 2's to avoid longname entries
        if(strcmp(directoryName, ROOT) == 0) {
            offset = 1;
            increment = 2;
        }
        else {
            offset = 0;
            increment = 1;
        }
		
        for(; offset < ENTRIES_PER_SECTOR; offset += increment) {
            //this is a hack to properly display the dir contents of folders
            //with . and .. w/o displaying longname entries
            //if not root folder we need to read the 1st 2 entries (. and ..)
            //sequential then resume skipping longnames by incrementing by 2
            if(strcmp(directoryName, ROOT) != 0 && offset == 2) {
                increment = 2;
                offset -= 1;			
                continue;
            }

			
            readEntry(bs, &dir, clusterNum, offset+1);
            makeFileDecriptor(&dir, searchFile);
            
            if( cd == false ) {
                //we don't want to print 0xE5 to the screen
                if(searchFile->filename[0] != 0xE5) {
                    if(searchFile->dir == true) 
                        printf("<%s>\t %d\n", searchFile->fullFilename,searchFile->size);
                    else
                        printf("%s\t %d\n", searchFile->fullFilename,searchFile->size);
                }
            }
            else
            {
                
                //if there's an extention we expect the user must type it in with the '.' and the extention
                //because of this we must compare it to a recontructed filename with the implied '.' put
                //back in. also searchForDir tells us whether the file being searched for is a directory or not
                if(useAsSearch == true) {
                     if(strcmp(searchFile->fullFilename, directoryName) == 0 && searchFile->dir == searchForDir ) 
                        return true;
                }
                else {   
                
                    /* if we're going down we search pwd for directory with the name passed in.
                    * if we find it we add it to the directory table and return true. else
                    * we fall out of this loop and return false at the end of the function
                    */
                    if(searchFile->dir == true && strcmp(searchFile->fullFilename, directoryName) == 0 && goingUp == false) {// <--------CHANGED
                        environment.pwd_cluster = searchFile->firstCluster;
                        if(strcmp(TBL_getParent(directoryName), "") == 0)//if directory not already in open dir history table
                            TBL_addFileToTbl(TBL_createFile(searchFile->filename, "", environment.pwd, searchFile->firstCluster, searchFile->mode, searchFile->size, true, false), true);

                        FAT_setIoWriteCluster(bs, environment.pwd_cluster);
                        return true;
                    }
                    
                    //fetch parent cluster from the directory table
                    if(searchFile->dir == true && strcmp(directoryName, PARENT) == 0 && goingUp == true) {
                        const char * parent = TBL_getParent(environment.pwd);
                        if(strcmp(parent, "") != 0) { //we found parent in the table
                        
                            if(strcmp(parent, "/") == 0) 
                                environment.pwd_cluster = 2; //sets pwd_cluster to where we're headed
                            else 
                                environment.pwd_cluster = searchFile->firstCluster; 
                            
                            strcpy(environment.last_pwd, environment.pwd);
                            strcpy(environment.pwd, TBL_getParent(environment.pwd));  
                            FAT_setIoWriteCluster(bs, environment.pwd_cluster);
                            
                            return true; //cd to parent was successful
                        } else 
                            return false; //cd to parent not successful
                    }
                    
                } // end cd block

            }
            
        }
        
        clusterNum = FAT_getFatEntry(bs, clusterNum);
    
    
    }
    return false;
}




/* descriptioin: takes a FILEDESCRIPTOR and checks if the entry it was
 * created from is empty. Helper function
 */ 
bool isEntryEmpty(FILEDESCRIPTOR * fd) {
    if((fd->filename[0] != 0x00) && (fd->filename[0] != 0xE5) )
        return false;
    else
        return true;
}

/* description: walks a directory cluster chain looking for empty entries, if it finds one
 * it returns the byte offset of that entry. If it finds none then it returns -1;
 */ 
uint32_t FAT_findNextOpenEntry(fat32BS * bs, uint32_t pwdCluster) {

    DIR_ENTRY dir;
    FILEDESCRIPTOR fd;
    int dirSizeInCluster = getFileSizeInClusters(bs, pwdCluster);
    int clusterCount;
    
    int offset;
    int increment;
    
    for(clusterCount = 0; clusterCount < dirSizeInCluster; clusterCount++) {
        if(strcmp(environment.pwd, ROOT) == 0) {
            offset = 1;
            increment = 2;
        } else {
            offset = 0;
            increment = 1;
        }
		
        for(; offset < ENTRIES_PER_SECTOR; offset += increment) {
            if(strcmp(environment.pwd, ROOT) != 0 && offset == 2) {
                increment = 2;
                offset -= 1;	
                continue;
            }

            readEntry(bs, &dir, pwdCluster, offset);
            makeFileDecriptor(&dir, &fd);

            if( isEntryEmpty(&fd) == true ) {
                //this should tell me exactly where to write my new entry to
                //printf("cluster #%d, byte offset: %d: ", offset, byteOffsetofDirectoryEntry(bs, pwdCluster, offset));             
                return byteOffsetofDirectoryEntry(bs, pwdCluster, offset);
            }
        }
        //pwdCluster becomes the next cluster in the chain starting at the passed in pwdCluster
       pwdCluster = FAT_getFatEntry(bs, pwdCluster); 
      
    }
    return -1; //cluster chain is full
}

/* description: wrapper for doDir. tries to change the pwd to <directoryName> 
 * This doesn't use the print to screen functinoality of doDir. if <goingUp> 
 * is set it uses the open directory table to find the pwd's parent
 */ 
int doCD(fat32BS * bs, const char * directoryName, int goingUp,  FILEDESCRIPTOR * searchFile) {
   
    return doDir(bs, environment.pwd_cluster, true, directoryName, goingUp, searchFile, false, false); 
}

/* desctipion: wrapper that uses doDir to search a pwd for a file and puts its info
 * into searchFile if found. if <useAsFileSearch> is set it disables the printing enabling
 * it to be used as a file search utility. if <searchForDirectory> is set it excludes
 * files from the search
 */ 
bool searchOrPrintFileSize(fat32BS * bs, const char * fileName, bool useAsFileSearch, bool searchForDirectory, FILEDESCRIPTOR * searchFile) {
    
    if(doDir(bs, environment.pwd_cluster, true, fileName, false, searchFile, true, searchForDirectory) == true) {
        if(useAsFileSearch == false)
            printf("File: %s is %d byte(s) in size", fileName ,searchFile->size);
        return true;
    } else {
        if(useAsFileSearch == false)
            printf("ERROR: File: %s was not found", fileName);
        return false;
    }
}

/* desctipion: wrapper that uses doDir to search a pwd for a file and puts its info
 * into searchFile if found. Prints result for both file and directories
 */ 
bool printFileOrDirectorySize(fat32BS * bs, const char * fileName, FILEDESCRIPTOR * searchFile) {
    
    if(doDir(bs, environment.pwd_cluster, true, fileName, false, searchFile, true, true) == true ||
    doDir(bs, environment.pwd_cluster, true, fileName, false, searchFile, true, false) == true) {
        printf("File: %s is %d byte(s) in size", fileName ,searchFile->size);
        return true;
    } else 
        return false;
    
}

/* description: wrapper for searchOrPrintFileSize searchs pwd for filename. 
 * returns true if found, if found the resulting entry is put into searchFile
 * for use outside the function. if <searchForDirectory> is set it excludes
 * files from the search
 */ 
bool searchForFile(fat32BS * bs, const char * fileName, bool searchForDirectory, FILEDESCRIPTOR * searchFile) {
    return searchOrPrintFileSize(bs, fileName, true, searchForDirectory, searchFile);
} 

/* decription: feed this a cluster number that's part of a chain and this
 * traverses it to find the last entry as well as finding the first available
 * free cluster and adds that free cluster to the chain
 */ 
uint32_t FAT_extendClusterChain(fat32BS * bs,  uint32_t clusterChainMember) {
    uint32_t firstFreeCluster = FAT_findFirstFreeCluster(bs);
	uint32_t lastClusterinChain = getLastClusterInChain(bs, clusterChainMember);

    FAT_writeFatEntry(bs, lastClusterinChain, &firstFreeCluster);
    FAT_writeFatEntry(bs, firstFreeCluster, &FAT_EOC);
    return firstFreeCluster;
}

/* desctipion: pass in a free cluster and this marks it with EOC and sets 
 * the environment.io_writeCluster to the end of newly created cluster 
 * chain
*/
int FAT_allocateClusterChain(fat32BS * bs,  uint32_t clusterNum) {
	FAT_writeFatEntry(bs, clusterNum, &FAT_EOC);
	return 0;
}

/* desctipion: pass in the 1st cluster of a chain and this traverses the 
 * chain and writes FAT_FREE_CLUSTER to every link, thereby freeing the 
 * chain for reallocation
*/
int FAT_freeClusterChain(fat32BS * bs,  uint32_t firstClusterOfChain){
    int dirSizeInCluster = getFileSizeInClusters(bs, firstClusterOfChain);
    int currentCluster= firstClusterOfChain;
    int nextCluster;
    int clusterCount;
    
    for(clusterCount = 0; clusterCount < dirSizeInCluster; clusterCount++) {  
       nextCluster = FAT_getFatEntry(bs, currentCluster); 
       FAT_writeFatEntry(bs, currentCluster, &FAT_FREE_CLUSTER);
       currentCluster = nextCluster;
    }
    return 0; 
    
}

/* description: feed this a cluster that's a member of a chain and this
 * sets your environment variable "io_writeCluster" to the last in that
 * chain. The thinking is this is where you'll write in order to grow
 * that file. before writing to cluster check with FAT_findNextOpenEntry()
 * to see if there's space or you need to extend the chain using FAT_extendClusterChain()
 */ 
int FAT_setIoWriteCluster(fat32BS * bs, uint32_t clusterChainMember) {
     environment.io_writeCluster = getLastClusterInChain(bs, clusterChainMember);
	 return 0;
}

/* description: this will write a new entry to <destinationCluster>. if that cluster
 * is full it grows the cluster chain and write the entry in the first spot
 * of the new cluster. if <isDotEntries> is set this automatically writes both
 * '.' and '..' to offsets 0 and 1 of <destinationCluster>
 * 
 */ 
int writeFileEntry(fat32BS * bs, DIR_ENTRY * entry, uint32_t destinationCluster, bool isDotEntries) {
    int dataAddress;
    int freshCluster;
    int fd = open(environment.imageName, O_RDWR);
    checkForFileError(fd);
    if(isDotEntries == false) {
        if((dataAddress = FAT_findNextOpenEntry(bs, destinationCluster)) != -1) {//-1 means current cluster is at capacity
            lseek(fd, dataAddress, SEEK_SET);
            write(fd,entry,sizeof(DIR_ENTRY));
        }
        else {
            freshCluster = FAT_extendClusterChain(bs, destinationCluster);
            dataAddress = FAT_findNextOpenEntry(bs, freshCluster);
            lseek(fd, dataAddress, SEEK_SET);
            write(fd,entry,sizeof(DIR_ENTRY));
        }
    }
    else {
        DIR_ENTRY dotEntry;
        DIR_ENTRY dotDotEntry;
        makeSpecialDirEntries(&dotEntry, &dotDotEntry, destinationCluster, environment.pwd_cluster);
        //seek to first spot in new dir cluster chin and write the '.' entry
        dataAddress = byteOffsetofDirectoryEntry(bs, destinationCluster, 0);
        lseek(fd, dataAddress, SEEK_SET);
        
        write(fd,&dotEntry,sizeof(DIR_ENTRY));
        //seek to second spot in new dir cluster chin and write the '..' entry
        dataAddress = byteOffsetofDirectoryEntry(bs, destinationCluster, 1);
        lseek(fd,dataAddress, SEEK_SET);
        
        write(fd,&dotDotEntry,sizeof(DIR_ENTRY));
    }
     close(fd);
     return 0;
}



/* description: takes a directory entry and all the necesary info
	and populates the entry with the info in a correct format for
	insertion into a disk.
*/
int createEntry(DIR_ENTRY * entry, const char * filename, const char * ext,
			int isDir, uint32_t firstCluster, uint32_t filesize,
            bool emptyAfterThis, bool emptyDirectory) {
    //set the same no matter the entry
    entry->DIR_NTRes = 0;
	entry->DIR_CrtTimeTenth = 0;
	entry->DIR_CrtTime = 0;
	entry->DIR_CrtDate = 0;
	entry->DIR_LstAccDate = 0;
	entry->DIR_WrtTime = 0;
	entry->DIR_WrtDate = 0;

    if(emptyAfterThis == false && emptyDirectory == false) { //if both are false
        int x;
        for(x = 0; x < MAX_FILENAME_SIZE; x++) {
            if(x < strlen(filename))
                entry->filename[x] = filename[x];
            else
                entry->filename[x] = ' ';
        }

        for(x = 0; x < MAX_EXTENTION_SIZE; x++) {
            if(x < strlen(ext))
                entry->filename[MAX_FILENAME_SIZE + x] = ext[x];
            else
                entry->filename[MAX_FILENAME_SIZE + x] = ' ';
        }
        
        deconstructClusterAddress(entry, firstCluster);

        if(isDir == true) {
            entry->DIR_FileSize = 0;
            entry->DIR_Attr = ATTR_DIRECTORY;
		}
        else {
            entry->DIR_FileSize = filesize;
            entry->DIR_Attr = ATTR_ARCHIVE;
		}
        return 0; //stops execution so we don't flow out into empty entry config code below
        
    }
    else if(emptyAfterThis == true) { //if this isn't true, then the other must be
        entry->filename[0] = 0xE5;
        entry->DIR_Attr = 0x00;
    }
    else {                             //hence no condition check here
        entry->filename[0] = 0x00;
        entry->DIR_Attr = 0x00;
    }
    
    //if i made it here we're creating an empty entry and both conditions
    //require this setup
    int x;

    for(x = 1; x < 11; x++) entry->filename[x] = 0x00;
    
	entry->DIR_FstClusLO[0] = 0x00;
    entry->DIR_FstClusLO[1] = 0x00;
    entry->DIR_FstClusHI[0] = 0x00;
    entry->DIR_FstClusHI[1] = 0x00;
	entry->DIR_Attr = 0x00;
	entry->DIR_FileSize = 0;
	
    return 0;
}

/* description: take two entries and cluster info and populates	them with 
 * '.' and '..' entry information. The entries are	ready for writing to 
 * the disk after this. helper function for mkdir()
  */
int makeSpecialDirEntries(DIR_ENTRY * dot, DIR_ENTRY * dotDot,
						uint32_t newlyAllocatedCluster, uint32_t pwdCluster ) 
{
	createEntry(dot, ".", "", true, newlyAllocatedCluster, 0, false, false);	
	createEntry(dotDot, "..", "", true, pwdCluster, 0, false, false);	
	return 0;
}

/* returns offset of the entry, <searchName> in the pwd 
 * if found return the offset, if not return -1 
 */ 
int getEntryOffset(fat32BS * bs, const char * searchName) {
    DIR_ENTRY entry;
    FILEDESCRIPTOR fd;
    uint32_t currentCluster = environment.pwd_cluster;
    int dirSizeInCluster = getFileSizeInClusters(bs, currentCluster);
    int clusterCount;
    int offset;
    int increment;

    for(clusterCount = 0; clusterCount < dirSizeInCluster; clusterCount++) {
        if(strcmp(environment.pwd, ROOT) == 0) {
            offset = 1;
            increment = 2;
        } 
        else {
            offset = 0;
            increment = 1;
        }
		
        for(; offset < ENTRIES_PER_SECTOR; offset += increment) {
            if(strcmp(environment.pwd, ROOT) != 0 && offset == 2) {
                increment = 2;
                offset -= 1;	
                continue;
            }
			
            readEntry(bs, &entry, currentCluster, offset);
            makeFileDecriptor(&entry, &fd);

            if(strcmp(searchName, fd.fullFilename) == 0) {
            
                return offset;
            }
                
       
      
        }
        currentCluster = FAT_getFatEntry(bs, currentCluster); 
   
    }
     return -1;
}
/* description: prints info about the image to the screen
 */ 
int printInfo(fat32BS * bs) {

    printf("---- Device Info ----\n");
	printf("OEM Name: %s\n",bs->BS_OEMName);
	printf("Label: %.*s\n",BS_VolLab_LENGTH,bs->BS_VolLab);

	printf("File System Type: %.*s\n",BS_FilSysType_LENGTH,bs->BS_FilSysType);

	if(bs->BPB_Media == 0xf8)printf("Media Type: 0x%X (fixed)\n",bs->BPB_Media);
	else if(bs->BPB_Media == 0xf0)printf("Media Type: 0x%X (removable)\n",bs->BPB_Media);

	long long int disk_size = bs->BPB_BytesPerSec * bs->BPB_FATSz32/4;
    disk_size *= bs->BPB_BytesPerSec * bs->BPB_SecPerClus/1000;
	printf("Size: %lld bytes( %lldMB, %.3lfGB)\n",disk_size,disk_size>>10,((double)disk_size)/(1<<20));
	if(bs->BS_DrvNum==128)printf("Drive Number: %d (hard disk)\n\n",bs->BS_DrvNum);
	else printf("Drive Number: %d (A drive disk)\n\n",bs->BS_DrvNum);

	printf("--- Geometry ---\n");
	printf("Bytes per Sector: %d\n",bs->BPB_BytesPerSec);
	printf("Sectors per Cluster: %d\n",bs->BPB_SecPerClus);
	printf("Geom: Sectors per Track: %d\n",bs->BPB_SecPerTrk);
	printf("Geom: Heads: %d\n",bs->BPB_NumHeads);
	printf("Hidden Sectors: %d\n\n",bs->BPB_HiddSec);

	printf("--- FS Info ---\n");
	printf("Volume ID: %d\n",bs->BS_VolID);
	printf("Version: %d:%d\n",bs->BPB_FSVerHigh,bs->BPB_FSVerLow);
	printf("Reserved Sectors: %d\n",bs->BPB_RsvdSecCnt);
	printf("Number of FATs: %d\n",bs->BPB_NumFATs);
	printf("FAT Size: %d\n",bs->BPB_FATSz32);
	
	if((bs->BPB_ExtFlags&256)!=256)printf("Mirrored FAT: 0 (yes)\n");
	else printf("Mirrored FAT: 1 (%d FAT is active)\n",(bs->BPB_ExtFlags&7));
	
	printf("Boot Sector Backup Sector No: %d\n",bs->BPB_BkBootSec);

    // puts("\nBoot Sector Info:\n");
	// printf("Bytes Per Sector (block size):%d\n", bs->BPB_BytesPerSec);
	// printf("Sectors per Cluster: %d\n", bs->BPB_SecPerClus);
    // printf("Total clusters: %d\n", countOfClusters(bs));
	// printf("Number of FATs: %d\n", bs->BPB_NumFATs);
    // printf("Sectors occupied by one FAT: %d\nComputing Free Sectors.......\n", bs->BPB_FATSz32);
    // printf("Number of free sectors: %d\n", FAT_findTotalFreeCluster(bs));
   
    
    return 0;
}
/* description: DOES NOT allocate a cluster chain. Writes a directory 
 * entry to <targetDirectoryCluster>. Error handling is done in the driver
 */ 
int touch(fat32BS * bs, const char * filename, const char * extention, uint32_t targetDirectoryCluster) {
    DIR_ENTRY newFileEntry;
    createEntry(&newFileEntry, filename, extention, false, 0, 0, false, false);
    writeFileEntry(bs, &newFileEntry, targetDirectoryCluster, false);
    return 0;
}

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
int fClose(fat32BS * bs, int argC, const char * filename) {
    int fIndex;
    FILEDESCRIPTOR searchFileInflator;
    FILEDESCRIPTOR * searchFile = &searchFileInflator;
     
        if(argC != 2) {
            return 1;
        }

         if(searchForFile(bs, filename, false, searchFile) == false) {
            return 2;
        }  

        if(TBL_getFileDescriptor(&fIndex, filename, false) == false ) {
            return 3;
        }

        if(TBL_getFileDescriptor(&fIndex,filename, false) == true && environment.openFilesTbl[fIndex].isOpen == false) {
            return 4;
        }

        environment.openFilesTbl[fIndex].isOpen = false;
        return 0;
    }

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
int fOpen(fat32BS * bs, int argC, const char * filename, const char * option) {

    FILEDESCRIPTOR searchFileInflator;
    FILEDESCRIPTOR * searchFile = &searchFileInflator;
    int fileMode;
    int fileTblIndex;
    
    if(argC != 2) {
        return 1;
    }
      
    if(searchForFile(bs, filename, true, searchFile) == true) {
        return 2;
    } 
    
    if(searchForFile(bs, filename, false, searchFile) == false) {
        return 3;
    }  
    
    if(TBL_getFileDescriptor(&fileTblIndex, filename, false) == true && environment.openFilesTbl[fileTblIndex].isOpen == true) {
        return 4;
    } 

    if(strcmp(option, "w") == 0) {
        fileMode = MODE_WRITE;
 
    }
    else if(strcmp(option, "r") == 0) {
        fileMode = MODE_READ;

    }
    else if(strcmp(option, "x") == 0) {
        fileMode = MODE_BOTH;
    }
    else {
        return 5;
    }
    
    if(TBL_getFileDescriptor(&fileTblIndex, filename, false) == false ) {
        TBL_addFileToTbl(TBL_createFile(searchFile->filename, searchFile->extention, environment.pwd, searchFile->firstCluster, fileMode, searchFile->size, false, true), false);
    }
    else {
        //update  entry if file is reopened
        environment.openFilesTbl[fileTblIndex].isOpen = true;
        uint32_t offset;
        DIR_ENTRY entry;
        if( ( offset = getEntryOffset(bs, filename) ) == -1)
            return 6; 
            
        readEntry(bs, &entry, environment.pwd_cluster, offset);
        environment.openFilesTbl[fileTblIndex].size = entry.DIR_FileSize;
        environment.openFilesTbl[fileTblIndex].mode = fileMode;
       
    }
 
    return 0;
}

/* description: takes a cluster number of the first cluster of a file and its size. 
 * this function will write <dataSize> bytes starting at position <pos>. 
 */ 
int FWRITE_writeData(fat32BS * bs, uint32_t firstCluster, uint32_t pos, const char * data, uint32_t dataSize){

            //determine the cluster offset of the cluster
            uint32_t currentCluster = firstCluster;
            // where pos is to start writing data
            uint32_t writeClusterOffset = (pos / bs->BPB_BytesPerSec);
            uint32_t posRelativeToCluster = pos %  bs->BPB_BytesPerSec;
            uint32_t fileSizeInClusters = getFileSizeInClusters(bs, firstCluster);
            uint32_t remainingClustersToWalk = fileSizeInClusters - writeClusterOffset;
            
            int x;
            int dataIndex;
            //seek to the proper cluster offset from first cluster
            for(x = 0; x < writeClusterOffset; x++)
                currentCluster = FAT_getFatEntry(bs, currentCluster);
                
            //if here, we just start writing at the last cluster right after
            //the last writen byte
            dataIndex = 0;
			uint32_t dataWriteLength = dataSize;
			uint32_t dataRemaining = dataSize;
			uint32_t fileWriteOffset;
			uint32_t startWritePos;
            int fd = open(environment.imageName, O_RDWR);
            checkForFileError(fd);
            for(x = 0; x < remainingClustersToWalk; x++) {
                
                startWritePos = byteOffsetOfCluster(bs, currentCluster);
                
                if(x == 0)
                    startWritePos += posRelativeToCluster;
                        
                if(dataRemaining > bs->BPB_BytesPerSec)
                    dataWriteLength = bs->BPB_BytesPerSec;
                else
                    dataWriteLength = dataRemaining;
                //write data 1 character at a time
                for(fileWriteOffset = 0; fileWriteOffset < dataWriteLength; fileWriteOffset++) {
                    lseek(fd, startWritePos + fileWriteOffset, SEEK_SET);
                    uint8_t dataChar[1] = {data[dataIndex++]};
                    write(fd,dataChar,1);
                    
                }
                
                dataRemaining -= dataWriteLength;
                if(dataRemaining == 0){
                    break;                      
                }    
                
                currentCluster = FAT_getFatEntry(bs, currentCluster); 
            }
            close(fd);
			return 0;
}
/* description: takes a cluster number of a file and its size. this function will read <dataSize>
 * bytes starting at position <pos>. It will print <dataSize> bytes to the screen until the end of
 * the file is reached, as determined by <fileSize>
 */ 
int FREAD_readData(fat32BS * bs, uint32_t firstCluster, uint32_t pos, uint32_t dataSize, uint32_t fileSize, const char *filename ){

    //determine the cluster offset of the cluster
    uint32_t currentCluster = firstCluster;
    // where pos is to start writing data
    uint32_t readClusterOffset = (pos / bs->BPB_BytesPerSec);
    uint32_t posRelativeToCluster = pos %  bs->BPB_BytesPerSec;
    uint32_t fileSizeInClusters = getFileSizeInClusters(bs, firstCluster);
    uint32_t remainingClustersToWalk = fileSizeInClusters - readClusterOffset;
    int x;
    //seek to the proper cluster offset from first cluster
    for(x = 0; x < readClusterOffset; x++)
        currentCluster = FAT_getFatEntry(bs, currentCluster);
    //if here, we just start writing at the last cluster right after
    //the last writen byte
    uint32_t dataReadLength = dataSize;
    uint32_t dataRemaining = dataSize;
    uint32_t fileReadOffset;
    uint32_t startReadPos;
    int fd = open(environment.imageName, O_RDWR);
    FILE * newfd = fopen(filename, "w");
    checkForFileError(fd);
    for(x = 0; x < remainingClustersToWalk; x++) {
        
        startReadPos = byteOffsetOfCluster(bs, currentCluster);
        
        if(x == 0)
            startReadPos += posRelativeToCluster;
                
        if(dataRemaining > bs->BPB_BytesPerSec)
            dataReadLength = bs->BPB_BytesPerSec;
        else
            dataReadLength = dataRemaining;
        //write data 1 character at a time
        for(fileReadOffset = 0; fileReadOffset < dataReadLength && (pos + fileReadOffset) < fileSize; fileReadOffset++) {
            lseek(fd, startReadPos + fileReadOffset, SEEK_SET);
            uint8_t dataChar[1];
            read(fd,dataChar,1);
            fprintf(newfd,"%c", dataChar[0]);
        }
        
        dataRemaining -= dataReadLength;
        if(dataRemaining == 0){
            break;                      
        }    
        
        currentCluster = FAT_getFatEntry(bs, currentCluster); 
    }
    fclose(newfd);
    close(fd);
    return 0;
}



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
int fWrite(fat32BS * bs, const char * filename, uint32_t pos, const char * data,uint32_t dataSize) {

    int fileTblIndex;
    FILEDESCRIPTOR searchFileInflator;
    FILEDESCRIPTOR * searchFile = &searchFileInflator;
    DIR_ENTRY entry;
    uint32_t offset; //for updating directory entry
    
    uint32_t firstCluster;
    uint32_t currentCluster;
    // uint32_t dataSize = strlen(data);
    uint32_t fileSize;
    uint32_t newSize;
    
    uint32_t totalSectors;
    uint32_t additionalsectors;
    uint32_t paddingClusterOffset;

    uint32_t remainingClustersToWalk;
    
    uint32_t paddingLength = 0;
    uint32_t paddingRemaining;    
    
    uint32_t usedBytesInLastSector = 0;
    uint32_t openBytesInLastSector = 0;
    uint32_t startWritePos;
    
    uint32_t paddingWriteLength;
        
    uint32_t fileWriteOffset;

    int x;
    //error checking
    if(searchForFile(bs, filename, true, searchFile) == true) 
        return 1; 
    if(TBL_getFileDescriptor(&fileTblIndex, filename, false) == false)
        return 2;
    if(TBL_getFileDescriptor(&fileTblIndex, filename, false) == true && environment.openFilesTbl[fileTblIndex].isOpen == false)
        return 3;
    if(dataSize < 1)
        return 4;
    if(environment.openFilesTbl[fileTblIndex].mode != MODE_WRITE && environment.openFilesTbl[fileTblIndex].mode != MODE_BOTH) 
        return 5;
    if(searchForFile(bs, filename, false, searchFile) == false) 
        return 6;
    
    int fd = open(environment.imageName, O_RDWR);
    checkForFileError(fd);
    //if file was touched, no cluster chain was allocated, do it here
    if(searchFile->firstCluster == 0) {
        if( ( offset = getEntryOffset(bs, filename) ) == -1)
            return 7;
        readEntry(bs,  &entry, environment.pwd_cluster, offset);
        
        firstCluster = FAT_findFirstFreeCluster(bs);
        FAT_allocateClusterChain(bs, firstCluster);
        
        deconstructClusterAddress(&entry, firstCluster);
        environment.openFilesTbl[fileTblIndex].firstCluster = firstCluster;//set firstCluster
        environment.openFilesTbl[fileTblIndex].size = 0;                    // set size, althoug it's probaly set
        writeEntry(bs,  &entry, environment.pwd_cluster, offset);
    }
    else{
        fileSize = environment.openFilesTbl[fileTblIndex].size;
        firstCluster = currentCluster = environment.openFilesTbl[fileTblIndex].firstCluster;
    }
    //get max chain size for walking the chain
    uint32_t fileSizeInClusters = getFileSizeInClusters(bs, firstCluster);
    
    
    if((pos + dataSize) > fileSize) {
        //cluster size must grow
        newSize = pos + dataSize;
        
        totalSectors = newSize / bs->BPB_BytesPerSec;
        if(newSize > 0 && (newSize % bs->BPB_BytesPerSec != 0))
            totalSectors++;
        
        //determine newsize of chain
        additionalsectors = totalSectors - fileSizeInClusters;
        
        //grow the chain to new necessary size
        for(x = 0; x < additionalsectors; x++) {
            FAT_extendClusterChain(bs, firstCluster);
        }
        
        //reacquire new size
        fileSizeInClusters = getFileSizeInClusters(bs, firstCluster);
        
        //create padding with 0x20
        
        if(pos > fileSize)
            paddingLength = pos - fileSize;
        else
            paddingLength = 0;
            
        paddingRemaining = paddingLength;
        //determine used/open bytes in the last sector
        if(fileSize > 0) {
            if((fileSize % bs->BPB_BytesPerSec) == 0)    
                usedBytesInLastSector = bs->BPB_BytesPerSec;
            else
                usedBytesInLastSector = fileSize % bs->BPB_BytesPerSec;
        }
        
        openBytesInLastSector = bs->BPB_BytesPerSec - usedBytesInLastSector;
        if(paddingLength > 0) {
            //determine the cluster offset of the last cluster
            //to start padding 
            paddingClusterOffset = (fileSize / bs->BPB_BytesPerSec);
            
            //set remaining number of clusters to be walked til EOC
            remainingClustersToWalk = fileSizeInClusters - paddingClusterOffset;
            
            //seek to the proper cluster offset
            for(x = 0; x < paddingClusterOffset; x++)
                currentCluster = FAT_getFatEntry(bs, currentCluster);
            
            uint8_t padding[1] = {0x20};
            //determine where start writing padding
            for(x = 0; x < remainingClustersToWalk; x++) {
                if(x == 0) {
                    startWritePos = byteOffsetOfCluster(bs, currentCluster) + usedBytesInLastSector;
                    if((usedBytesInLastSector + paddingRemaining) > bs->BPB_BytesPerSec)
                        paddingWriteLength = openBytesInLastSector;
                    else
                        paddingWriteLength = paddingRemaining;
                    
                }
                else {
                    startWritePos = byteOffsetOfCluster(bs, currentCluster);
                    if(paddingRemaining > bs->BPB_BytesPerSec)
                        paddingWriteLength = bs->BPB_BytesPerSec;
                    else
                        paddingWriteLength = paddingRemaining;
                    
                    
                }

                //write padding
                for(fileWriteOffset = 0; fileWriteOffset < paddingWriteLength ; fileWriteOffset++) {
                    lseek(fd, startWritePos + fileWriteOffset, SEEK_SET);
                    // fwrite(padding, 1, 1, fd);
                    write(fd,padding,1);
                }
                
                paddingRemaining -= paddingWriteLength;
                if(paddingRemaining == 0) {
                    break;                      
                }
                currentCluster = FAT_getFatEntry(bs, currentCluster); 
            }  
        } //end padding if
        
        FWRITE_writeData(bs, firstCluster, pos, data, dataSize);

    } else {
        //cluster size will not grow
        newSize = fileSize;
        FWRITE_writeData(bs, firstCluster, pos, data, dataSize);
    }
    
    
        
    //update filesize
    
    if(fileSize != newSize) {
        if( ( offset = getEntryOffset(bs, filename) ) == -1)
            return 8;
        readEntry(bs,  &entry, environment.pwd_cluster, offset);
        entry.DIR_FileSize = newSize;
        environment.openFilesTbl[fileTblIndex].size = newSize;
        writeEntry(bs,  &entry, environment.pwd_cluster, offset);
    }
    close(fd);
    return 0;
    
}

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
int fRead(fat32BS * bs, const char * filename, const char * position, uint32_t * actualBytesRead) {
    int fileTblIndex;
    FILEDESCRIPTOR searchFileInflator;
    FILEDESCRIPTOR * searchFile = &searchFileInflator;
    
    uint32_t pos = atoi(position);
    
    uint32_t firstCluster;
    uint32_t currentCluster;
    uint32_t fileSize;


    //error checking
    if(searchForFile(bs, filename, true, searchFile) == true) 
        return 1; 
    if(TBL_getFileDescriptor(&fileTblIndex, filename, false) == false)
        return 2;
    if(TBL_getFileDescriptor(&fileTblIndex, filename, false) == true && environment.openFilesTbl[fileTblIndex].isOpen == false)
        return 3;
    if(pos > environment.openFilesTbl[fileTblIndex].size)
        return 4;
    if(environment.openFilesTbl[fileTblIndex].mode != MODE_READ && environment.openFilesTbl[fileTblIndex].mode != MODE_BOTH) 
        return 5;
    if(searchForFile(bs, filename, false, searchFile) == false) 
        return 6;
    
    int fd = open(environment.imageName, O_RDWR);
    checkForFileError(fd);
    fileSize = environment.openFilesTbl[fileTblIndex].size;
    *actualBytesRead = fileSize; //to be passed back to the caller
    firstCluster = currentCluster = environment.openFilesTbl[fileTblIndex].firstCluster;
    FREAD_readData(bs, firstCluster, pos, fileSize, fileSize, filename);

    close(fd);
    return 0;
}

//------ARGUMENT PARSING UTILITIES --------------
//puts tokens into argV[x], returns number of tokens in argC
int tokenize(char * argString, char ** argV, const char * delimiter)
{
	int index = 0;
    char * tokens =  strtok(argString, delimiter);
	while(tokens != NULL) {
		argV[index] = tokens;
		tokens = strtok(NULL, delimiter);
		index++;
	}
	
	return index;
}

/*
* error code 1: too many invalid characters
* error code 2: extention given but no filename
* error code 3: filename longer than 8 characters
* error code 4: extension longer than 3 characters
* error code 5: cannot touch or mkdir special directories
* success code 0: filename is valid
*/

int filterFilename(const char * argument, bool isPut, bool isCdOrLs){
	char filename[200];
	int invalid = 0;
	int periodCnt = 0;
	int tokenCount = 0;
	char * tokens[10];
    bool periodFlag = false;
    periodFlag &=false; //uneccesary satement, compiler returns unused variable though it's used 

    int x;

	strcpy(filename, argument);

	// '.' and '..' are not allowed with put
	if(isPut == true && (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0))
		return 5;

	//cd and ls will allow special directories
	if(isCdOrLs == true && (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0))
		return 0;

	for(x = 0; x < strlen(filename); x++) {
		if(((filename[x] >= ALPHA_NUMBERS_LOWER_BOUND && filename[x] <= ALPHA_NUMBERS_UPPER_BOUND) || 
			(filename[x] >= ALPHA_UPPERCASE_LOWER_BOUND && filename[x] <= ALPHA_UPPERCASE_UPPER_BOUND)) == false) 
			{      
					invalid++;   
					if(filename[x] == PERIOD) {
						periodCnt++;
                        periodFlag = true;
                    }
			} 
	}

	if(invalid > 1 ) {//more than 1 invalid character
		return 1;
	} else 
    if(invalid == 1 && periodCnt == 0) {//invalid that's not a period
        return 1;
    }
    else if(invalid == 1 && periodCnt == 1) { // a single period

        tokenCount = tokenize(filename, tokens, ".");

        if(tokenCount < 2) { //is filename or ext missing?
            return 2;
        }

        if(strlen(tokens[0]) > 8) { //is filename > 8?
            return 3;
        }

        if(strlen(tokens[1]) > 3) { //is ext < 3
            return 4;
        }
        return 0; //we've passed all check for filename with '.'
    } 
    else if(invalid == 0)  { //there's no period
		//is filename > 8?
		if(strlen(filename) > 8) {
			return 3;
		} else
            return 0; //we've passed all check for filename with no extenion
	}
	//re-tokenize once we exit here
    return -1;
}
//this does just what it sounds like
void printFilterError(const char * commandName, const char * filename, bool isDir, int exitCode) {
	char fileType[11];
    
	if(isDir == true)
		strcpy(fileType, "directory");
	else
		strcpy(fileType, "file");
		
	switch(exitCode) {
		case 1:
			printf("\nERROR: \"%s\" not a valid %s name: only uppercase alphameric characters allowed\n", filename, fileType);
			break;
		case 2:
			printf("\nERROR: %s failed on filename %s, name or extention not given", commandName, fileType);
			break;
		case 3:
			printf("\nERROR: %s failed, %s name \"%s\" longer than 8 characters", commandName, fileType, filename );
			break;
		case 4:
			printf("\nERROR: %s failed, extention \"%s\" longer than 3 characters", commandName, filename);
			break;
		case 5:
			printf("\nERROR: %s failed, cannot \"%s\" using \"%s\" as a name", commandName, commandName, filename);
			break;
	
	}
}

//------------ INITIALIZATION ------------------------------
/* loads up boot sector and initalizes file and directory tables

*/

void allocateFileTable() {
    //initalize and allocate open directory table
	environment.dirStatsTbl =  malloc(TBL_DIR_STATS * sizeof(FILEDESCRIPTOR));
	int i;
	for (i = 0; i < TBL_DIR_STATS; i++)
		environment.dirStatsTbl[i] = *( (FILEDESCRIPTOR *) malloc( sizeof(FILEDESCRIPTOR) ));

	//initalize and allocate open files table
	environment.openFilesTbl =  malloc(TBL_OPEN_FILES * sizeof(FILEDESCRIPTOR));
	for (i = 0; i < TBL_OPEN_FILES; i++)
		environment.openFilesTbl[i] = *( (FILEDESCRIPTOR *) malloc( sizeof(FILEDESCRIPTOR) ));
}
uint32_t getFileSize(char * filename)
{
    FILE * fdd = fopen(filename,"r");
    fseek(fdd, 0, SEEK_END); // seek to end of file
    u_int32_t  datasize = ftell(fdd); // get current file pointer
    fseek(fdd, 0, SEEK_SET); // seek back to beginning of file
    fclose(fdd);
    return datasize;
}

int initEnvironment(const char * imageName, fat32BS * boot_sector) {
    
	int f = open(imageName, O_RDWR);
    if(f == -1) {
        return 1;
    }
    strcpy(environment.imageName, imageName);
    read(f,boot_sector,sizeof(fat32BS));
    close(f);
    strcpy(environment.pwd, ROOT);
    strcpy(environment.last_pwd, ROOT);
	
    environment.pwd_cluster = boot_sector->BPB_RootClus;
    environment.tbl_dirStatsIndex = 0;
    environment.tbl_dirStatsSize = 0;
    environment.tbl_openFilesIndex = 0;
    environment.tbl_openFilesSize = 0;
	
    FAT_setIoWriteCluster(boot_sector, environment.pwd_cluster);
    allocateFileTable();

    return 0;
}
