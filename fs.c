/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include "unixv6fs.h"
#include "error.h"
#include "mount.h"
#include "inode.h"
#include "filev6.h"
#include "direntv6.h"

struct unix_filesystem fs;

static int fs_getattr(const char *path, struct stat *stbuf)
{
	int inr = direntv6_dirlookup(&fs, ROOT_INUMBER, path);
	if(inr < 0) return inr;
	
	struct inode inode;
	int err = inode_read(&fs, (uint16_t)inr, &inode);
	if(err < 0) return err;
	
	memset(stbuf, 0, sizeof(struct stat));
	stbuf->st_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
	
	if(inode.i_mode & IFDIR){
		stbuf->st_mode = stbuf->st_mode | S_IFDIR;
		stbuf->st_nlink = 2;
	}
	else {
		stbuf->st_mode = stbuf->st_mode | S_IFREG;
		stbuf->st_nlink = 1;
	}
	
	stbuf->st_size = inode_getsize(&inode);
	stbuf->st_blocks = inode_getsectorsize(&inode);
	stbuf->st_ino = inr;
	stbuf->st_blksize = SECTOR_SIZE;
	stbuf->st_uid = getuid();
	stbuf->st_gid = getgid();

	return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	struct directory_reader d;
	char name[DIRENT_MAXLEN+1];
	int err = direntv6_dirlookup(&fs, ROOT_INUMBER, path);
	if (err<0) return err;
	uint16_t inr = err;
	err = direntv6_opendir(&fs,inr,&d);
	if (err<0) return err;
		
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	int r = direntv6_nonempty(&d);
	while (r){
		//we change the condition of the loop r
		r = direntv6_readdir(&d,name,&inr);
		if (r<0) return r;
		filler(buf,name,NULL,0);
	}
	return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	(void) fi;
	int inr = direntv6_dirlookup(&fs, ROOT_INUMBER, path);
	if(inr < 0) return 0;

	struct filev6 fv6;
	int err = filev6_open(&fs, inr, &fv6);
	if(err < 0) return 0;
	
	if (!(fv6.i_node.i_mode & IALLOC)) return ERR_UNALLOCATED_INODE;
	if (fv6.i_node.i_mode & IFDIR) return ERR_BAD_PARAMETER;
	err = filev6_lseek(&fv6, offset);
	if(err < 0) return 0;
	
	uint8_t tab[SECTOR_SIZE];
	int readBytes = 0;
	unsigned int total = 0;
	while(total < size && (readBytes = filev6_readblock(&fv6, tab)) > 0){
		//copy 'size' bytes of the fv6 to buf
		memcpy(buf + total, tab, readBytes);
		total += readBytes;
	}
	return total;
}

static struct fuse_operations available_ops = {
	.getattr	= fs_getattr,
	.readdir	= fs_readdir,
	.read		= fs_read,
};

/* From https://github.com/libfuse/libfuse/wiki/Option-Parsing.
 * This will look up into the args to search for the name of the FS.
 */
static int arg_parse(void *data, const char *filename, int key, struct fuse_args *outargs)
{
    (void) data;
    (void) outargs;
    if (key == FUSE_OPT_KEY_NONOPT && fs.f == NULL && filename != NULL) {
		int err = mountv6(filename, &fs);
		if (err<0){
			printf("ERROR FS: %s\n", ERR_MESSAGES[err - ERR_FIRST]);
			exit(1);
		}
        return err;
    }
    return 1;
}

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    int ret = fuse_opt_parse(&args, NULL, NULL, arg_parse);
    if (ret == 0) {
        ret = fuse_main(args.argc, args.argv, &available_ops, NULL);
        (void)umountv6(&fs);
    }
    return ret;
}
