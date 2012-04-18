/*
 * Generic SPI driver for PIC32.
 *
 * Copyright (C) 2012 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include "param.h"
#include "conf.h"
#include "user.h"
#include "ioctl.h"
#include "systm.h"
#include "uio.h"
#include "spi.h"

#define NSPI    6       /* Ports SPI1...SPI6 */

/*
 * To enable debug output, comment out the first line,
 * and uncomment the second line.
 */
#define PRINTDBG(...) /*empty*/
//#define PRINTDBG printf

typedef struct {
    int select_pin;
} spi_t;

spi_t spi_data [NSPI];

int spi_open (dev_t dev, int flag, int mode)
{
    int channel = minor (dev);
    spi_t *spi;

    if (channel >= NSPI)
        return ENXIO;

    if (u.u_uid != 0)
            return EPERM;
    spi = &spi_data[channel];
    spi->select_pin = 0;

    return 0;
}

int spi_close (dev_t dev, int flag, int mode)
{
    return 0;
}

int spi_read (dev_t dev, struct uio *uio, int flag)
{
    return 0;
}

int spi_write (dev_t dev, struct uio *uio, int flag)
{
    return 0;
}

int spi_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag)
{
    int channel = minor (dev);
    spi_t *spi;

    PRINTDBG ("gpioioctl (cmd=%08x, addr=%08x, flag=%d)\n", cmd, addr, flag);
    if (channel >= NSPI)
        return ENXIO;

    spi = &spi_data[channel];
    if (! spi->select_pin)
        return EINVAL;

    switch (cmd) {
    default:
        return ENODEV;

    case SPICTL_SETMODE:        /* set SPI mode */
    case SPICTL_SETRATE:        /* set clock rate */
    case SPICTL_SETSELPIN:      /* set select pin */
    case SPICTL_IO8:            /* transfer 8 bits */
    case SPICTL_IO16:           /* transfer 16 bits */
    case SPICTL_IO24:           /* transfer 24 bits */
    case SPICTL_IO32:           /* transfer 32 bits */
        //TODO
        return ENODEV;
    }
    return 0;
}
