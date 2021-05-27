# COMP 3413 Assignment 4 FAT32

Implemented a library to read directly from a FAT32 file system, the code is partially taken from other github source, allows commands like _info , dir , cd , get and put_ , there are still a few bugs in the code. Use _make all_ to create an executable, and run the code using.
```bash
./fat32 <diskimage>
```
It'll simulate a terminal opened at root directory in the fat32 diskimage. To run the code without bug use _fat32demo_.