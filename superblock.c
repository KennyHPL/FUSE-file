#include "superblock.h"

void sb_init(SuperBlock *sb,
             unsigned int totalBlocks,
             unsigned int faBlocks,
             unsigned int blockSize,
             unsigned int rootDirStart)
{
    sb->magicNumber = 0xfa19283e;
    sb->totalBlocks = totalBlocks;
    sb->fileAllocBlocks = faBlocks;
    sb->blockSize = blockSize;
    sb->rootDirStart = rootDirStart;
}
