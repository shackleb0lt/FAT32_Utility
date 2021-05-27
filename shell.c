#include "shell.h"
#include "fat32.h"


void showPromptAndClearBuffer(char * buffer) {
    printf("\n>");
    bzero(buffer, BUF_SIZE);
}

void shellLoop(const char * filename) 
{

	fat32BS boot_sector;
    FILEDESCRIPTOR searchFileInflator;
    FILEDESCRIPTOR * searchFile = &searchFileInflator;
    

    char bufferRaw[BUF_SIZE];
    char * my_argv[400];
    
    bzero(bufferRaw, 100);
    int argC = 0;

    char * tokens[10];
	int exitCode;

	bool running = true;


	if(initEnvironment(filename, &boot_sector) == 1) {
        printf("\nFATAL ERROR: %s image not found\n", filename);
        running = false;
    }
    long long int free = FAT_findTotalFreeCluster(&boot_sector);
    free*= boot_sector.BPB_SecPerClus*boot_sector.BPB_BytesPerSec;
	printf(">");
	while(running) 
	{		
		if (fgets(bufferRaw, BUF_SIZE, stdin) == NULL) 
		{
			running = false;
			continue;
		}
        bufferRaw[strlen(bufferRaw)-1] = '\0';
        argC = tokenize(bufferRaw, my_argv, " ");

	
		for (int i=0; i < strlen(my_argv[0])+1; i++)
			my_argv[0][i] = toupper(my_argv[0][i]);

		if(strcmp(my_argv[0], CMD_EXIT) == 0) //EXIT COMMAND
        {
            running = false;
            continue;
        }

		else if (strncmp(my_argv[0], CMD_INFO, strlen(CMD_INFO)) == 0)
		{
			printInfo(&boot_sector);
            showPromptAndClearBuffer(bufferRaw);
            continue;
		}	

		else if (strncmp(my_argv[0], CMD_DIR, strlen(CMD_DIR)) == 0){
			doDir(&boot_sector, environment.pwd_cluster, false, environment.pwd, false, searchFile, false, false);
            printf("---Bytes Free: %lld\n---DONE\n",free);
            showPromptAndClearBuffer(bufferRaw);
            continue;
		}
	
		else if (strncmp(my_argv[0], CMD_CD, strlen(CMD_CD)) == 0)
		{
			if(argC != 2)
                puts("\nUsage: cd <directory_name | path>\n");
            
            if(strcmp(my_argv[1], SELF) == 0) // going to '.'
                ;
            else if(strcmp(my_argv[1], ROOT) == 0) {// going to '/'
                    strcpy(environment.last_pwd, ROOT);
                    strcpy(environment.pwd, ROOT);
                    environment.pwd_cluster = boot_sector.BPB_RootClus;
                    FAT_setIoWriteCluster(&boot_sector, environment.pwd_cluster);
            } 

            else if(strcmp(my_argv[1], PARENT) == 0) {
                //going up the tree
                if(doCD(&boot_sector, PARENT, true, searchFile) == true) ;

                else puts("\n ERROR: directory not found: '..'\n");
                
            }
            else {
                if(doCD(&boot_sector, my_argv[1], false, searchFile) == true) {
                    strcpy(environment.last_pwd, environment.pwd);
                    strcpy(environment.pwd, my_argv[1]);
                } 
                else {
                    if(doCD(&boot_sector, PARENT, false, searchFile) == true)
                        printf("\n ERROR: \"%s\" is a file:\n", my_argv[1]);
                    else
                        printf("\nERROR: directory not found: %s\n", my_argv[1]);
                }
            }
            showPromptAndClearBuffer(bufferRaw);
            continue;
		}

		else if (strncmp(my_argv[0], CMD_GET, strlen(CMD_GET)) == 0)
		{
			if(argC < 2) {
                puts("\nUsage: get <filename>\n");
                showPromptAndClearBuffer(bufferRaw);
                continue;
            }
            if((exitCode = filterFilename(my_argv[1], false, false)) != SUCCESS) 
            {
                printFilterError("fread", my_argv[1], false, exitCode);
                showPromptAndClearBuffer(bufferRaw);
                continue;
            }
            if(searchForFile(&boot_sector, my_argv[1], false, searchFile) == false ) {
                printf("ERROR: failed to get, \"%s\" does not exists", my_argv[1]);
                showPromptAndClearBuffer(bufferRaw);
                continue;
            }

            fOpen(&boot_sector, argC, my_argv[1], "r");
            uint32_t actualBytesRead;
            fRead(&boot_sector, my_argv[1], "0", &actualBytesRead);
            fClose(&boot_sector, argC, my_argv[1]);

            showPromptAndClearBuffer(bufferRaw);
            continue;

		}

		else if (strncmp(my_argv[0], CMD_PUT, strlen(CMD_PUT)) == 0)
		{
			printf("Bonus marks!\n");
			
            int tokenCnt = 0;
            char commandName[4]="put";
            char fileType[5]="file";
            char fullFilename[13];

            //if only "put" is typed with no args
            if(argC != 2) {
                printf("\nUsage: %s <%sname>\n", commandName, fileType);
                showPromptAndClearBuffer(bufferRaw);
                continue;    
            } 

            //fetching the file size       
            u_int32_t  datasize = getFileSize(my_argv[1]);

            //opening file for write
            int f = open(my_argv[1],O_RDWR);
            if(f==-1)
            {
                printf("File %s not found on your disk\n",my_argv[1]);
                showPromptAndClearBuffer(bufferRaw);
                continue;
            }

            
            char filename[strlen(my_argv[1])+1];

            for(int i=0;i<strlen(my_argv[1]);i++){
                my_argv[1][i] = toupper(my_argv[1][i]);
                filename[i] = my_argv[1][i]; 
            }
            filename[strlen(my_argv[1])]='\0';
            
            uint32_t targetCluster = environment.pwd_cluster;

            //check if filename is valid
            if((exitCode = filterFilename(my_argv[1], true, false)) != SUCCESS) {
                printFilterError(commandName, my_argv[1], false, exitCode);
                showPromptAndClearBuffer(bufferRaw);
                continue;
            }
                    
            strcpy(fullFilename, my_argv[1]);
            tokenCnt = tokenize(my_argv[1], tokens, ".");
            

            if(searchForFile(&boot_sector, fullFilename, false, searchFile) == true ) {
                printf("ERROR: %s failed, \"%s\" already exists", commandName, fullFilename);
            }
            //creating file entry on FAT diskimage
            else {
                if(tokenCnt > 1)
                    touch(&boot_sector, tokens[0], tokens[1], targetCluster);
                else
                    touch(&boot_sector, tokens[0], "", targetCluster);
                
                printf("%s \"%s\" created\n", fileType, my_argv[1]);
            }
           

            fOpen(&boot_sector, argC, filename, "w");

            char temp_buffer[datasize+1];
            read(f,temp_buffer,datasize);

            fWrite(&boot_sector, filename, 0, temp_buffer,datasize);
            fClose(&boot_sector, argC, filename);
            close(f);

            showPromptAndClearBuffer(bufferRaw);
            continue;
		}
		
		else{
			printf("\nCommand not found\n");
			showPromptAndClearBuffer(bufferRaw);
            continue;
		}
			
	}
	printf("\nExited...\n");
}