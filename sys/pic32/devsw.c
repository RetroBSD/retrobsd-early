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

#ifdef GPIO_ENABLED
#include "gpio.h"
#endif

#ifndef SWAPDEV
#define swopen      nulldev
#define swstrategy  nostrategy
#endif

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
	return -1;
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

const struct bdevsw	bdevsw[] = {
{ /* sd = 0 */
	sdopen,		nulldev,	sdstrategy,
	noroot,		sdsize,		0 },
{ /* sw = 1 */
	swopen,		nulldev,	swstrategy,
	noroot,		0,		0 },
};
const int nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

const struct cdevsw	cdevsw[] = {
{ /* cn = 0 */
	cnopen,		cnclose,	cnread,		cnwrite,
	cnioctl,	nulldev,	cnttys,		cnselect,
	nostrategy,	},
{ /* mem = 1 */
	nulldev,	nulldev,	mmrw,		mmrw,
	noioctl,	nulldev,	0,		seltrue,
	nostrategy,	},
{ /* tty = 2 */
	syopen,		nulldev,	syread,		sywrite,
	syioctl,	nulldev,	0,		syselect,
	nostrategy,	},
{ /* sd = 3 */
	sdopen,		nulldev,	rawrw,		rawrw,
	noioctl,	nulldev,	0,		seltrue,
	sdstrategy,	},
{ /* log = 4 */
	logopen,	logclose,	logread,	norw,
	logioctl,	nulldev,	0,		logselect,
	nostrategy,	},
{ /* fd = 5 */
	fdopen,		nulldev,	norw,		norw,
	noioctl,	nulldev,	0,		seltrue,
	nostrategy,	},
{ /* sw = 6 */
	swopen,		nulldev,	rawrw,		rawrw,
	noioctl,	nulldev,	0,		seltrue,
	swstrategy,	},

#ifdef GPIO_ENABLED
{ /* gpio = 7 */
	gpioopen,	gpioclose,	gpioread,	gpiowrite,
	gpioioctl,	nulldev,	0,              seltrue,
	nostrategy,	},
#else
{
        nulldev,        nulldev,        norw,           norw,
        noioctl,        nulldev,        0,              seltrue,
        nostrategy,     },
#endif

};
const int nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

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
	case 0:			/* sd */
	case 1:			/* sw */
		if (type == IFBLK)
			return (1);
		return (0);
	case 3:			/* rsd */
	case 6:			/* rsw */
		if (type == IFCHR)
			return (1);
		/* fall through */
	default:
		return (0);
	}
	/* NOTREACHED */
}

#define MAXDEV	7
static const char chrtoblktbl[MAXDEV] =  {
	/* CHR */      /* BLK */
	/* 0 */		NODEV,
	/* 1 */		NODEV,
	/* 2 */		NODEV,
	/* 3 */		0,		/* sd */
	/* 4 */		NODEV,
	/* 5 */		NODEV,
	/* 6 */		1,
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
