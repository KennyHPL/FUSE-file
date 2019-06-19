#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "directory.h"
#include "superblock.h"
#include "fsbase.h"

static FileSystemBase fsb;

static int ops_getattr(const char *path, struct stat *stbuf)
{
    printf("ops_getattr: path: %s\n", path);
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));
    Directory dir;
    DirectoryLocation dl;
    fsb_pathToDirectory(&fsb, path, NULL, &dir, &dl);
    printf("ops_getattr: dir is: fileLength: %d, startBlock: %d\n", 
        dir.fileLength, dir.startBlock);
    stbuf->st_ctime = dir.creTime;
    stbuf->st_atime = dir.accTime;
    stbuf->st_mtime = dir.modTime;
    stbuf->st_size = dir.fileLength;
    stbuf->st_blocks = dir.startBlock;
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();
    stbuf->st_mode = S_IFDIR | 0777;

    return res;
}

static int ops_readdir(const char *path,
                       void *buf,
                       fuse_fill_dir_t filler,
                       off_t offset,
                       struct fuse_file_info *fi)
{
    printf("ops_readdir: path is: %s\n", path);

    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    //filler(buf, ".", NULL, 0);
    //filler(buf, "..", NULL, 0);
    //filler(buf, hello_path + 1, NULL, 0);

    return 0;
}

static int ops_opendir(const char *path, struct fuse_file_info *fi)
{
    //Not implemented
    return 0;
}

static int ops_mkdir(const char *path, mode_t mode)
{
    printf("ops_mkdir: path: %s\n", path);
    char* name = "test.txt";
    Directory dir;
    fsb_addDir(&fsb, path, name, 1, &dir, NULL);
    return 0;
}

static int ops_rmdir(const char *path)
{
    //Not implemented
    return 0;
}

static int ops_open(const char *path , struct fuse_file_info *fi)
{
    //Not implemented
    return 0;
}

static int ops_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
    printf("ops_read: path is: %s, size is: %ld, offset: %ld\n", 
        path, size, offset);
    //Not implemented
    return size;
}

static int ops_create(const char *path, mode_t mide, struct fuse_file_info *fi)
{
    //Not implemented
    return 0;
}

static struct fuse_operations ops = {
    .getattr    = ops_getattr,
    .readdir    = ops_readdir,
    .opendir    = ops_opendir,
    .mkdir      = ops_mkdir,
    .rmdir      = ops_rmdir,
    .open       = ops_open,
    .read       = ops_read,
    .create     = ops_create,
};

int main(int argc, char *argv[])
{
    FILE *file = fopen("disk.img", "w+a");
    fsb_init(&fsb, file, 100, 1, 1024);
    return fuse_main(argc, argv, &ops, NULL);
    //return 0;
}
