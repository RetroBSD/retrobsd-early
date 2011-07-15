/*
 * Inode routines for unix v6 filesystem.
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

int fs_inode_get (fs_t *fs, fs_inode_t *inode, unsigned inum)
{
	unsigned long offset;
	int i, reserved;

	memset (inode, 0, sizeof (*inode));
	inode->fs = fs;
	inode->number = inum;

	/* Inodes are numbered starting from 1.
	 * 64 bytes per inode, 16 inodes per block.
	 * Skip first block. */
	if (inum == 0 || inum > (fs->isize-1) * BSDFS_INODES_PER_BLOCK)
		return 0;
	offset = (inode->number + BSDFS_INODES_PER_BLOCK - 1) *
		BSDFS_BSIZE / BSDFS_INODES_PER_BLOCK;

	if (! fs_seek (fs, offset))
		return 0;

	if (! fs_read16 (fs, &inode->mode))	/* file type and access mode */
		return 0;
	if (! fs_read16 (fs, &inode->nlink))	/* directory entries */
		return 0;
	if (! fs_read32 (fs, &inode->uid))	/* owner */
		return 0;
	if (! fs_read32 (fs, &inode->gid))	/* group */
		return 0;
	if (! fs_read32 (fs, (unsigned*) &inode->size))	/* size */
		return 0;

	for (i=0; i<NADDR; ++i) {		/* device addresses constituting file */
		if (! fs_read32 (fs, &inode->addr[i]))
			return 0;
	}
	if (! fs_read32 (fs, (unsigned*) &reserved))
		return 0;
	if (! fs_read32 (fs, (unsigned*) &inode->flags))
		return 0;			/* inode flags */
	if (! fs_read32 (fs, (unsigned*) &inode->atime))
		return 0;			/* last access time */
	if (! fs_read32 (fs, (unsigned*) &inode->mtime))
		return 0;			/* last modification time */
	if (! fs_read32 (fs, (unsigned*) &inode->ctime))
		return 0;			/* creation time */
/*if (inode->mode) { fs_inode_print (inode, stdout); printf ("---\n"); }*/
	return 1;
}

/*
 * Free all the disk blocks associated
 * with the specified inode structure.
 * The blocks of the file are removed
 * in reverse order. This FILO
 * algorithm will tend to maintain
 * a contiguous free list much longer
 * than FIFO.
 */
void fs_inode_truncate (fs_inode_t *inode)
{
	unsigned *blk;

	if ((inode->mode & INODE_MODE_FMT) == INODE_MODE_FCHR ||
	    (inode->mode & INODE_MODE_FMT) == INODE_MODE_FBLK)
		return;

#define	SINGLE	4	/* index of single indirect block */
#define	DOUBLE	5	/* index of double indirect block */
#define	TRIPLE	6	/* index of triple indirect block */

	for (blk = &inode->addr[TRIPLE]; blk >= &inode->addr[0]; --blk) {
		if (*blk == 0)
			continue;

		if (blk == &inode->addr [TRIPLE])
			fs_triple_indirect_block_free (inode->fs, *blk);
		else if (blk == &inode->addr [DOUBLE])
			fs_double_indirect_block_free (inode->fs, *blk);
		else if (blk == &inode->addr [SINGLE])
			fs_indirect_block_free (inode->fs, *blk);
		else
			fs_block_free (inode->fs, *blk);

		*blk = 0;
	}

	inode->size = 0;
	inode->dirty = 1;
}

void fs_inode_clear (fs_inode_t *inode)
{
	inode->dirty = 1;
	inode->mode = 0;
	inode->nlink = 0;
	inode->uid = 0;
	inode->size = 0;
	memset (inode->addr, 0, sizeof(inode->addr));
	inode->atime = 0;
	inode->mtime = 0;
}

