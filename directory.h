#ifndef DIRECTORY_H
#define DIRECTORY_H

typedef struct {
    char fileName[24];
    long long int creTime;
    long long int modTime;
    long long int accTime;
    unsigned int fileLength;
    unsigned int startBlock;
    unsigned int flags;
    unsigned int padding;
} Directory;

//sets up the values of a new directory, also setting all time values to current time
void dir_init(Directory *dir,
              char *name,
              unsigned int length,
              unsigned int startBlock,
              unsigned int isFolder);

//updates modTime and accTime to current time
void dir_updateModificationTime(Directory *dir);
//updates accTime
void dir_updateAccessTime(Directory *dir);

#endif //file gaurd DIRECTORY_H
