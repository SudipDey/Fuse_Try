#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#ifdef linux
#define _XOPEN_SOURCE 500
#endif
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <unistd.h>
#define PATHLEN_MAX 1024
#include "basic_ops_impl.h"
// #include "debugf.c"

static char working_dir[PATHLEN_MAX+1] = {'\0'};
static char cached_mountpoint[PATHLEN_MAX+1] = {'\0'};
static int mnt_fd;

const char *
get_real_mnt(const char *path)
{
	char *ep, *buff;

	printf("%s Path send by Fuse is : %s\n", __func__, path);
	buff = strdup(path+1);
	if (buff == NULL)
		exit(1);

	ep = buff + strlen(buff) - 1;
	if (*ep == '/')
		*ep = '\0';
	if (*buff == '\0')
		strcpy(buff, ".");

	return (buff);
}

static int __getattr(const char *path, struct stat *stbuf)
{
	int ret;

	path = get_real_mnt(path);
	printf("fuse_: getattr(%s)\n", path);
	ret = lstat(path, stbuf);
	if (ret == -1)
		return (-errno);
	do_file_ops_accounting();
	return (0);
}

static int __access(const char *path, int mask)
{
	int ret;

	path = get_real_mnt(path);
	printf("fuse_: access(%s)\n", path);
	ret = access(path, mask);
	if (ret == -1)
		return (-errno);

	do_file_ops_accounting();

	return (0);
}

static int __readlink(const char *path, char *buf, size_t size)
{
	int ret;

	path = get_real_mnt(path);
	printf("fuse_: readlink(%s)\n", path);
	ret = readlink(path, buf, size - 1);
	if (ret == -1)
		return (-errno);
	buf[ret] = '\0';
	do_file_ops_accounting();

	return (0);
}


static int __readdir(const char *path, void *buf, fuse_fill_dir_t filler,
				   off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *di;
	struct stat st;
	int ret = 0;

	(void) offset;
	(void) fi;

	path = get_real_mnt(path);
	printf("fuse_: readdir(%s)\n", path);
	dp = opendir(path);
	if (!dp)
		return (-errno);

	while ((di = readdir(dp))) {
		memset(&st, 0, sizeof(st));
		if (stat(di->d_name, &st))
			return (-errno);
		if (filler(buf, di->d_name, &st, 0))
				break;
	}

	closedir(dp);
	return (0);
}

static int __mknod(const char *path, mode_t mode, dev_t rdev)
{
	int ret;

	path = get_real_mnt(path);
	printf("fuse_: mknod(%s)\n", path);
	if (S_ISREG(mode)) {
		ret = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (ret >= 0)
				ret = close(ret);
	} else if (S_ISFIFO(mode)) {
		ret = mkfifo(path, mode);
	} else {
		ret = mknod(path, mode, rdev);
	}
	if (ret == -1)
		return (-errno);

	return (0);
}

static int __mkdir(const char *path, mode_t mode)
{
	int ret;

	path = get_real_mnt(path);
	printf("fuse_: mkdir(%s)\n", path);
	ret = mkdir(path, mode);
	if (ret)
		return (-errno);

	return (0);
}

static int __unlink(const char *path)
{
	int ret;

	path = get_real_mnt(path);
	printf("fuse_: unlink(%s)\n", path);
	ret = unlink(path);
	if (ret)
		return (-errno);

	return (0);
}

static int __rmdir(const char *path)
{
	int ret;

	path = get_real_mnt(path);
	printf("fuse_: rmdir(%s)\n", path);
	ret = rmdir(path);
	if (ret)
		return (errno);

	return (0);
}

static int __symlink(const char *from, const char *to)
{
	int ret;

	from = get_real_mnt(from);
	to = get_real_mnt(to);
	printf("fuse_: symlink(%s, %s)\n", from, to);
	ret = symlink(from, to);
	if (ret)
		return (-errno);
	return (0);
}

static int __rename(const char *from, const char *to)
{
	int ret;

	from = get_real_mnt(from);
	to = get_real_mnt(to);
	printf("fuse_: rename(%s, %s)\n", from, to);
	ret = rename(from, to);
	if (ret)
		return (-errno);

	return (0);
}

static int __link(const char *from, const char *to)
{
	int ret;

	from = get_real_mnt(from);
	to = get_real_mnt(to);
	printf("fuse_: link(%s, %s)\n", from, to);
	ret = link(from, to);
	if (ret)
		return (-errno);

	return (0);
}

static int __chmod(const char *path, mode_t mode)
{
	int ret;

	path = get_real_mnt(path);
	printf("fuse_: chmod(%s)\n", path);
	ret = chmod(path, mode);
	if (ret == -1)
		return (-errno);

	return (0);
}

static int __chown(const char *path, uid_t uid, gid_t gid)
{
	int ret;

path = get_real_mnt(path);
printf("fuse_: lchown(%s)\n", path);
	ret = lchown(path, uid, gid);
	if (ret == -1)
		return (-errno);

	return (0);
}

static int __truncate(const char *path, off_t size)
{
	int ret;

path = get_real_mnt(path);
printf("fuse_: truncate(%s)\n", path);
	ret = truncate(path, size);
	if (ret == -1)
		return (-errno);

	return (0);
}

static int __utimens(const char *path, const struct timespec ts[2])
{
	int ret;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	path = get_real_mnt(path);
	printf("fuse_: utimes(%s)\n", path);
	ret = utimes(path, tv);
	if (ret == -1)
		return (-errno);

	return (0);
}

