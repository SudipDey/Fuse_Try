CC=gcc
Cflags=-I/usr/include/fuse -pthread -lfuse -D_FILE_OFFSET_BITS=64 -Wall \
	-I/usr/include/
all: basic_ops.c
	$(CC) basic_ops.c $(Cflags) -o basic_ops
clean:
	rm -rf basic_ops
