CC=gcc
Cflags=-I/usr/include/fuse -pthread -lfuse -D_FILE_OFFSET_BITS=64 -Wall \
	-I/usr/include/ -lm
Cflags += -g -D_HAVE_SETXATTR -D_linux

all: basic_ops_impl.c ops_acc_impl.c 
	$(CC) basic_ops_impl.c ops_acc_impl.c $(Cflags) -o basic_ops_impl
clean:
	rm -rf basic_ops_impl
