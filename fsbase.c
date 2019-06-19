#include "fsbase.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

void fsb_load(FileSystemBase *fsb, FILE *storageFile)
{
    fsb->storageFile = storageFile;
    fseek(storageFile, 0, SEEK_SET);
    int result = fread(&fsb->superBlock, sizeof(SuperBlock), 1, storageFile);
    assert(result == 1);
}

void fsb_init(FileSystemBase *fsb,
              FILE *storageFile,
              unsigned int totalBlocks,
              unsigned int faBlocks,
              unsigned int blockSize)
{
    fsb->storageFile = storageFile;
    fseek(storageFile, 0, SEEK_SET);

    sb_init(&fsb->superBlock, totalBlocks, faBlocks, blockSize, faBlocks + 1);

    int result = fwrite(&fsb->superBlock, sizeof(SuperBlock), 1, storageFile);
    assert(result == 1);

    //set up FAT (no need to seek, we are at the end of the first block)
    unsigned int val = -1;
    for (int i = 0; i < faBlocks + 1; ++i) {
        result = fwrite(&val, 4, 1, storageFile);
        assert(result == 1);
    }

    //Value for root dir block, and start of root dir block.
    val = -2;
    result = fwrite(&val, 4, 1, storageFile);
    assert(result == 1);
    result = fwrite(&val, 4, 1, storageFile);
    assert(result == 1);

    //Value for the rest of the blocks
    val = 0;
    for (int i = 0; i < totalBlocks - faBlocks - 3; ++i) {
        result = fwrite(&val, 4, 1, storageFile);
        assert(result == 1);
    }

    //write root Dir
    Directory rootDir;
    dir_init(&rootDir, "", 0, faBlocks + 2, 1);
    fseek(storageFile, (faBlocks + 1) * blockSize, SEEK_SET);
    result = fwrite(&rootDir, sizeof(Directory), 1, storageFile);
    assert(result == 1);
}

Directory rootDirTmp;

int fsb_pathToDirectory(FileSystemBase *fsb,
                        char *path,
                        Directory *baseDir,
                        Directory *dir,
                        DirectoryLocation *dl)
{
    if (baseDir == NULL) {
        fsb_read(fsb, (char *)&rootDirTmp, fsb->superBlock.rootDirStart, 0, sizeof(Directory));
        baseDir = &rootDirTmp;
        if (dl != NULL) {
            dl->block = dl->index = 0;
        }
    }

    if (path == NULL) {
        memcpy(dir, baseDir, sizeof(Directory));
        return 0;
    }

    size_t pathLen = strlen(path);
    if (pathLen == 0) return -1;
    if (path[0] == '/') {
        if (pathLen == 1) {
            memcpy(dir, baseDir, sizeof(Directory));
            return 0;
        }
        ++path;
        --pathLen;
    }

    char *slash = memchr(path, '/', pathLen);
    if (slash != NULL) {
        //the current word in the path best be a directory...
        if (!(baseDir->flags | 1)) return -1;//Is a folder
        slash[0] = 0;
        ++slash;
        if (slash[0] == 0) slash = NULL;
    }

    unsigned int subDirCnt = baseDir->fileLength / sizeof(Directory);
    Directory *subDirs = malloc(baseDir->fileLength);
    int result = fsb_read(fsb, (char *)subDirs, baseDir->startBlock, 0, baseDir->fileLength);
    if (result != baseDir->fileLength) return -1;

    for (int i = 0; i < subDirCnt; ++i) {
        if (strcmp(path, subDirs[i].fileName) != 0) continue;

        //We have found the correct directory
        Directory newBase = subDirs[i];
        free(subDirs);
        if (dl != NULL) {
            dl->block = baseDir->startBlock;
            dl->index = i;
        }
        fsb_pathToDirectory(fsb, slash, &newBase, dir, dl);
        return 0;
    }

    return -1;
}

int fsb_read(FileSystemBase *fsb,
             char *dest,
             unsigned int startBlock,
             unsigned int startOffset,
             unsigned int numBytes)
{
    SuperBlock *sb = &fsb->superBlock;

    //we go ahead and load the whole FAT
    unsigned int fat[4 * sb->totalBlocks];
    fseek(fsb->storageFile, sb->blockSize, SEEK_SET);
    int result = fread(fat, 4, sb->totalBlocks, fsb->storageFile);
    if (result != sb->totalBlocks) return -1;
    //first get to start position. startOffset can be greater than blockSize, so some traversal
    //is needed.
    while (startOffset > sb->blockSize) {
        if (startBlock <= 0) return -1;
        startBlock = fat[startBlock];
        if (startBlock >= 0 && startBlock != -2) return -1;

        startOffset -= sb->blockSize;
    }

    //we are at the right block, now we start reading into dest
    while (1) {
        unsigned int readPos = startBlock * sb->blockSize + startOffset;
        unsigned bytesLeftInBlock = sb->blockSize - startOffset;
        startOffset = 0;

        unsigned int readAmount;
        if (bytesLeftInBlock < numBytes) {
            readAmount = bytesLeftInBlock;
        } else {
            readAmount = numBytes;
        }

        numBytes -= readAmount;

        fseek(fsb->storageFile, readPos, SEEK_SET);
        result = fread(dest, readAmount, 1, fsb->storageFile);
        if (result != 1) return -1;

        dest += readAmount;

        if (numBytes != 0) {
            if (startBlock <= 0) return -1;
            startBlock = fat[startBlock];
        } else {
            break;
        }
    }

    return 0;
}

