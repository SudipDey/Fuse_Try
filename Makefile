all: hello.c
	gcc -Wall hello.c -D_FILE_OFFSET_BITS=64 -I/usr/include/fuse -pthread \
	-lfuse -o hello