int fs_inode_save (fs_inode_t *inode, int force)
{
	unsigned long offset;
	int i;

	if (! inode->fs->writable)
		return 0;
	if (! force && ! inode->dirty)
		return 1;
	if (inode->number == 0 ||
	    inode->number > (inode->fs->isize-1) * BSDFS_INODES_PER_BLOCK)
		return 0;
	offset = (inode->number + BSDFS_INODES_PER_BLOCK - 1) *
		BSDFS_BSIZE / BSDFS_INODES_PER_BLOCK;

	time (&inode->atime);
	time (&inode->mtime);

	if (! fs_seek (inode->fs, offset))
		return 0;

	if (! fs_write16 (inode->fs, inode->mode))	/* file type and access mode */
		return 0;
	if (! fs_write16 (inode->fs, inode->nlink))	/* directory entries */
		return 0;
	if (! fs_write32 (inode->fs, inode->uid))	/* owner */
		return 0;
	if (! fs_write32 (inode->fs, inode->gid))	/* group */
		return 0;
	if (! fs_write32 (inode->fs, inode->size))	/* size */
		return 0;

	for (i=0; i<NADDR; ++i) {	/* device addresses constituting file */
		if (! fs_write32 (inode->fs, inode->addr[i]))
			return 0;
	}
	if (! fs_write32 (inode->fs, 0))		/* reserved */
		return 0;
	if (! fs_write32 (inode->fs, inode->atime))	/* last access time */
		return 0;
	if (! fs_write32 (inode->fs, inode->mtime))	/* last modification time */
		return 0;
	if (! fs_write32 (inode->fs, inode->ctime))	/* creation time */
		return 0;

	inode->dirty = 0;
	return 1;
}

void fs_inode_print (fs_inode_t *inode, FILE *out)
{
	int i;

	fprintf (out, "     I-node: %u\n", inode->number);
	fprintf (out, "       Type: %s\n",
		(inode->mode & INODE_MODE_FMT) == INODE_MODE_FDIR ? "Directory" :
		(inode->mode & INODE_MODE_FMT) == INODE_MODE_FCHR ? "Character device" :
		(inode->mode & INODE_MODE_FMT) == INODE_MODE_FBLK ? "Block device" :
		(inode->mode & INODE_MODE_FMT) == INODE_MODE_FREG ? "File" :
		(inode->mode & INODE_MODE_FMT) == INODE_MODE_FLNK ? "Symbolic link" :
		(inode->mode & INODE_MODE_FMT) == INODE_MODE_FSOCK? "Socket" :
		"Unknown");
	fprintf (out, "       Size: %lu bytes\n", inode->size);
	fprintf (out, "       Mode: %#o\n", inode->mode);

	fprintf (out, "            ");
        if (inode->mode & INODE_MODE_SUID)  fprintf (out, " SUID");
        if (inode->mode & INODE_MODE_SGID)  fprintf (out, " SGID");
        if (inode->mode & INODE_MODE_SVTX)  fprintf (out, " SVTX");
        if (inode->mode & INODE_MODE_READ)  fprintf (out, " READ");
        if (inode->mode & INODE_MODE_WRITE) fprintf (out, " WRITE");
        if (inode->mode & INODE_MODE_EXEC)  fprintf (out, " EXEC");
	fprintf (out, "\n");

	fprintf (out, "      Links: %u\n", inode->nlink);
	fprintf (out, "   Owner id: %u\n", inode->uid);

	fprintf (out, "     Blocks:");
	for (i=0; i<NADDR; ++i) {
		fprintf (out, " %u", inode->addr[i]);
	}
	fprintf (out, "\n");

	fprintf (out, "Last access: %s", ctime (&inode->atime));
	fprintf (out, "   Modified: %s", ctime (&inode->mtime));
}

void fs_directory_scan (fs_inode_t *dir, char *dirname,
	fs_directory_scanner_t scanner, void *arg)
{
	fs_inode_t file;
	unsigned long offset;
	unsigned char name [BSDFS_BSIZE - 12];
	struct {
            unsigned int inum;
            unsigned short reclen;
            unsigned short namlen;
        } dirent;

	/* Variable record per file */
	for (offset = 0; offset < dir->size; offset += dirent.reclen) {
		if (! fs_inode_read (dir, offset, (unsigned char*) &dirent, sizeof(dirent))) {
			fprintf (stderr, "%s: read error at offset %ld\n",
				dirname[0] ? dirname : "/", offset);
			return;
		}
/*printf ("scan offset %lu: inum=%u, reclen=%u, namlen=%u\n", offset, dirent.inum, dirent.reclen, dirent.namlen);*/
		if (! fs_inode_read (dir, offset+sizeof(dirent), name, (dirent.namlen + 3) / 4 * 4)) {
			fprintf (stderr, "%s: name read error at offset %ld\n",
				dirname[0] ? dirname : "/", offset);
			return;
		}
/*printf ("scan offset %lu: name='%s'\n", offset, name);*/

		if (dirent.inum == 0 || (name[0]=='.' && name[1]==0) ||
		    (name[0]=='.' && name[1]=='.' && name[2]==0))
			continue;

		if (! fs_inode_get (dir->fs, &file, dirent.inum)) {
			fprintf (stderr, "cannot scan inode %d\n", dirent.inum);
			continue;
		}
		scanner (dir, &file, dirname, (char*) name, arg);
	}
}

