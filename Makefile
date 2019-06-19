CC=gcc
CFLAGS= -D_FILE_OFFSET_BITS=64
OBJS= directory.o fsbase.o superblock.o main.o

fileSystem : $(OBJS)
	$(CC) -o $@ `pkgconf fuse --cflags --libs` $(OBJS)


%.o : %.c
	$(CC) -c -o $@

clean:
	rm $(OBJS)
