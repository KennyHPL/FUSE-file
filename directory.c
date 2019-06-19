#include "directory.h"

#include <string.h>
#include <time.h>

long long int getCurrentTime()
{
    return time(NULL);
}

void dir_init(Directory *dir,
              char *name,
              unsigned int length,
              unsigned int startBlock,
              unsigned int isFolder)
{
    size_t nameLen = strlen(name);
    if (nameLen >= 24) {
        memcpy(dir->fileName, name, 23);
        dir->fileName[23] = 0;
    } else {
        memcpy(dir->fileName, name, nameLen);
    }

    dir->creTime = dir->modTime = dir->accTime = getCurrentTime();
    dir->fileLength = length;
    dir->startBlock = startBlock;
    dir->flags = (isFolder? 1 : 0);
}

void dir_updateModificationTime(Directory *dir)
{
    dir->modTime = dir->accTime = getCurrentTime();
}

void dir_updateAccessTime(Directory *dir)
{
    dir->accTime = getCurrentTime();
}
