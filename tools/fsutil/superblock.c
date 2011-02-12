/*
 * Superblock routines for unix v6 filesystem.
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

int fs_seek (fs_t *fs, unsigned long offset)
{
/*	printf ("seek %ld, block %ld\n", offset, offset / BSDFS_BSIZE);*/
	if (lseek (fs->fd, offset, 0) < 0) {
		if (verbose)
			printf ("error seeking %ld, block %ld\n",
				offset, offset / BSDFS_BSIZE);
		return 0;
	}
	fs->seek = offset;
	return 1;
}

int fs_read8 (fs_t *fs, unsigned char *val)
{
	if (read (fs->fd, val, 1) != 1) {
		if (verbose)
			printf ("error read8, seek %ld block %ld\n", fs->seek, fs->seek / BSDFS_BSIZE);
		return 0;
	}
	return 1;
}

int fs_read16 (fs_t *fs, unsigned short *val)
{
	unsigned char data [2];

	if (read (fs->fd, data, 2) != 2) {
		if (verbose)
			printf ("error read16, seek %ld block %ld\n", fs->seek, fs->seek / BSDFS_BSIZE);
		return 0;
	}
	*val = data[1] << 8 | data[0];
	return 1;
}

int fs_read32 (fs_t *fs, unsigned *val)
{
	unsigned char data [4];

	if (read (fs->fd, data, 4) != 4) {
		if (verbose)
			printf ("error read32, seek %ld block %ld\n", fs->seek, fs->seek / BSDFS_BSIZE);
		return 0;
	}
	*val = (unsigned long) data[0] | (unsigned long) data[1] << 8 |
		data[2] << 16 | data[3] << 24;
	return 1;
}

int fs_write8 (fs_t *fs, unsigned char val)
{
	if (write (fs->fd, &val, 1) != 1)
		return 0;
	return 1;
}

int fs_write16 (fs_t *fs, unsigned short val)
{
	unsigned char data [2];

	data[0] = val;
	data[1] = val >> 8;
	if (write (fs->fd, data, 2) != 2)
		return 0;
	return 1;
}

int fs_write32 (fs_t *fs, unsigned val)
{
	unsigned char data [4];

	data[0] = val;
	data[1] = val >> 8;
	data[2] = val >> 16;
	data[3] = val >> 24;
	if (write (fs->fd, data, 4) != 4)
		return 0;
	return 1;
}

int fs_read (fs_t *fs, unsigned char *data, int bytes)
{
	int len;

	while (bytes > 0) {
		len = bytes;
		if (len > 128)
			len = 128;
		if (read (fs->fd, data, len) != len)
			return 0;
		data += len;
		bytes -= len;
	}
	return 1;
}

int fs_write (fs_t *fs, unsigned char *data, int bytes)
{
	int len;

	if (! fs->writable)
		return 0;
	while (bytes > 0) {
		len = bytes;
		if (len > 128)
			len = 128;
		if (write (fs->fd, data, len) != len)
			return 0;
		data += len;
		bytes -= len;
	}
	return 1;
}

int fs_open (fs_t *fs, const char *filename, int writable)
{
	int i;
	unsigned magic;

	memset (fs, 0, sizeof (*fs));
	fs->filename = filename;
	fs->seek = 0;

	fs->fd = open (fs->filename, writable ? O_RDWR : O_RDONLY);
	if (fs->fd < 0)
		return 0;
	fs->writable = writable;

	if (! fs_read32 (fs, &magic) ||		/* magic word */
	    magic != FSMAGIC1) {
		if (verbose)
			printf ("fs_open: bad magic1 = %08x, expected %08x\n",
				magic, FSMAGIC1);
		return 0;
	}
	if (! fs_read32 (fs, &fs->isize))	/* size in blocks of I list */
		return 0;
	if (! fs_read32 (fs, &fs->fsize))	/* size in blocks of entire volume */
		return 0;
	if (! fs_read32 (fs, &fs->nfree))	/* number of in core free blocks (0-100) */
		return 0;
	for (i=0; i<NICFREE; ++i) {		/* in core free blocks */
		if (! fs_read32 (fs, &fs->free[i]))
			return 0;
	}
	if (! fs_read32 (fs, &fs->ninode))	/* number of in core I nodes (0-100) */
		return 0;
	for (i=0; i<NICINOD; ++i) {		/* in core free I nodes */
		if (! fs_read32 (fs, &fs->inode[i]))
			return 0;
	}
	if (! fs_read32 (fs, &magic))		/* flock, fmod, ilock, ronly */
		return 0;
	if (! fs_read32 (fs, (unsigned*) &fs->time))
		return 0;			/* current date of last update */
	if (! fs_read32 (fs, &fs->tfree))	/* total free blocks */
		return 0;
	if (! fs_read32 (fs, &fs->tinode))	/* total free inodes */
		return 0;
	if (! fs_read32 (fs, (unsigned*) &fs->step))
		return 0;			/* optimal step in free list pattern */
	if (! fs_read32 (fs, (unsigned*) &fs->cyl))
		return 0;			/* number of blocks per pattern */
	if (! fs_read (fs, (unsigned char*) fs->fsmnt, MAXMNTLEN))
		return 0;			/* ordinary file mounted on */
	if (! fs_read32 (fs, &fs->lasti))	/* start place for circular search */
		return 0;
	if (! fs_read32 (fs, &fs->nbehind))	/* est # free inodes before s_lasti */
		return 0;
	if (! fs_read32 (fs, &fs->flags))	/* mount time flags */
		return 0;
	if (! fs_read32 (fs, &magic) ||		/* magic word */
	    magic != FSMAGIC2)
		return 0;
	return 1;
}

