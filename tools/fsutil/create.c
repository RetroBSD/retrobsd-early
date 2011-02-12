/*
 * Create new unix v6 filesystem.
 *
 * Copyright (C) 2006 Serge Vakulenko, <vak@cronyx.ru>
 *
 * This file is part of BKUNIX project, which is distributed
 * under the terms of the GNU General Public License (GPL).
 * See the accompanying file "COPYING" for more details.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "bsdfs.h"

extern int verbose;

static int build_inode_list (fs_t *fs)
{
	fs_inode_t inode;
	unsigned int inum, total_inodes;

	total_inodes = fs->isize * 16;
	for (inum = 1; inum <= total_inodes; inum++) {
		if (! fs_inode_get (fs, &inode, inum))
			return 0;
		if (inode.mode == 0) {
			fs->inode [fs->ninode++] = inum;
			if (fs->ninode >= 100)
				break;
		}
	}
	return 1;
}

static int create_root_directory (fs_t *fs)
{
	fs_inode_t inode;
	unsigned char buf [BSDFS_BSIZE];
	unsigned int bno;

	memset (&inode, 0, sizeof(inode));
	inode.mode = INODE_MODE_ALLOC | INODE_MODE_FDIR | 0777;
	inode.fs = fs;
	inode.number = 1;

	/* directory - put in extra links */
	memset (buf, 0, sizeof(buf));
	buf[0] = inode.number;
	buf[1] = inode.number >> 8;
	buf[2] = '.';
	buf[16] = inode.number;
	buf[17] = inode.number >> 8;
	buf[18] = '.';
	buf[19] = '.';
	inode.nlink = 2;
	inode.size = 32;

	if (! fs_block_alloc (fs, &bno))
		return 0;
	if (! fs_write_block (fs, bno, buf))
		return 0;
	inode.addr[0] = bno;

	time (&inode.atime);
	time (&inode.mtime);

	if (! fs_inode_save (&inode, 1))
		return 0;
	return 1;
}

int fs_create (fs_t *fs, const char *filename, unsigned long bytes)
{
	int n;
	unsigned char buf [BSDFS_BSIZE];

	memset (fs, 0, sizeof (*fs));
	fs->filename = filename;
	fs->seek = 0;

	fs->fd = open (fs->filename, O_CREAT | O_RDWR, 0666);
	if (fs->fd < 0)
		return 0;
	fs->writable = 1;

	/* get total disk size
	 * and inode block size */
	fs->fsize = bytes / BSDFS_BSIZE;
	fs->isize = (fs->fsize / 6 + 15) / 16;
	if (fs->isize < 1)
		return 0;

	/* make sure the file is of proper size - for SIMH */
	if (lseek(fs->fd, bytes-1, SEEK_SET) == bytes-1) {
		write(fs->fd, "", 1);
		lseek(fs->fd, 0, SEEK_SET);
	} else
		return 0;
	/* build a list of free blocks */
	fs_block_free (fs, 0);
	for (n = fs->fsize - 1; n >= fs->isize + 2; n--)
		if (! fs_block_free (fs, n))
			return 0;

	/* initialize inodes */
	memset (buf, 0, BSDFS_BSIZE);
	if (! fs_seek (fs, 1024))
		return 0;
	for (n=0; n < fs->isize; n++)
		if (! fs_write (fs, buf, BSDFS_BSIZE))
			return 0;

	/* root directory */
	if (! create_root_directory (fs))
		return 0;

	/* build a list of free inodes */
	if (! build_inode_list (fs))
		return 0;

	/* write out super block */
	return fs_sync (fs, 1);
}
