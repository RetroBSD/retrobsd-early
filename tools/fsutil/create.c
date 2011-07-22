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

	total_inodes = (fs->isize - 1) * BSDFS_INODES_PER_BLOCK;
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

static int create_inode1 (fs_t *fs)
{
	fs_inode_t inode;

	memset (&inode, 0, sizeof(inode));
	inode.mode = INODE_MODE_FREG;
	inode.fs = fs;
	inode.number = 1;
	if (! fs_inode_save (&inode, 1))
		return 0;
	fs->tinode--;
	return 1;
}

static int create_root_directory (fs_t *fs)
{
	fs_inode_t inode;
	unsigned char buf [BSDFS_BSIZE];
	unsigned int bno;

	memset (&inode, 0, sizeof(inode));
	inode.mode = INODE_MODE_FDIR | 0777;
	inode.fs = fs;
	inode.number = BSDFS_ROOT_INODE;
	inode.size = BSDFS_BSIZE;
	inode.flags = 0;

	time (&inode.ctime);
	time (&inode.atime);
	time (&inode.mtime);

	/* directory - put in extra links */
	memset (buf, 0, sizeof(buf));
	buf[0] = inode.number;
	buf[1] = inode.number >> 8;
	buf[2] = inode.number >> 16;
	buf[3] = inode.number >> 24;
	buf[4] = 12;
	buf[5] = 12 >> 8;
	buf[6] = 1;
	buf[7] = 1 >> 8;
	buf[8] = '.';
	buf[9] = 0;
	buf[10] = 0;
	buf[11] = 0;
	buf[12+0] = BSDFS_ROOT_INODE;
	buf[12+1] = BSDFS_ROOT_INODE >> 8;
	buf[12+2] = BSDFS_ROOT_INODE >> 16;
	buf[12+3] = BSDFS_ROOT_INODE >> 24;
	buf[12+4] = 12;
	buf[12+5] = 12 >> 8;
	buf[12+6] = 2;
	buf[12+7] = 2 >> 8;
	buf[12+8] = '.';
	buf[12+9] = '.';
	buf[12+10] = 0;
	buf[12+11] = 0;
	buf[24+0] = BSDFS_LOSTFOUND_INODE;
	buf[24+1] = BSDFS_LOSTFOUND_INODE >> 8;
	buf[24+2] = BSDFS_LOSTFOUND_INODE >> 16;
	buf[24+3] = BSDFS_LOSTFOUND_INODE >> 24;
	buf[24+4] = (unsigned char) (BSDFS_BSIZE - 12 - 12);
	buf[24+5] = (BSDFS_BSIZE - 12 - 12) >> 8;
	buf[24+6] = 10;
	buf[24+7] = 10 >> 8;
	memcpy (&buf[24+8], "lost+found\0\0\0\0", 12);

	inode.nlink = 3;

	if (! fs_block_alloc (fs, &bno))
		return 0;
	if (! fs_write_block (fs, bno, buf))
		return 0;
	inode.addr[0] = bno;

	if (! fs_inode_save (&inode, 1))
		return 0;
	fs->tinode--;
	return 1;
}

static int create_lost_found_directory (fs_t *fs)
{
	fs_inode_t inode;
	unsigned char buf [BSDFS_BSIZE];
	unsigned int bno;

	memset (&inode, 0, sizeof(inode));
	inode.mode = INODE_MODE_FDIR | 0777;
	inode.fs = fs;
	inode.number = BSDFS_LOSTFOUND_INODE;
	inode.size = BSDFS_BSIZE;
	inode.flags = 0;

	time (&inode.ctime);
	time (&inode.atime);
	time (&inode.mtime);

	/* directory - put in extra links */
	memset (buf, 0, sizeof(buf));
	buf[0] = inode.number;
	buf[1] = inode.number >> 8;
	buf[2] = inode.number >> 16;
	buf[3] = inode.number >> 24;
	buf[4] = 12;
	buf[5] = 12 >> 8;
	buf[6] = 1;
	buf[7] = 1 >> 8;
	buf[8] = '.';
	buf[9] = 0;
	buf[10] = 0;
	buf[11] = 0;
	buf[12+0] = BSDFS_ROOT_INODE;
	buf[12+1] = BSDFS_ROOT_INODE >> 8;
	buf[12+2] = BSDFS_ROOT_INODE >> 16;
	buf[12+3] = BSDFS_ROOT_INODE >> 24;
	buf[12+4] = (unsigned char) (BSDFS_BSIZE - 12);
	buf[12+5] = (BSDFS_BSIZE - 12) >> 8;
	buf[12+6] = 2;
	buf[12+7] = 2 >> 8;
	buf[12+8] = '.';
	buf[12+9] = '.';
	buf[12+10] = 0;
	buf[12+11] = 0;

	inode.nlink = 2;

	if (! fs_block_alloc (fs, &bno))
		return 0;
	if (! fs_write_block (fs, bno, buf))
		return 0;
	inode.addr[0] = bno;

	if (! fs_inode_save (&inode, 1))
		return 0;
	fs->tinode--;
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
	fs->isize = 1 + (fs->fsize / 4 + BSDFS_INODES_PER_BLOCK - 1) /
		BSDFS_INODES_PER_BLOCK;
	if (fs->isize < 2)
		return 0;

	/* make sure the file is of proper size */
	if (lseek (fs->fd, bytes-1, SEEK_SET) != bytes-1 ||
	    write (fs->fd, "", 1) != 1) {
		return 0;
	}
	lseek (fs->fd, 0, SEEK_SET);

	/* build a list of free blocks */
	fs_block_free (fs, 0);
	for (n = fs->fsize - 1; n >= fs->isize; n--)
		if (! fs_block_free (fs, n))
			return 0;

	/* initialize inodes */
	memset (buf, 0, BSDFS_BSIZE);
	if (! fs_seek (fs, BSDFS_BSIZE))
		return 0;
	for (n=1; n < fs->isize; n++) {
		if (! fs_write (fs, buf, BSDFS_BSIZE))
			return 0;
		fs->tinode += BSDFS_INODES_PER_BLOCK;
	}

	/* legacy empty inode 1 */
	if (! create_inode1 (fs))
		return 0;

	/* lost+found directory */
	if (! create_lost_found_directory (fs))
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
