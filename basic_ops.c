#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static const char *g_str = "To Infinity and beyond..\n";
static const char *g_path = "/what";
static FILE *fptr = NULL;

static int __getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, g_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(g_str);
	} else
		res = -ENOENT;

	return res;
}

static int __readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, g_path + 1, NULL, 0);

	return 0;
}

static int __open(const char *path, struct fuse_file_info *fi)
{
	printf("%s path: %s gpath: %s\n", __func__, path, g_path);
	if (strcmp(path, g_path) != 0)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int __read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	if(strcmp(path, g_path) != 0)
		return -ENOENT;
	fprintf(fptr, "path: %s gpath: %s\n", path, g_path);
	printf("path: %s gpath: %s\n", path, g_path);
	len = strlen(g_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, g_str + offset, size);
	} else
		size = 0;

	return size;
}

static void*
__init(struct fuse_conn_info *ptr)
{
	fptr = fopen("/tmp/fuse.log", "a+");
	if (!fptr)
		return (NULL);
	return (ptr);
}
static int
__write(const char *path, const char *buf, size_t size,
		      off_t offset, struct fuse_file_info *fi)
{
	(void)buf;
	(void)offset;
	(void)fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	return size;
}

static int
__truncate(const char *path, off_t size)
{
	(void)size;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	return 0;
}

static struct fuse_operations __oper = {
	.init		= __init,
	.getattr	= __getattr,
	.readdir	= __readdir,
	.open		= __open,
	.read		= __read,
	.write		= __write,
	.truncate	= __truncate
};

int
main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &__oper, NULL);
}
