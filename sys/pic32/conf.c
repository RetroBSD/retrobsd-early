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
#include "systm.h"

/*
 * Null routine; placed in insignificant entries
 * in the bdevsw and cdevsw tables.
 */
static int
nulldev ()
{
	return (0);
}

static int
norw (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	return (0);
}

static int
noioctl (dev, cmd, data, flag)
	dev_t dev;
	u_int cmd;
	caddr_t data;
	int flag;
{
	return (0);
}

/*
 * root attach routine
 */
static void
noroot (csr)
	caddr_t csr;
{
	/* Empty. */
}

#if NRK > 0
#else
#define	rkopen		noopen
#define	rkstrategy	nostrategy
#define	rksize		NULL
#endif

struct bdevsw	bdevsw[] = {
	{ rkopen,	nulldev,	rkstrategy,	/* rk = 0 */
	  noroot,	rksize,		0 },
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

struct cdevsw	cdevsw[] = {
{ /* cn = 0 */
	cnopen,		cnclose,	cnread,		cnwrite,
	cnioctl,	nulldev,	cnttys,		cnselect,
	nostrategy,	},
{ /* mem = 1 */
	nulldev,	nulldev,	mmrw,		mmrw,
	noioctl,	nulldev,	0,		seltrue,
	nostrategy,	},
{ /* tty = 3 */
	syopen,		nulldev,	syread,		sywrite,
	syioctl,	nulldev,	0,		syselect,
	nostrategy,	},
{ /* rk = 4 */
	rkopen,		nulldev,	rawrw,		rawrw,
	noioctl,	nulldev,	0,		seltrue,
	rkstrategy,	},
{ /* log = 5 */
	logopen,	logclose,	logread,	norw,
	logioctl,	nulldev,	0,		logselect,
	nostrategy,	},
{ /* fd = 6 */
	fdopen,		nulldev,	norw,		norw,
	noioctl,	nulldev,	0,		seltrue,
	nostrategy,	},
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
