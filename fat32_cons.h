#ifndef FAT32_CONS_H
#define FAAT32_CONS_H

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

#endif