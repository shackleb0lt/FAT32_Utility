#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <fcntl.h>

#include "shell.h"

int main(int argc, char *argv[]) 
{
	if (argc != 2)
	{
		printf("Usage: %s <file>\n", argv[0]);
		exit(1);
	}

	shellLoop(argv[1]);
}
