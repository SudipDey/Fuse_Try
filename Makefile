CC=gcc
Cflags=-I/usr/include/fuse -pthread -lfuse -D_FILE_OFFSET_BITS=64 -Wall
all: hello.c
	$(CC) hello.c $(Cflags) -o hello
clean:
	rm -rf hello
