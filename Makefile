CC = gcc
CFLAGS = -Wall -g -std=gnu99

LDLIBS =

OBJS = main.o shell.o fat32.o

EXE = fat32 

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(EXE) $(LDLIBS)

shell.o: shell.c shell.h fat32.h
	$(CC) $(CFLAGS) -c shell.c

fat32.o: fat32.h fat32.c
	$(CC) $(CFLAGS) -c fat32.c

main.o: main.c shell.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f $(OBJS)
	rm -f *~
	rm -f $(EXE)