/*
 * Return the physical block number on a device given the
 * inode and the logical block number in a file.
 */
static unsigned map_block (fs_inode_t *inode, unsigned lbn)
{
	unsigned block [BSDFS_BSIZE / 4];
	unsigned int nb, i, j, sh;

	/*
	 * Blocks 0..NADDR-4 are direct blocks.
	 */
	if (lbn < NADDR-3) {
		/* small file algorithm */
		return inode->addr [lbn];
	}

	/*
	 * Addresses NADDR-3, NADDR-2, and NADDR-1
	 * have single, double, triple indirect blocks.
	 * The first step is to determine
	 * how many levels of indirection.
	 */
	sh = 0;
	nb = 1;
	lbn -= NADDR-3;
	for (j=3; ; j--) {
		if (j == 0)
			return 0;
		sh += NSHIFT;
		nb <<= NSHIFT;
		if (lbn < nb)
			break;
		lbn -= nb;
	}

	/*
	 * Fetch the first indirect block.
	 */
	nb = inode->addr [NADDR-j];
	if (nb == 0)
		return 0;

	/*
	 * Fetch through the indirect blocks.
	 */
	for(; j <= 3; j++) {
		if (! fs_read_block (inode->fs, nb, (unsigned char*) block))
			return 0;

		sh -= NSHIFT;
		i = (lbn >> sh) & NMASK;
		nb = block [i];
		if (nb == 0)
			return 0;
	}
	return nb;
}

/*
 * Bmap defines the structure of file system storage
 * by returning the physical block number on a device given the
 * inode and the logical block number in a file.
 */
static unsigned map_block_write (fs_inode_t *inode, unsigned lbn)
{
	unsigned char block [BSDFS_BSIZE];
	unsigned int nb, newb, sh, i, j;

	/*
	 * Blocks 0..NADDR-4 are direct blocks.
	 */
	if (lbn < NADDR-3) {
		/* small file algorithm */
		nb = inode->addr [lbn];
		if (nb != 0) {
/*			printf ("map logical block %d to physical %d\n", lbn, nb);*/
			return nb;
		}

		/* allocate new block */
		if (! fs_block_alloc (inode->fs, &nb))
			return 0;
		inode->addr[lbn] = nb;
		inode->dirty = 1;
		return nb;
	}

	/*
	 * Addresses NADDR-3, NADDR-2, and NADDR-1
	 * have single, double, triple indirect blocks.
	 * The first step is to determine
	 * how many levels of indirection.
	 */
	sh = 0;
	nb = 1;
	lbn -= NADDR-3;
	for (j=3; ; j--) {
		if (j == 0)
			return 0;
		sh += NSHIFT;
		nb <<= NSHIFT;
		if (lbn < nb)
			break;
		lbn -= nb;
	}

	/*
	 * Fetch the first indirect block.
	 */
	nb = inode->addr [NADDR-j];
	if (nb == 0) {
		if (! fs_block_alloc (inode->fs, &nb))
			return 0;
/*		printf ("inode %d: allocate new block %d\n", inode->number, nb);*/
		memset (block, 0, BSDFS_BSIZE);
		if (! fs_write_block (inode->fs, nb, block))
			return 0;
		inode->addr [NADDR-j] = nb;
		inode->dirty = 1;
	}

	/*
	 * Fetch through the indirect blocks
	 */
	for(; j <= 3; j++) {
		if (! fs_read_block (inode->fs, nb, block))
			return 0;

		sh -= NSHIFT;
		i = (lbn >> sh) & NMASK;
		nb = block [i];
		if (nb == 0) {
			/* Allocate new block. */
			if (! fs_block_alloc (inode->fs, &newb))
				return 0;
/*			printf ("inode %d: allocate new block %d\n", inode->number, newb);*/
			block[i] = newb;
			if (! fs_write_block (inode->fs, nb, block))
				return 0;
			memset (block, 0, BSDFS_BSIZE);
			if (! fs_write_block (inode->fs, newb, block))
				return 0;
			nb = newb;
		}
	}
	return nb;
}

