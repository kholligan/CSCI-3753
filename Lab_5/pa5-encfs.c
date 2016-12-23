/*
 moune FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>
  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/
  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
  gcc -Wall `pkg-config fuse --cflags` fusexmp.c -o fusexmp `pkg-config fuse --libs`
  Note: This implementation is largely stateless and does not maintain
        open file handels between open and release calls (fi->fh).
        Instead, files are opened and closed as necessary inside read(), write(),
        etc calls. As such, the functions that rely on maintaining file handles are
        not implmented (fgetattr(), etc). Those seeking a more efficient and
        more complete implementation may wish to add fi->fh support to minimize
        open() and close() calls and support fh dependent functions.
*/

#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR
#define ENCRYPT 1
#define DECRYPT 0
#define PASS_THROUGH (-1)


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <assert.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include "aes-crypt.h"
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#define USAGE "Usage:\n\t./pa5-encfs <Key Phrase> <Mirror Directory> <Mount Point>"
#define ENCR_ATTR "user.pa5-encfs.encrypted"

char* root_directory;
char* password;

int check_encryption(const char *path)
{
	int attr;
	char xattr_val[5];
	getxattr(path, ENCR_ATTR, xattr_val, sizeof(char)*5);
	
	attr = (strcmp(xattr_val, "true") == 0);
	return attr;
}

int add_encryption_attr(const char *path)
{
	int attr;
	int setxattr_value;
	setxattr_value = setxattr(path, ENCR_ATTR, "true", (sizeof(char)*5), 0);
	attr = setxattr_value == 0;
	return attr;
}

char *rename_path(const char *path)
{
	size_t len = strlen(path) + strlen(root_directory) + 1;
	char *root_dir = malloc(len * sizeof(char));

	strcpy(root_dir, root_directory);
	strcat(root_dir, path);

	return root_dir;
}

static int xmp_getattr(const char *oldpath, struct stat *stbuf)
{
	char *path = rename_path(oldpath);

	int res;

	res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *oldpath, int mask)
{
	char *path = rename_path(oldpath);

	int res;

	res = access(path, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *oldpath, char *buf, size_t size)
{
	char *path = rename_path(oldpath);

	int res;

	res = readlink(path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}

static int xmp_readdir(const char *oldpath, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	char *path = rename_path(oldpath);

	DIR *dp;
	struct dirent *de;
	fprintf(stderr, "Path: %s\n", path);

	(void) offset;
	(void) fi;

	dp = opendir(path);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *oldpath, mode_t mode, dev_t rdev)
{
	char *path = rename_path(oldpath);

	int res;

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(path, mode);
	else
		res = mknod(path, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *oldpath, mode_t mode)
{
	char *path = rename_path(oldpath);

	int res;

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *oldpath)
{
	char *path = rename_path(oldpath);

	int res;

	res = unlink(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *oldpath)
{
	char *path = rename_path(oldpath);
	
	int res;

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *oldpath, mode_t mode)
{
	char *path = rename_path(oldpath);

	int res;

	res = chmod(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *oldpath, uid_t uid, gid_t gid)
{
	char *path = rename_path(oldpath);

	int res;

	res = lchown(path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *oldpath, off_t size)
{
	char *path = rename_path(oldpath);

	int res;

	res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *oldpath, const struct timespec ts[2])
{
	char *path = rename_path(oldpath);

	int res;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;
	res = utimes(path, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *oldpath, struct fuse_file_info *fi)
{
	char *path = rename_path(oldpath);
	int res;

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static inline int file_size(FILE *file) {
    struct stat st;

    if (fstat(fileno(file), &st) == 0)
        return st.st_size;

    return -1;
}

static int xmp_read(const char *oldpath, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	FILE *path_ptr, *tmpf;
	int res, encyption;
	char *path = rename_path(oldpath);
	path_ptr = fopen(path, "r");
	tmpf = tmpfile();

	//Decrypt encrypted files, otherwise do nothing
	encyption = check_encryption(path) ? DECRYPT : PASS_THROUGH;
	if (do_crypt(path_ptr, tmpf, encyption, password) == 0)
		return -errno;

	fflush(tmpf);
	fseek(tmpf, offset, SEEK_SET);

	//Read tmpfile to buffer
	res = fread(buf, 1, file_size(tmpf), tmpf);
	if (res == -1)
		res = -errno;

	//Cleanup
	fclose(tmpf);
	fclose(path_ptr);

	return res;
}

static int xmp_write(const char *oldpath, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	FILE *path_ptr, *tmpf;
	int res, encryption;
	int tmpf_descriptor;

	char *path = rename_path(oldpath);
	path_ptr = fopen(path, "r+");
	tmpf = tmpfile();
	tmpf_descriptor = fileno(tmpf);

	//Read the file into a tmpfile
	if (xmp_access(oldpath, R_OK) == 0 && file_size(path_ptr) > 0) {
		encryption = check_encryption(path) ? DECRYPT : PASS_THROUGH;
		if (do_crypt(path_ptr, tmpf, encryption, password) == 0)
			return --errno;

		rewind(path_ptr);
		rewind(tmpf);
	}

	//Read tmpfile to buffer
	res = pwrite(tmpf_descriptor, buf, size, offset);
	if (res == -1)
		res = -errno;
	encryption = check_encryption(path) ? ENCRYPT : PASS_THROUGH;

	if (do_crypt(tmpf, path_ptr, encryption, password) == 0)
		return -errno;

	fclose(tmpf);
	fclose(path_ptr);

	return res;
}

static int xmp_statfs(const char *oldpath, struct statvfs *stbuf)
{
	char *path = rename_path(oldpath);

	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_create(const char* oldpath, mode_t mode,
		      struct fuse_file_info* fi)
{
	char *path = rename_path(oldpath);

	(void) fi;

	int res;
	res = creat(path, mode);

	if(res == -1) {
		fprintf(stderr, "xmp_create: failed to creat\n");
		return -errno;
	}

	close(res);

	if (!add_encryption_attr(path)){
		fprintf(stderr, "xmp_create: failed to add xattr.\n");
		return -errno;
	}

	return 0;
}


static int xmp_release(const char *oldpath, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
	char *path = rename_path(oldpath);

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *oldpath, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
	char *path = rename_path(oldpath);

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int xmp_setxattr(const char *oldpath, const char *name,
			const char *value, size_t size, int flags)
{
	char *path = rename_path(oldpath);

	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *oldpath, const char *name, char *value,
			size_t size)
{
	char *path = rename_path(oldpath);

	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *oldpath, char *list, size_t size)
{
	char *path = rename_path(oldpath);

	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *oldpath, const char *name)
{
	char *path = rename_path(oldpath);

	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.create		= xmp_create,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	umask(0);

	if(argc < 4){
		fprintf(stderr, "Error: insufficient arguments. \n");
		fprintf(stderr, USAGE);
		exit(EXIT_FAILURE);
	}
	
	password = argv[1];
	root_directory = realpath(argv[2], NULL);
	
	//Update args to pass to fuse_main
	argv[1] = argv[3];
	argv[3] = NULL;
	argv[4] = NULL;
	argc = argc - 3;

	return fuse_main(argc, argv, &xmp_oper, NULL);
}

