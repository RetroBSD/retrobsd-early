/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "conf.h"
#include "buf.h"
#include "time.h"
#include "ioctl.h"
#include "resource.h"
#include "inode.h"
#include "proc.h"
#include "clist.h"
#include "tty.h"

dev_t	rootdev = makedev(6,0),
	swapdev = makedev(6,1),
	pipedev = makedev(6,0);

dev_t	dumpdev = NODEV;
daddr_t	dumplo = (daddr_t)512;
int	nulldev();
int	(*dump)() = nulldev;

int	nulldev();
int	nodev();
int	rawrw();

#if NRK > 0
int	rkopen(), rkstrategy();
daddr_t	rksize();
#define	rkclose		nulldev
#else
#define	rkopen		nodev
#define	rkclose		nodev
#define	rkstrategy	nodev
#define	rksize		NULL
#endif

struct bdevsw	bdevsw[] = {
/* ht = 0 */
	nodev,		nodev,		nodev,		nulldev,	NULL,
	0,
/* tm = 1 */
	nodev,		nodev,		nodev,		nulldev,	NULL,
	0,
/* ts = 2 */
	nodev,		nodev,		nodev,		nulldev,	NULL,
	0,
/* ram = 3 */
	nodev,		nodev,		nodev,		nulldev,	NULL,
	0,
/* hk = 4 */
	nodev,		nodev,		nodev,		nulldev,	NULL,
	0,
/* ra = 5 */
	nodev,		nodev,		nodev,		nulldev,	NULL,
	0,
/* rk = 6 */
	rkopen,		rkclose,	rkstrategy,	nulldev,	rksize,
	0,
/* rl = 7 */
	nodev,		nodev,		nodev,		nulldev,	NULL,
	0,
/* rx = 8 */
	nodev,		nodev,		nodev,		nulldev,	NULL,
	0,
/* si = 9 */
	nodev,		nodev,		nodev,		nulldev,	NULL,
	0,
/* xp = 10 */
	nodev,		nodev,		nodev,		nulldev,	NULL,
	0,
/* br = 11 */
	nodev,		nodev,		nodev,		nulldev,	NULL,
	0,
/* tmscp = 12 (tu81/tk50) */
	nodev,		nodev,		nodev,		nulldev,	NULL,
	0,
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

int	cnopen(), cnclose(), cnread(), cnwrite(), cnioctl();
extern struct tty cons[];

int	logopen(), logclose(), logread(), logioctl(), logselect();
int	syopen(), syread(), sywrite(), syioctl(), syselect();

int	mmrw();
#define	mmselect	seltrue

int	fdopen();
int	ttselect(), seltrue();

struct cdevsw	cdevsw[] = {
/* cn = 0 */
	cnopen,		cnclose,	cnread,		cnwrite,
	cnioctl,	nulldev,	cons,		ttselect,
	nulldev,
/* mem = 1 */
	nulldev,	nulldev,	mmrw,		mmrw,
	nodev,		nulldev,	0,		mmselect,
	nulldev,
/* dz = 2 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* dh = 3 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* dhu = 4 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* lp = 5 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* ht = 6 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* tm = 7 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* ts = 8 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* tty = 9 */
	syopen,		nulldev,	syread,		sywrite,
	syioctl,	nulldev,	0,		syselect,
	nulldev,
/* ptc = 10 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* pts = 11 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* dr = 12 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* hk = 13 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* ra = 14 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* rk = 15 */
	rkopen,		rkclose,	rawrw,		rawrw,
	nodev,		nulldev,	0,		seltrue,
	rkstrategy,
/* rl = 16 */
	nodev,		nodev,		rawrw,		rawrw,
	nodev,		nulldev,	0,		seltrue,
	nodev,
/* rx = 17 */
	nodev,		nodev,		rawrw,		rawrw,
	nodev,		nulldev,	0,		seltrue,
	nodev,
/* si = 18 */
	nodev,		nodev,		rawrw,		rawrw,
	nodev,		nulldev,	0,		seltrue,
	nodev,
/* xp = 19 */
	nodev,		nodev,		rawrw,		rawrw,
	nodev,		nulldev,	0,		seltrue,
	nodev,
/* br = 20 */
	nodev,		nodev,		rawrw,		rawrw,
	nodev,		nulldev,	0,		seltrue,
	nodev,
/* dn = 21 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* log = 22 */
	logopen,	logclose,	logread,	nodev,
	logioctl,	nulldev,	0,		logselect,
	nulldev,
/* tmscp = 23 (tu81/tk50) */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* dhv = 24 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* ingres = 25 */
	nodev,		nodev,		nodev,		nodev,
	nodev,		nulldev,	0,		nodev,
	nodev,
/* fd = 26 */
	fdopen,		nodev,		nodev,		nodev,
	nodev,		nodev,		0,		nodev,
	nodev,
};

int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 *
 * A minimal stub routine can always return 0.
 */
iskmemdev(dev)
	register dev_t dev;
{

	if (major(dev) == 1 && (minor(dev) == 0 || minor(dev) == 1))
		return (1);
	return (0);
}

/*
 * Routine to determine if a device is a disk.
 *
 * A minimal stub routine can always return 0.
 */
isdisk(dev, type)
	dev_t dev;
	register int type;
{

	switch (major(dev)) {
	case 6:			/* rk */
		if (type == IFBLK)
			return (1);
		return (0);
	case 15:		/* rrk */
		if (type == IFCHR)
			return (1);
		/* fall through */
	default:
		return (0);
	}
	/* NOTREACHED */
}

#define MAXDEV	27
static char chrtoblktbl[MAXDEV] =  {
      /* CHR */      /* BLK */
	/* 0 */		NODEV,
	/* 1 */		NODEV,
	/* 2 */		NODEV,
	/* 3 */		NODEV,
	/* 4 */		NODEV,
	/* 5 */		NODEV,
	/* 6 */		NODEV,
	/* 7 */		NODEV,
	/* 8 */		NODEV,
	/* 9 */		NODEV,
	/* 10 */	NODEV,
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	NODEV,
	/* 14 */	NODEV,
	/* 15 */	6,		/* rk */
	/* 16 */	NODEV,
	/* 17 */	NODEV,
	/* 18 */	NODEV,
	/* 19 */	NODEV,
	/* 20 */	NODEV,
	/* 21 */	NODEV,
	/* 22 */	NODEV,
	/* 23 */	NODEV,
	/* 24 */	NODEV,
	/* 25 */	NODEV,
	/* 26 */	NODEV
};

/*
 * Routine to convert from character to block device number.
 *
 * A minimal stub routine can always return NODEV.
 */
chrtoblk(dev)
	register dev_t dev;
{
	register int blkmaj;

	if (major(dev) >= MAXDEV || (blkmaj = chrtoblktbl[major(dev)]) == NODEV)
		return (NODEV);
	return (makedev(blkmaj, minor(dev)));
}

/*
 * This routine returns the cdevsw[] index of the block device
 * specified by the input parameter.    Used by init_main and ufs_mount to
 * find the diskdriver's ioctl entry point so that the label and partition
 * information can be obtained for 'block' (instead of 'character') disks.
 *
 * Rather than create a whole separate table 'chrtoblktbl' is scanned
 * looking for a match.  This routine is only called a half dozen times during
 * a system's life so efficiency isn't a big concern.
*/

blktochr(dev)
	register dev_t dev;
	{
	register int maj = major(dev);
	register int i;

	for	(i = 0; i < MAXDEV; i++)
		{
		if	(maj == chrtoblktbl[i])
			return(i);
		}
	return(NODEV);
	}