int fs_inode_read (fs_inode_t *inode, unsigned long offset,
	unsigned char *data, unsigned long bytes)
{
	unsigned char block [BSDFS_BSIZE];
	unsigned long n;
	unsigned int bn, inblock_offset;

	if (bytes + offset > inode->size)
		return 0;
	while (bytes != 0) {
		inblock_offset = offset % BSDFS_BSIZE;
		n = BSDFS_BSIZE - inblock_offset;
		if (n > bytes)
			n = bytes;

		bn = map_block (inode, offset / BSDFS_BSIZE);
		if (bn == 0)
			return 0;

		if (! fs_read_block (inode->fs, bn, block))
			return 0;
		memcpy (data, block + inblock_offset, n);
		offset += n;
		bytes -= n;
	}
	return 1;
}

int fs_inode_write (fs_inode_t *inode, unsigned long offset,
	unsigned char *data, unsigned long bytes)
{
	unsigned char block [BSDFS_BSIZE];
	unsigned long n;
	unsigned int bn, inblock_offset;

	while (bytes != 0) {
		inblock_offset = offset % BSDFS_BSIZE;
		n = BSDFS_BSIZE - inblock_offset;
		if (n > bytes)
			n = bytes;

		bn = map_block_write (inode, offset / BSDFS_BSIZE);
		if (bn == 0)
			return 0;
		if (inode->size < offset + n) {
			/* Increase file size. */
			inode->size = offset + n;
			inode->dirty = 1;
		}
		if (verbose)
			printf ("inode %d offset %ld: write %ld bytes to block %d\n",
				inode->number, offset, n, bn);

		if (n == BSDFS_BSIZE) {
			if (! fs_write_block (inode->fs, bn, data))
				return 0;
		} else {
			if (! fs_read_block (inode->fs, bn, block))
				return 0;
			memcpy (block + inblock_offset, data, n);
			if (! fs_write_block (inode->fs, bn, block))
				return 0;
		}
		offset += n;
		bytes -= n;
	}
	return 1;
}

/*
 * Convert from dirent to raw data.
 */
void fs_dirent_pack (unsigned char *data, fs_dirent_t *dirent)
{
	int i;

	*data++ = dirent->ino;
	*data++ = dirent->ino >> 8;
	*data++ = dirent->ino >> 16;
	*data++ = dirent->ino >> 24;
	*data++ = dirent->reclen;
	*data++ = dirent->reclen >> 8;
	*data++ = dirent->namlen;
	*data++ = dirent->namlen >> 8;
	for (i=0; dirent->name[i]; ++i)
		*data++ = dirent->name[i];
	for (; i & 3; ++i)
		*data++ = 0;
}

/*
 * Read dirent from raw data.
 */
void fs_dirent_unpack (fs_dirent_t *dirent, unsigned char *data)
{
	dirent->ino = *data++;
	dirent->ino |= *data++ << 8;
	dirent->ino |= *data++ << 16;
	dirent->ino |= *data++ << 24;
	dirent->reclen = *data++;
	dirent->reclen |= *data++ << 8;
	dirent->namlen = *data++;
	dirent->namlen |= *data++ << 8;
	memset (dirent->name, 0, sizeof (dirent->name));
	memcpy (dirent->name, data, dirent->namlen);
}

/*
 * Convert a pathname into a pointer to
 * an inode. Note that the inode is locked.
 *
 * flag = 0 if name is saught
 *	1 if name is to be created, mode is given
 *	2 if name is to be deleted
 *	3 if name is to be linked, mode contains inode number
 */
