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

dev_t	rootdev = makedev(0,0),
	swapdev = makedev(0,1),
	pipedev = makedev(0,0);

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
{ /* rk = 0 */
	rkopen,		rkclose,	rkstrategy,	nulldev,	rksize,
	0,		},
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
{ /* cn = 0 */
	cnopen,		cnclose,	cnread,		cnwrite,
	cnioctl,	nulldev,	cons,		ttselect,
	nulldev,	},
{ /* mem = 1 */
	nulldev,	nulldev,	mmrw,		mmrw,
	nodev,		nulldev,	0,		mmselect,
	nulldev,	},
{ /* tty = 3 */
	syopen,		nulldev,	syread,		sywrite,
	syioctl,	nulldev,	0,		syselect,
	nulldev,	},
{ /* rk = 4 */
	rkopen,		rkclose,	rawrw,		rawrw,
	nodev,		nulldev,	0,		seltrue,
	rkstrategy,	},
{ /* log = 5 */
	logopen,	logclose,	logread,	nodev,
	logioctl,	nulldev,	0,		logselect,
	nulldev,	},
{ /* fd = 6 */
	fdopen,		nodev,		nodev,		nodev,
	nodev,		nodev,		0,		nodev,
	nodev,	},
};

int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 *
 * A minimal stub routine can always return 0.
 */
int
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
int
isdisk(dev, type)
	dev_t dev;
	register int type;
{

	switch (major(dev)) {
	case 0:			/* rk */
		if (type == IFBLK)
			return (1);
		return (0);
	case 4:			/* rrk */
		if (type == IFCHR)
			return (1);
		/* fall through */
	default:
		return (0);
	}
	/* NOTREACHED */
}

#define MAXDEV	7
static char chrtoblktbl[MAXDEV] =  {
      /* CHR */      /* BLK */
	/* 0 */		NODEV,
	/* 1 */		NODEV,
	/* 2 */		NODEV,
	/* 3 */		NODEV,
	/* 4 */		0,		/* rk */
	/* 5 */		NODEV,
	/* 6 */		NODEV,
};

/*
 * Routine to convert from character to block device number.
 *
 * A minimal stub routine can always return NODEV.
 */
int
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
int
blktochr(dev)
	register dev_t dev;
{
	register int maj = major(dev);
	register int i;

	for (i = 0; i < MAXDEV; i++) {
		if (maj == chrtoblktbl[i])
			return(i);
	}
	return(NODEV);
}