int fs_sync (fs_t *fs, int force)
{
	int i;

	if (! fs->writable)
		return 0;
	if (! force && ! fs->dirty)
		return 1;

	time (&fs->time);
	if (! fs_seek (fs, 0))
		return 0;

	if (! fs_write32 (fs, FSMAGIC1))	/* magic word */
		return 0;
	if (! fs_write32 (fs, fs->isize))	/* size in blocks of I list */
		return 0;
	if (! fs_write32 (fs, fs->fsize))	/* size in blocks of entire volume */
		return 0;
	if (! fs_write32 (fs, fs->nfree))	/* number of in core free blocks (0-100) */
		return 0;
	for (i=0; i<100; ++i) {			/* in core free blocks */
		if (! fs_write32 (fs, fs->free[i]))
			return 0;
	}
	if (! fs_write32 (fs, fs->ninode))	/* number of in core I nodes (0-100) */
		return 0;
	for (i=0; i<100; ++i) {			/* in core free I nodes */
		if (! fs_write32 (fs, fs->inode[i]))
			return 0;
	}
	if (! fs_write32 (fs, 0))		/* flock, fmod, ilock, ronly */
		return 0;
	if (! fs_write32 (fs, fs->time))	/* current date of last update */
		return 0;
	if (! fs_write32 (fs, fs->tfree))	/* total free blocks */
		return 0;
	if (! fs_write32 (fs, fs->tinode))	/* total free inodes */
		return 0;
	if (! fs_write32 (fs, fs->step))
		return 0;			/* optimal step in free list pattern */
	if (! fs_write32 (fs, fs->cyl))
		return 0;			/* number of blocks per pattern */
	if (! fs_write (fs, (unsigned char*) fs->fsmnt, MAXMNTLEN))
		return 0;			/* ordinary file mounted on */
	if (! fs_write32 (fs, 0))		/* lasti*/
		return 0;
	if (! fs_write32 (fs, 0))		/* nbehind */
		return 0;
	if (! fs_write32 (fs, 0))		/* flags */
		return 0;
	if (! fs_write32 (fs, FSMAGIC2))	/* magic word */
		return 0;
	fs->dirty = 0;
	return 1;
}

void fs_print (fs_t *fs, FILE *out)
{
	int i;

	fprintf (out, "                File: %s\n", fs->filename);
	fprintf (out, "         Volume size: %u blocks\n", fs->fsize);
	fprintf (out, "     Inode list size: %u blocks\n", fs->isize);
	fprintf (out, "   In-core free list: %u blocks\n", fs->nfree);
	fprintf (out, "   Total free blocks: %u blocks\n", fs->tfree);
	fprintf (out, "   Total free inodes: %u inodes\n", fs->tinode);
	fprintf (out, "     Last mounted on: %.*s\n", MAXMNTLEN,
		fs->fsmnt[0] ? fs->fsmnt : "(none)");
	if (verbose)
	    for (i=0; i < 100 && i < fs->nfree; ++i) {
		if (i % 10 == 0)
			fprintf (out, "\n                     ");
		fprintf (out, " %u", fs->free[i]);
	    }
	fprintf (out, "\n");

	fprintf (out, " In-core free inodes: %u inodes", fs->ninode);
	if (verbose)
	    for (i=0; i < 100 && i < fs->nfree; ++i) {
		if (i % 10 == 0)
			fprintf (out, "\n                     ");
		fprintf (out, " %u", fs->inode[i]);
	    }
	fprintf (out, "\n");
	if (verbose) {
//		fprintf (out, "      Free list lock: %u\n", fs->flock);
//		fprintf (out, "     Inode list lock: %u\n", fs->ilock);
//		fprintf (out, "Super block modified: %u\n", fs->fmod);
//		fprintf (out, "   Mounted read-only: %u\n", fs->ronly);
		fprintf (out, "   Step in free list: %u\n", fs->step);
		fprintf (out, "  Blocks per pattern: %u\n", fs->cyl);
//		fprintf (out, "   Circ.search start: %u\n", fs->lasti);
//		fprintf (out, "  Circ.search behind: %u\n", fs->nbehind);
//		fprintf (out, "         Mount flags: 0x%x\n", fs->flags);
	}

	fprintf (out, "    Last update time: %s", ctime (&fs->time));
}

void fs_close (fs_t *fs)
{
	if (fs->fd < 0)
		return;

	close (fs->fd);
	fs->fd = -1;
}
