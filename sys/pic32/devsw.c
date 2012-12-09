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
#include "rdisk.h"
#include "errno.h"
#include "uart.h"

#ifdef UARTUSB_ENABLED
#include "usb_uart.h"
#endif

#ifdef GPIO_ENABLED
#include "gpio.h"
#endif

#ifdef ADC_ENABLED
#include "adc.h"
#endif

#ifdef SPI_ENABLED
#include "spi.h"
#endif

#ifdef GLCD_ENABLED
#include "glcd.h"
#endif

#ifdef OC_ENABLED
#include "oc.h"
#endif

#ifdef PICGA_ENABLED
#include "picga.h"
#endif

extern int rdopen (dev_t dev, int flag, int mode);
extern int rdclose(dev_t dev, int flag, int mode);
extern daddr_t rdsize (dev_t dev);
extern void rdstrategy(register struct buf *bp);

extern int swopen(dev_t dev, int mode, int flag);
extern int swclose(dev_t dev, int mode, int flag);
extern void swstrategy(register struct buf *bp);
extern daddr_t swsize(dev_t dev);
extern int swcread(dev_t dev, register struct uio *uio, int flag);
extern int swcwrite(dev_t dev, register struct uio *uio, int flag);
extern int swcioctl (dev_t dev, register u_int cmd, caddr_t addr, int flag);
extern int swcopen(dev_t dev, int mode, int flag);
extern int swcclose(dev_t dev, int mode, int flag);

/*
 * Null routine; placed in insignificant entries
 * in the bdevsw and cdevsw tables.
 */
int nulldev ()
{
	return (0);
}

int norw (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	return (0);
}

int noioctl (dev, cmd, data, flag)
	dev_t dev;
	u_int cmd;
	caddr_t data;
	int flag;
{
	return EIO; 
;
}

/*
 * root attach routine
 */
void noroot (csr)
	caddr_t csr;
{
	/* Empty. */
}

const struct bdevsw	bdevsw[] = {
{ 
	rdopen,		rdclose,	rdstrategy,
	noroot,		rdsize,		rdioctl,	0 },
{ 
	rdopen,		rdclose,	rdstrategy,
	noroot,		rdsize,		rdioctl,	0 },
{ 
	rdopen,		rdclose,	rdstrategy,
	noroot,		rdsize,		rdioctl,	0 },
{ 
	rdopen,		rdclose,	rdstrategy,
	noroot,		rdsize,		rdioctl,	0,},
{ 
	swopen,		swclose,	swstrategy,
	noroot,		swsize,		swcioctl,	0 },
};

const int nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

// The RetroDisks require the same master number as the disk entry in the
// rdisk.c file.  A bit of a bind, but it means that the RetroDisk
// devices must be numbered from master 0 upwards.

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
{ /* log = 3 */
	logopen,	logclose,	logread,	norw,
	logioctl,	nulldev,	0,		logselect,
	nostrategy,	},
{ /* fd = 4 */
	fdopen,		nulldev,	norw,		norw,
	noioctl,	nulldev,	0,		seltrue,
	nostrategy,	},

#ifdef GPIO_ENABLED
{ /* gpio = 5 */
	gpioopen,	gpioclose,	gpioread,	gpiowrite,
	gpioioctl,	nulldev,	0,              seltrue,
	nostrategy,	},
#else
{
    nulldev,        nulldev,        norw,           norw,
    noioctl,        nulldev,        0,              seltrue,
    nostrategy,     },
#endif

#ifdef ADC_ENABLED
{ /* adc = 6 */
	adc_open,	adc_close,	adc_read,	adc_write,
	adc_ioctl,	nulldev,	0,              seltrue,
	nostrategy,	},
#else
{
    nulldev,        nulldev,        norw,           norw,
    noioctl,        nulldev,        0,              seltrue,
    nostrategy,     },
#endif

#ifdef SPI_ENABLED
{ /* spi = 7 */
	spidev_open,	spidev_close,	spidev_read,	spidev_write,
	spidev_ioctl,	nulldev,	0,              seltrue,
	nostrategy,	},
#else
{
    nulldev,        nulldev,        norw,           norw,
    noioctl,        nulldev,        0,              seltrue,
    nostrategy,     },
#endif
// GLCD = 8 
#ifdef GLCD_ENABLED
{
    glcd_open,      glcd_close,     glcd_read,      glcd_write,
    glcd_ioctl,     nulldev,        0,              seltrue,
    nostrategy,     },
#else
{
    nulldev,        nulldev,        norw,           norw,
    noioctl,        nulldev,        0,              seltrue,
    nostrategy,     },
#endif
// OC = 9
#ifdef OC_ENABLED
{
    oc_open,        oc_close,       oc_read,        oc_write,
    oc_ioctl,       nulldev,        0,              seltrue,
    nostrategy,     },
#else
{
    nulldev,        nulldev,        norw,           norw,
    noioctl,        nulldev,        0,              seltrue,
    nostrategy,     },
#endif
{ // SWAP = 10
	swcopen,	swcclose,	swcread,	swcwrite,
	swcioctl,	nulldev,	0,		seltrue,
	nostrategy,	},

// Ignore this for now - it's WIP.
#ifdef PICGA_ENABLED
{ // PICGA = 11
    picga_open,     picga_close,    picga_read,     picga_write,
    picga_ioctl,    nulldev,        0,              seltrue,
    nostrategy,     },
#else
{
    nulldev,        nulldev,        norw,           norw,
    noioctl,        nulldev,        0,              seltrue,
    nostrategy,     },
#endif

#if defined(UART1_ENABLED) || defined(UART2_ENABLED) || defined(UART3_ENABLED) || defined(UART4_ENABLED) || defined(UART5_ENABLED) || defined(UART6_ENABLED)
{ // UARTS = 12
    uartopen,      uartclose,     uartread,      uartwrite,
    uartioctl,     nulldev,        uartttys,       uartselect,
    nostrategy,     },
#else
{
    nulldev,        nulldev,        norw,           norw,
    noioctl,        nulldev,        0,              seltrue,
    nostrategy,     },
#endif

#ifdef UARTUSB_ENABLED
{   // USB - 13
    usbopen,        usbclose,       usbread,        usbwrite,
    usbioctl,       nulldev,        usbttys,        usbselect,
    nostrategy,     },
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
	case 2:
	case 3:
	case 4:
		if (type == IFBLK)
			return (1);
		return (0);
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
