/*
 * Block handling for 2.xBSD filesystem.
 *
 * Copyright (C) 2006-2011 Serge Vakulenko, <serge@vak.ru>
 *
 * This file is part of RetroBSD project, which is distributed
 * under the terms of the GNU General Public License (GPL).
 * See the accompanying file "COPYING" for more details.
 */
#include <stdio.h>
#include "bsdfs.h"

extern int verbose;

int fs_read_block (fs_t *fs, unsigned bnum, unsigned char *data)
{
        if (verbose > 3)
                printf ("read block %d\n", bnum);
	if (bnum < fs->isize)
		return 0;
	if (! fs_seek (fs, bnum * BSDFS_BSIZE))
		return 0;
	if (! fs_read (fs, data, BSDFS_BSIZE))
		return 0;
	return 1;
}

int fs_write_block (fs_t *fs, unsigned bnum, unsigned char *data)
{
        if (verbose > 3)
                printf ("write block %d\n", bnum);
	if (! fs->writable || bnum < fs->isize)
		return 0;
	if (! fs_seek (fs, bnum * BSDFS_BSIZE))
		return 0;
	if (! fs_write (fs, data, BSDFS_BSIZE))
		return 0;
	fs->modified = 1;
	return 1;
}

/*
 * Add a block to free list.
 */
int fs_block_free (fs_t *fs, unsigned int bno)
{
	int i;
	unsigned buf [BSDFS_BSIZE / 4];

        if (verbose > 1)
                printf ("free block %d, total %d\n", bno, fs->nfree);
	if (fs->nfree >= NICFREE) {
		buf[0] = fs->nfree;
		for (i=0; i<NICFREE; i++)
			buf[i+1] = fs->free[i];
		if (! fs_write_block (fs, bno, (char*) buf)) {
			fprintf (stderr, "block_free: write error at block %d\n", bno);
			return 0;
		}
		fs->nfree = 0;
	}
	fs->free [fs->nfree] = bno;
	fs->nfree++;
	fs->dirty = 1;
	if (bno)			/* Count total free blocks. */
		++fs->tfree;
	return 1;
}

/*
 * Free an indirect block.
 */
int fs_indirect_block_free (fs_t *fs, unsigned int bno)
{
	unsigned nb;
	unsigned char data [BSDFS_BSIZE];
	int i;

	if (! fs_read_block (fs, bno, data)) {
		fprintf (stderr, "inode_clear: read error at block %d\n", bno);
		return 0;
	}
	for (i=BSDFS_BSIZE-2; i>=0; i-=2) {
		nb = data [i+1] << 8 | data [i];
		if (nb)
			fs_block_free (fs, nb);
	}
	fs_block_free (fs, bno);
	return 1;
}

/*
 * Free a double indirect block.
 */
int fs_double_indirect_block_free (fs_t *fs, unsigned int bno)
{
	unsigned nb;
	unsigned char data [BSDFS_BSIZE];
	int i;

	if (! fs_read_block (fs, bno, data)) {
		fprintf (stderr, "inode_clear: read error at block %d\n", bno);
		return 0;
	}
	for (i=BSDFS_BSIZE-2; i>=0; i-=2) {
		nb = data [i+1] << 8 | data [i];
		if (nb)
			fs_indirect_block_free (fs, nb);
	}
	fs_block_free (fs, bno);
	return 1;
}

/*
 * Free a triple indirect block.
 */
int fs_triple_indirect_block_free (fs_t *fs, unsigned int bno)
{
	unsigned nb;
	unsigned char data [BSDFS_BSIZE];
	int i;

	if (! fs_read_block (fs, bno, data)) {
		fprintf (stderr, "inode_clear: read error at block %d\n", bno);
		return 0;
	}
	for (i=BSDFS_BSIZE-2; i>=0; i-=2) {
		nb = data [i+1] << 8 | data [i];
		if (nb)
			fs_double_indirect_block_free (fs, nb);
	}
	fs_block_free (fs, bno);
	return 1;
}

/*
 * Get a block from free list.
 */
int fs_block_alloc (fs_t *fs, unsigned int *bno)
{
	int i;
	unsigned buf [BSDFS_BSIZE / 4];
again:
	if (fs->nfree == 0)
		return 0;
	fs->nfree--;
	--fs->tfree;			/* Count total free blocks. */
	*bno = fs->free [fs->nfree];
        if (verbose)
                printf ("allocate new block %d from slot %d\n", *bno, fs->nfree);
	fs->free [fs->nfree] = 0;
	fs->dirty = 1;
	if (fs->nfree <= 0) {
		if (! fs_read_block (fs, *bno, (char*) buf))
			return 0;
		fs->nfree = buf[0];
		for (i=0; i<NICFREE; i++)
			fs->free[i] = buf[i+1];
	}
	if (*bno == 0)
		goto again;
	return 1;
}
