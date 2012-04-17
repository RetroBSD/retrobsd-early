/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Ioctl definitions for SPI driver.
 */
#ifndef	_SPI_H
#define _SPI_H

#include <sys/ioctl.h>

#define SPICTL_SETCHAN      _IO(p, 0)           /* set SPI channel */
#define SPICTL_SETRATE      _IO(p, 1)           /* set clock rate */
#define SPICTL_SETMODE      _IO(p, 2)           /* set SPI mode */
#define SPICTL_SETSELPIN    _IO(p, 3)           /* set select pin */
#define SPICTL_IO32         _IOWR(p, 4, int)    /* transfer 32 bits */

#ifdef KERNEL
int spi_open (dev_t dev, int flag, int mode);
int spi_close (dev_t dev, int flag, int mode);
int spi_read (dev_t dev, struct uio *uio, int flag);
int spi_write (dev_t dev, struct uio *uio, int flag);
int spi_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#endif
