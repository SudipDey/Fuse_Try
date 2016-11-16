CC=gcc
Cflags=-I/usr/include/fuse -pthread -lfuse -D_FILE_OFFSET_BITS=64 -Wall \
	-I/usr/include/ -lm
Cflags += -g

all: basic_ops.c sfs_impl
	$(CC) basic_ops.c $(Cflags) -o basic_ops

sfs_impl : sfs_impl.c
	$(CC) sfs_impl.c $(Cflags) -o sfs_impl
clean:
	rm -rf basic_ops sfs_impl
