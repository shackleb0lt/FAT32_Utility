#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#define BUF_SIZE 256
#define CMD_INFO "INFO"
#define CMD_DIR "DIR"
#define CMD_CD "CD"
#define CMD_GET "GET"
#define CMD_PUT "PUT"
#define CMD_EXIT "EXIT"


void showPromptAndClearBuffer(char * buffer);
void shellLoop(const char * filename);

#endif
