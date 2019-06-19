#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H

typedef struct {
    unsigned int magicNumber;
    unsigned int totalBlocks;
    unsigned int fileAllocBlocks;
    unsigned int blockSize;
    unsigned int rootDirStart;
} SuperBlock;

void sb_init(SuperBlock *sb,
             unsigned int totalBlocks,
             unsigned int faBlocks,
             unsigned int blockSize,
             unsigned int rootDirStart);

#endif /* end of include guard: SUPERBLOCK_H */