int fsb_write(FileSystemBase *fsb,
              char *source,
              unsigned int startBlock,
              unsigned int startOffset,
              unsigned int numBytes)
{
    SuperBlock *sb = &fsb->superBlock;

    //we go ahead and load the whole FAT
    unsigned int fat[4 * sb->totalBlocks];
    fseek(fsb->storageFile, sb->blockSize, SEEK_SET);
    int result = fread(fat, 4, sb->totalBlocks, fsb->storageFile);
    if (result != sb->totalBlocks) return -1;
    //first get to start position. startOffset can be greater than blockSize, so some traversal
    //is needed.
    while (startOffset > sb->blockSize) {
        if (startBlock == -1 || startBlock == 0) return -1;
        //allocate more space for the file TODO update file's directory
        if (startBlock == -2) fsb_expandBlock(fsb, startBlock, fat);
        startBlock = fat[startBlock];
        if (startBlock <= 0 && startBlock != -2) return -1;

        startOffset -= sb->blockSize;
    }

    //we are at the right block, now we start writing from source
    while (1) {
        unsigned int writePos = startBlock * sb->blockSize + startOffset;
        unsigned bytesLeftInBlock = sb->blockSize - startOffset;
        startOffset = 0;

        unsigned int writeAmount;
        if (bytesLeftInBlock < numBytes) {
            writeAmount = bytesLeftInBlock;
        } else {
            writeAmount = numBytes;
        }

        numBytes -= writeAmount;

        fseek(fsb->storageFile, writePos, SEEK_SET);
        result = fwrite(source, writeAmount, 1, fsb->storageFile);
        if (result != 1) return -1;

        source += writeAmount;

        if (numBytes != 0) {
            if (startBlock == -1 || startBlock == 0) return -1;
            //allocate more space for the file TODO update file's directory
            if (startBlock == -2) fsb_expandBlock(fsb, startBlock, fat);
            startBlock = fat[startBlock];
            if (startBlock <= 0 && startBlock != -2) return -1;
        } else {
            break;
        }
    }

    return 0;
}

int fsb_addDir(FileSystemBase *fsb,
               char *path,
               char *name,
               int isFolder,
               Directory *dir,
               DirectoryLocation *dl)
{
    Directory parentDir;
    DirectoryLocation parentDirLoc;
    int result;

    result = fsb_pathToDirectory(fsb, path, NULL, &parentDir, &parentDirLoc);
    if (result != 0) return -1;

    //make the new directory
    dir_init(dir, name, 0, fsb_findFreeBlock(fsb, NULL), isFolder);

    //write the new directory
    result = fsb_write(fsb, (char *)dir, parentDir.startBlock, parentDir.fileLength, sizeof(Directory));
    if (result != 0) return -1;

    //set dl
    if (dl != NULL) {
        dl->block = parentDir.startBlock;
        dl->index = parentDir.fileLength / sizeof(Directory);
    }

    //update parentDir to reflect the addition
    parentDir.fileLength += sizeof(Directory);
    dir_updateModificationTime(&parentDir);

    //write parentDir back
    result = fsb_write(fsb, (char *)&parentDir, parentDirLoc.block, parentDirLoc.index * sizeof(Directory), sizeof(Directory));
    if (result != 0) return -1;
    return 0;
}

int fsb_removeFile(FileSystemBase *fsb, char *path)
{
    Directory dir;
    DirectoryLocation dl;

    int result = fsb_pathToDirectory(fsb, path, NULL, &dir, &dl);
    if (result != 0) return -1;

    if (dir.flags | 1) return -1;

    //TODO need to be able to update parent dir.
}

int fsb_removeFolder(FileSystemBase *fsb, char *path)
{

}

int fsb_freeChain(FileSystemBase *fsb, unsigned int startBlock, unsigned int *fat)
{
    int shouldFree = fat == NULL;

    if (fat == NULL) {
        SuperBlock *sb = &fsb->superBlock;
        fat = malloc(4 * sb->totalBlocks);
        fseek(fsb->storageFile, sb->blockSize, SEEK_SET);
        int result = fread(fat, 4, sb->totalBlocks, fsb->storageFile);
        if (result != sb->totalBlocks) {
            free(fat);
            return -1;
        }
    }

    unsigned int next = fat[startBlock];
    fat[startBlock] = 0;
    int result = fsb_write(fsb, (char*) &fat[startBlock], 1, startBlock * 4, 4);
    if (result != 0) return -1;
    if (next > 0) {
        result = fsb_freeChain(fsb, next, fat);
        if (shouldFree) free(fat);
        return result;
    }

    return 0;
}

unsigned int fsb_findFreeBlock(FileSystemBase *fsb, unsigned int *fat)
{
    SuperBlock *sb = &fsb->superBlock;
    int shouldFree = fat == NULL;

    if (fat == NULL) {
        fat = malloc(4 * sb->totalBlocks);

        fseek(fsb->storageFile, sb->blockSize, SEEK_SET);
        int result = fread(fat, 4, sb->totalBlocks, fsb->storageFile);
        if (result != sb->totalBlocks) {
            free(fat);
            return -1;
        }
    }

    for (int i = 0; i < sb->totalBlocks; ++i) {
        if (fat[i] != 0) continue;

        fat[i] = -2;
        int result = fsb_write(fsb, (char *)&fat[i], 1, i * 4, 4);
        if (result != 0) return -1;

        if (shouldFree) free(fat);
        return i;
    }

    if (shouldFree) free(fat);
    return -1;
}

int fsb_expandBlock(FileSystemBase *fsb, unsigned int block, unsigned int *fat)
{
    if (block != -2) return -1;

    unsigned int newBlock = fsb_findFreeBlock(fsb, fat);
    if (newBlock == -1) return -1;

    if (fat != NULL)
        fat[block] = newBlock;

    int result = fsb_write(fsb, (char *)&newBlock, 1, block * 4, 4);
    if (result != 0) return -1;
    return 0;
}
