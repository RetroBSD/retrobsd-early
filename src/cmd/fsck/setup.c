/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifdef CROSS
#include </usr/include/stdio.h>
#else
#include <stdio.h>
#endif
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/swap.h>
#include "fsck.h"

int
setup(dev)
	char *dev;
{
	dev_t rootdev;
	off_t smapsz, totsz, lncntsz;
	daddr_t bcnt, nscrblk;
	struct stat statb;
	u_int msize;
	char *mbase;
	BUFAREA *bp;
    off_t allocd;

	if (stat("/", &statb) < 0)
		errexit("Can't stat root\n");
	rootdev = statb.st_dev;
	if (stat(dev, &statb) < 0) {
		printf("Can't stat %s\n", dev);
		return (0);
	}
	rawflg = 0;
	if ((statb.st_mode & S_IFMT) == S_IFBLK)
		;
	else if ((statb.st_mode & S_IFMT) == S_IFCHR)
		rawflg++;
	else {
#ifndef CROSS
		if (reply("file is not a block or character device; OK") == 0)
			return (0);
#endif
	}
	if (rootdev == statb.st_rdev)
		hotroot++;
	if ((dfile.rfdes = open(dev, 0)) < 0) {
		printf("Can't open %s\n", dev);
		return (0);
	}
	if (preen == 0)
		printf("** %s", dev);
	if (nflag || (dfile.wfdes = open(dev, 1)) < 0) {
		dfile.wfdes = -1;
		if (preen)
			pfatal("NO WRITE ACCESS\n");
		printf(" (NO WRITE)");
	}
	if (preen == 0)
		printf("\n");
	dfile.mod = 0;
	lfdir = 0;
	initbarea(&sblk);
	initbarea(&fileblk);
	initbarea(&inoblk);
	/*
	 * Read in the super block and its summary info.
	 */
	if (bread(&dfile, (char *)&sblock, SUPERB, SBSIZE) != 0)
		return (0);
	sblk.b_bno = SUPERB;
	sblk.b_size = SBSIZE;

	imax = ((ino_t)sblock.fs_isize - (SUPERB+1)) * INOPB;
	fsmin = (daddr_t)sblock.fs_isize;	/* first data blk num */
	fsmax = sblock.fs_fsize;		/* first invalid blk num */
	startib = fsmax;
	if (fsmin >= fsmax ||
		(imax/INOPB) != ((ino_t)sblock.fs_isize-(SUPERB+1))) {
		pfatal("Size check: fsize %ld isize %d\n",
			sblock.fs_fsize,sblock.fs_isize);
		printf("\n");
		ckfini();
		return(0);
	}
	if (preen == 0)
		printf("File System: %.12s\n\n", sblock.fs_fsmnt);
	/*
	 * allocate and initialize the necessary maps
	 */
	bmapsz = roundup (howmany (fsmax, BITSPB), sizeof (*lncntp));
	smapsz = roundup (howmany ((long) (imax+1), STATEPB), sizeof (*lncntp));
	lncntsz = (long) (imax+1) * sizeof (*lncntp);
	if (bmapsz > smapsz+lncntsz)
		smapsz = bmapsz-lncntsz;
	totsz = bmapsz+smapsz+lncntsz;
	msize = memsize;
	mbase = membase;
	bzero(mbase,msize);
	muldup = enddup = duplist;
	zlnp = zlnlist;

    totsz = fsmax / 3640 + 4;

	if ((off_t)msize < (totsz<<10)) {
		bmapsz = roundup(bmapsz,DEV_BSIZE);
		smapsz = roundup(smapsz,DEV_BSIZE);
		lncntsz = roundup(lncntsz,DEV_BSIZE);
		nscrblk = totsz; //(bmapsz+smapsz+lncntsz)>>DEV_BSHIFT;

        strcpy(scrfile,"/dev/temp0");
        sfile.wfdes = open(scrfile,1);
        if (!sfile.wfdes) {
            errexit("Unable to open temp device\n");
        }
        allocd = totsz;
        ioctl(sfile.wfdes, TFALLOC, &allocd);
        if (allocd != totsz) {
            printf("Unable to allocate temp space (wanted %lu, got %lu\n",
                totsz,allocd);
            errexit("alloc fail\n");
        }
        sfile.rfdes = open(scrfile,0);
            
        sfile.offset = 0;

		bp = &((BUFAREA *)mbase)[(msize/sizeof(BUFAREA))];
		poolhead = NULL;
		while(--bp >= (BUFAREA *)mbase) {
			initbarea(bp);
			bp->b_next = poolhead;
			poolhead = bp;
		}
		bp = poolhead;
		for(bcnt = 0; bcnt < nscrblk; bcnt++) {
			bp->b_bno = bcnt;
			dirty(bp);
			flush(&sfile,bp);
		}
		blockmap = freemap = statemap = (char *) NULL;
		lncntp = (short *) NULL;
		bmapblk = 0;
		smapblk = bmapblk + bmapsz / DEV_BSIZE;
		lncntblk = smapblk + smapsz / DEV_BSIZE;
		fmapblk = smapblk;
	} else {
		poolhead = NULL;
		blockmap = mbase;
		statemap = &mbase[bmapsz];
		freemap = statemap;
		lncntp = (short *)&statemap[smapsz];
	}
	return(1);
}