int fs_inode_by_name (fs_t *fs, fs_inode_t *inode, char *name,
	int op, int mode)
{
	int c;
	char *cp;
	char dbuf [14];
	unsigned long offset;
	unsigned char data [16];
	unsigned int inum;
	fs_inode_t dir;

	/* Start from root. */
	if (! fs_inode_get (fs, &dir, BSDFS_ROOT_INODE)) {
		fprintf (stderr, "inode_open(): cannot get root\n");
		return 0;
	}
	c = *name++;
	while (c == '/')
		c = *name++;
	if (! c && op != 0) {
		/* Cannot write or delete root directory. */
		return 0;
	}
cloop:
	/* Here inode contains pointer
	 * to last component matched. */
	if (! c) {
		*inode = dir;
		return 1;
	}

	/* If there is another component,
	 * inode must be a directory. */
	if ((dir.mode & INODE_MODE_FMT) != INODE_MODE_FDIR) {
		return 0;
	}

	/* Gather up dir name into buffer. */
	cp = &dbuf[0];
	while (c && c != '/') {
		if (cp < dbuf + sizeof(dbuf))
			*cp++ = c;
		c = *name++;
	}
	while (cp < dbuf + sizeof(dbuf))
		*cp++ = 0;
	while (c == '/')
		c = *name++;

	/* Search a directory, 16 bytes per file */
	for (offset = 0; dir.size - offset >= 16; offset += 16) {
		if (! fs_inode_read (&dir, offset, data, 16)) {
			fprintf (stderr, "inode %d: read error at offset %ld\n",
				dir.number, offset);
			return 0;
		}
		inum = data [1] << 8 | data [0];
		if (inum == 0)
			continue;
		if (strncmp (dbuf, (char*) data+2, 14) == 0) {
			/* Here a component matched in a directory.
			 * If there is more pathname, go back to
			 * cloop, otherwise return. */
			if (op == 2 && ! c) {
				goto delete_file;
			}
			if (! fs_inode_get (fs, &dir, inum)) {
				fprintf (stderr, "inode_open(): cannot get inode %d\n", inum);
				return 0;
			}
			goto cloop;
		}
	}
	/* If at the end of the directory,
	 * the search failed. Report what
	 * is appropriate as per flag. */
	if (op == 1 && ! c)
		goto create_file;
	if (op == 3 && ! c)
		goto create_link;
	return 0;

	/*
	 * Make a new file, and return it's inode.
	 */
create_file:
	if (! fs_inode_alloc (fs, inode)) {
		fprintf (stderr, "%s: cannot allocate inode\n", name);
		return 0;
	}
	inode->dirty = 1;
	inode->mode = mode & (07777 | INODE_MODE_FMT);
	if (inode->mode == 0)
		inode->mode = INODE_MODE_FREG;
	inode->nlink = 1;
	inode->uid = 0;
	if (! fs_inode_save (inode, 0)) {
		fprintf (stderr, "%s: cannot save file inode\n", name);
		return 0;
	}

	/* Write a directory entry. */
	data[0] = inode->number;
	data[1] = inode->number >> 8;
	memcpy (data+2, dbuf, 14);
write_back:
	if (! fs_inode_write (&dir, offset, data, 16)) {
		fprintf (stderr, "inode %d: write error at offset %ld\n",
			inode->number, offset);
		return 0;
	}
	if (! fs_inode_save (&dir, 0)) {
		fprintf (stderr, "%s: cannot save directory inode\n", name);
		return 0;
	}
	return 1;

	/*
	 * Delete file. Return inode of deleted file.
	 */
delete_file:
	if (! fs_inode_get (fs, inode, inum)) {
		fprintf (stderr, "%s: cannot get inode %d\n", name, inum);
		return 0;
	}
	inode->dirty = 1;
	inode->nlink--;
	if (inode->nlink <= 0) {
		fs_inode_truncate (inode);
		fs_inode_clear (inode);
		if (inode->fs->ninode < 100) {
			inode->fs->inode [inode->fs->ninode++] = inum;
			inode->fs->dirty = 1;
		}
	}
	memset (data, 0, 16);
	goto write_back;

	/*
	 * Make a link. Return a directory inode.
	 */
create_link:
	data[0] = mode;
	data[1] = mode >> 8;
/*printf ("*** link inode %d to %s\n", mode, dbuf);*/
	memcpy (data+2, dbuf, 14);
/*printf ("*** add entry '%.14s' to inode %d\n", dbuf, dir.number);*/
	if (! fs_inode_write (&dir, offset, data, 16)) {
		fprintf (stderr, "inode %d: write error at offset %ld\n",
			dir.number, offset);
		return 0;
	}
	if (! fs_inode_save (&dir, 0)) {
		fprintf (stderr, "%s: cannot save directory inode\n", name);
		return 0;
	}
	*inode = dir;
	return 1;
}

/*
 * Allocate an unused I node
 * on the specified device.
 * Used with file creation.
 * The algorithm keeps up to
 * 100 spare I nodes in the
 * super block. When this runs out,
 * a linear search through the
 * I list is instituted to pick
 * up 100 more.
 */
int fs_inode_alloc (fs_t *fs, fs_inode_t *inode)
{
	int ino;

	for (;;) {
		if (fs->ninode <= 0) {
			return 0;
		}
		ino = fs->inode[--fs->ninode];
		fs->dirty = 1;
		if (! fs_inode_get (fs, inode, ino)) {
			fprintf (stderr, "inode_alloc: cannot get inode %d\n", ino);
			return 0;
		}
		if (inode->mode == 0) {
			fs_inode_clear (inode);
			return 1;
		}
	}
}
