#ifndef FSBASE_H
#define FSBASE_H

#include "superblock.h"
#include "directory.h"

#include <stdio.h>

typedef struct {
    FILE *storageFile;
    //will be written into the file. Keep this in memory as it never changes, and is needed often
    SuperBlock superBlock;
} FileSystemBase;

typedef struct {
    unsigned int block;
    unsigned int index;
} DirectoryLocation;

//assumes storageFile is an open binary file, already with data
void fsb_load(FileSystemBase *fsb, FILE *storageFile);

//assumes storageFile is an open new binary file
void fsb_init(FileSystemBase *fsb,
              FILE *storageFile,
              unsigned int totalBlocks,
              unsigned int faBlocks,
              unsigned int blockSize);

//path will be modified! Copy first if you don't want it to be messed up.
//If baseDir is null, then the root directory will be used (make it null if in doubt)
//The result will be placed in directory
//If dl != NULL, the directory location will be placed in dl
//0 returned on success, -1 on failure.
int fsb_pathToDirectory(FileSystemBase *fsb,
                        char *path,
                        Directory *baseDir,
                        Directory *dir,
                        DirectoryLocation *dl);

//dest must have at least numBytes of memory claimed
//DOES NOT change anything to do with directory access time
//returns 0 on success, -1 on failure
int fsb_read(FileSystemBase *fsb,
             char *dest,
             unsigned int startBlock,
             unsigned int startOffset,
             unsigned int numBytes);

int fsb_write(FileSystemBase *fsb,
              char *source,
              unsigned int startBlock,
              unsigned int startOffset,
              unsigned int numBytes);

//Make a new directory as a subdir of path, with name name.
//Location of directory entry will be put in dl if dl != NULL
//returns 0 on success, -1 on failure
int fsb_addDir(FileSystemBase *fsb,
               char *path,
               char *name,
               int isFolder,
               Directory *dir,
               DirectoryLocation *dl);

//removes a file at path. Will not work on folders
//returns 0 on success, -1 on failure
int fsb_removeFile(FileSystemBase *fsb, char *path);

//removes a directory, and all subdirectories. If path is a file, will work too
//returns 0 on success, -1 on failure
int fsb_removeFolder(FileSystemBase *fsb, char *path);

//frees all the blocks in the chain starting with startBlock
//If fat == NULL, the FAT will be loaded
//If fat is provided, then both the file, and the passed in fat will be updated
//returns 0 on success, -1 on failure
int fsb_freeChain(FileSystemBase *fsb, unsigned int startBlock, unsigned int *fat);

//Finds a free block, and marks it as -2 (last block in file)
//If fat == NULL, the FAT will be loaded
//If fat is provided, then both the file, and the passed in fat will be updated
//returns the block allocated
unsigned int fsb_findFreeBlock(FileSystemBase *fsb, unsigned int *fat);

//block should be -2 in the FAT. Changes it to point to a newly allocated block
//If fat == NULL, the FAT will be loaded
//If fat is provided, then both the file, and the passed in fat will be updated
//returns 0 on success, -1 on failure
int fsb_expandBlock(FileSystemBase *fsb, unsigned int block, unsigned int *fat);

#endif /* end of include guard: FSBASE_H */
