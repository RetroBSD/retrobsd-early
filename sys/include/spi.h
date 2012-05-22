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

#define SPICTL_SETMODE      _IO ('p', 0)        /* set SPI mode */
#define SPICTL_SETRATE      _IO ('p', 1)        /* set clock rate, kHz */
#define SPICTL_SETSELPIN    _IO ('p', 2)        /* set select pin */
#define SPICTL_IO8(n)       _ION('p', 3, n)     /* transfer n*8 bits */
#define SPICTL_IO16(n)      _ION('p', 4, n)     /* transfer n*16 bits */
#define SPICTL_IO32(n)      _ION('p', 5, n)     /* transfer n*32 bits */

#ifdef KERNEL
int spi_open (dev_t dev, int flag, int mode);
int spi_close (dev_t dev, int flag, int mode);
int spi_read (dev_t dev, struct uio *uio, int flag);
int spi_write (dev_t dev, struct uio *uio, int flag);
int spi_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#endif