static int __open(const char *path, struct fuse_file_info *fi)
{
	int ret;

path = get_real_mnt(path);
printf("fuse_: open(%s)\n", path);
	ret = open(path, fi->flags);
	if (ret == -1)
		return (-errno);

	close(ret);
	return (0);
}

static int __read(const char *path, char *buf, size_t size, off_t offset,
					struct fuse_file_info *fi)
{
	int fd;
	int ret;

	(void) fi;
	path = get_real_mnt(path);
	printf("fuse_: read(%s)\n", path);
	fd = open(path, O_RDONLY);
	if (fd == -1)
		return (-errno);

	ret = pread(fd, buf, size, offset);
	if (ret == -1)
		ret = -errno;

	close(fd);
	return (ret);
}

static int __write(const char *path, const char *buf, size_t size,
			 off_t offset, struct fuse_file_info *fi)
{

// actually we don't need to look at the 'open' calls - when
// we call 'write', flag the file's private data with the
// fact that the file was updated, so that we can take a
// copy when we get the 'release' call.

	int fd;
	int ret;

	(void) fi;
	path = get_real_mnt(path);
	printf("fuse_: write(%s)\n", path);
	fd = open(path, O_WRONLY);
	if (fd == -1)
		return (-errno);

	ret = pwrite(fd, buf, size, offset);
	if (ret == -1)
		ret = -errno;

	close(fd);
	return (ret);
}

static int __statfs(const char *path, struct statvfs *stbuf)
{
	int ret;

	path = get_real_mnt(path);
	printf("fuse_: statvfs(%s)\n", path);
	ret = statvfs(path, stbuf);
	if (ret == -1)
		return (-errno);

	return (0);
}

static int __release(const char *path, struct fuse_file_info *fi)
{
// Release is called when there are no more open handles.  This is where
// we do whatever action we want to with the file as all updates are
// now complete.  For example, calling gpg to encrypt it, or rsync
// to transfer it to disaster-recovery storage

// OR look at fi->flags for write access, and assume if opened
// for write, it will have been written to

	path = get_real_mnt(path);
	printf("fuse_: release(%s) flags=%02x\n", path, fi->flags);
	if ((fi->flags&1) != 0) {
	  printf("fuse_ TRIGGER: save file to /mnt/backup/%s\n", path);
	}
	(void) path;
	(void) fi;
	return (0);
}

static int __fsync(const char *path, int isdatasync,
					 struct fuse_file_info *fi)
{
	/*
	 * Just a stub.  This method is optional and can safely be left
	 * unimplemented
	 */

	path = get_real_mnt(path);
	printf("fuse_: fsync(%s)\n", path);
	(void) path;
	(void) isdatasync;
	(void) fi;
	return (0);
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int __setxattr(const char *path, const char *name, const char *value,
						size_t size, int flags)
{
	int ret;

	path = get_real_mnt(path);
	printf("fuse_: setxattr(%s)\n", path);
	ret = lsetxattr(path, name, value, size, flags);
	if (ret == -1)
		return (-errno);
	return (0);
}

static int __getxattr(const char *path, const char *name, char *value,
					size_t size)
{
	int ret;

	path = get_real_mnt(path);
	printf("fuse_: getxattr(%s)\n", path);
	ret = lgetxattr(path, name, value, size);
	if (ret == -1)
		return (-errno);
	return (ret);
}

static int __listxattr(const char *path, char *list, size_t size)
{
	int ret;

	path = get_real_mnt(path);
	printf("fuse_: listxattr(%s)\n", path);
	ret = llistxattr(path, list, size);
	if (ret == -1)
		return (-errno);
	return (ret);
}

static int __removexattr(const char *path, const char *name)
{
	int ret;

	path = get_real_mnt(path);
	printf("fuse_: removexattr(%s)\n", path);
	ret = lremovexattr(path, name);
	if (ret == -1)
		return (-errno);
	return (0);
}
#endif /* HAVE_SETXATTR */

static void *
__init(void)
{
	printf("%s\n", __func__);
	fchdir(mnt_fd);
	close(mnt_fd);
	return (NULL);
}

static struct fuse_operations fuse_ops = {
	.init		= __init,
	.getattr	= __getattr,
	.access	= __access,
	.readlink	= __readlink,
	.readdir	= __readdir,
	.mknod	= __mknod,
	.mkdir	= __mkdir,
	.symlink	= __symlink,
	.unlink	= __unlink,
	.rmdir	= __rmdir,
	.rename	= __rename,
	.link	= __link,
	.chmod	= __chmod,
	.chown	= __chown,
	.truncate	= __truncate,
	.utimens	= __utimens,
	.open	= __open,
	.read	= __read,
	.write	= __write,
	.statfs	= __statfs,
	.release	= __release,
	.fsync	= __fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= __setxattr,
	.getxattr	= __getxattr,
	.listxattr	= __listxattr,
	.removexattr	= __removexattr,
#endif
};

int
main(int argc, char *argv[])
{
	int rc;
	umask(0);
	getcwd(working_dir, PATHLEN_MAX);
	printf("%s: cwd=%s\n", __func__, working_dir);
	printf("%s: main(%s, %s, %s, %s)\n", __func__, argv[0],
				argv[1], argv[2], argv[3]);
	strncpy(cached_mountpoint, argv[1], strlen(argv[1]));
	printf("%s: mountpoint=%s\n", __func__, cached_mountpoint);

	mnt_fd = open(cached_mountpoint, O_RDONLY);
	rc = fuse_main(argc, argv, &fuse_ops, NULL);

	printf("%s: umount(%s, %s, %s, %s)\n", __func__,
		argv[0], argv[1], argv[2], argv[3]);
	return (rc);
}
