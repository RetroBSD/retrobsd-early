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

#define NSPI    4       /* Ports SPI1...SPI4 */

/*
 * To enable debug output, uncomment the first line.
 */
//#define PRINTDBG printf
#ifndef PRINTDBG
#   define PRINTDBG(...) /*empty*/
#endif

/*
 * SPI registers.
 */
struct spireg {
    volatile unsigned con;		/* Control */
    volatile unsigned conclr;
    volatile unsigned conset;
    volatile unsigned coninv;
    volatile unsigned stat;		/* Status */
    volatile unsigned statclr;
    volatile unsigned statset;
    volatile unsigned statinv;
    volatile unsigned buf;		/* Transmit and receive buffer */
    volatile unsigned unused1;
    volatile unsigned unused2;
    volatile unsigned unused3;
    volatile unsigned brg;		/* Baud rate generator */
    volatile unsigned brgclr;
    volatile unsigned brgset;
    volatile unsigned brginv;
};

typedef struct {
    struct spireg *reg; /* hardware registers */
    int channel;        /* SPI channel number (0...3) */
    int select_pin;     /* index of select pin */
    int mode;           /* clock polarity and phase */
    int khz;            /* data rate */
} spi_t;

spi_t spi_data [NSPI];

/*
 * Open /dev/spi# device.
 * Use default SPI parameters:
 * - rate 250 kHz;
 * - no sleect pin.
 */
int spi_open (dev_t dev, int flag, int mode)
{
    int channel = minor (dev);
    spi_t *spi;
    static struct spireg *spi_reg[] = {
        (struct spireg *) &SPI1CON,
        (struct spireg *) &SPI2CON,
        (struct spireg *) &SPI3CON,
        (struct spireg *) &SPI4CON,
    };

    if (channel >= NSPI)
        return ENXIO;

    if (u.u_uid != 0)
            return EPERM;
    spi = &spi_data[channel];
    spi->channel = channel;
    spi->reg = spi_reg [spi->channel];
    spi->select_pin = 0;                // default no select pin
    spi->khz = 250;                     // default rate 250 kHz
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

/*
 * Setup the SPI channel registers.
 */
static void spi_setup (spi_t *spi)
{
    register struct spireg *reg = spi->reg;

    if (reg == (struct spireg*) &SD_PORT) {
        /* Do not change mode of SD card port. */
        return;
    }
    PRINTDBG ("spi%d: %u kHz, mode %d\n", spi->channel+1, spi->khz, spi->mode);

    reg->con = 0;
    reg->stat = 0;
    reg->brg = (BUS_KHZ / spi->khz + 1) / 2 - 1;
    reg->con = PIC32_SPICON_MSTEN;              // SPI master
    if (! (spi->mode & 1))
        reg->conset = PIC32_SPICON_CKE;         // sample on trailing clock edge
    if (spi->mode & 2)
        reg->conset = PIC32_SPICON_CKP;         // clock idle high
    reg->conset = PIC32_SPICON_ON;              // SPI enable
}

/*
 * Control the select pin.
 * Pin number: 00000XXX0000YYYY
 *  XXX: 0 for port A, 1 for port B, ... 6 for port G
 * YYYY: pin number, from 0 to 15
 */
static void spi_select (spi_t *spi, int on)
{
    static unsigned volatile *const latset[8] = {
        0, &LATASET, &LATBSET, &LATCSET, &LATDSET, &LATESET, &LATFSET, &LATGSET,
    };
    static unsigned volatile *const latclr[8] = {
        0, &LATACLR, &LATBCLR, &LATCCLR, &LATDCLR, &LATECLR, &LATFCLR, &LATGCLR,
    };
    static unsigned volatile *const trisclr[8] = {
        0, &TRISACLR,&TRISBCLR,&TRISCCLR,&TRISDCLR,&TRISECLR,&TRISFCLR,&TRISGCLR,
    };
    int mask, portnum;

    mask = 1 << (spi->select_pin & 15);
    portnum = (spi->select_pin >> 8) & 7;
    if (! portnum)
        return;

    /* Output pin on/off */
    if (on > 0)
        *latclr[portnum] = mask;
    else
        *latset[portnum] = mask;

    /* Direction output */
    if (on < 0) {
        *trisclr[portnum] = mask;
        PRINTDBG ("spi%d: select pin %c%d\n", spi->channel+1,
            "?ABCDEFG"[portnum], spi->select_pin & 15);
    }
}

/*
 * 32-bit SPI transaction.
 */
static void spi_io32 (spi_t *spi, unsigned int *data, int nelem)
{
    register struct spireg *reg = spi->reg;

    PRINTDBG ("spi%d [%d]", spi->channel+1, nelem);
    spi_select (spi, 1);
    reg->conset = PIC32_SPICON_MODE32;
    while (nelem-- > 0) {
        PRINTDBG (" %08x", *data);
        reg->buf = *data;
        while (! (reg->stat & PIC32_SPISTAT_SPIRBF))
            continue;
        *data = reg->buf;
        PRINTDBG ("-%08x", *data);
        data++;
    }
    reg->conclr = PIC32_SPICON_MODE32;
    spi_select (spi, 0);
    PRINTDBG ("\n");
}

/*
 * 16-bit SPI transaction.
 */
static void spi_io16 (spi_t *spi, unsigned short *data, int nelem)
{
    register struct spireg *reg = spi->reg;

    PRINTDBG ("spi%d [%d]", spi->channel+1, nelem);
    spi_select (spi, 1);
    reg->conset = PIC32_SPICON_MODE16;
    while (nelem-- > 0) {
        PRINTDBG (" %04x", *data);
        reg->buf = *data;
        while (! (reg->stat & PIC32_SPISTAT_SPIRBF))
            continue;
        *data = reg->buf;
        PRINTDBG ("-%04x", *data);
        data++;
    }
    reg->conclr = PIC32_SPICON_MODE16;
    spi_select (spi, 0);
    PRINTDBG ("\n");
}

/*
 * 8-bit SPI transaction.
 */
static void spi_io8 (spi_t *spi, unsigned char *data, int nelem)
{
    register struct spireg *reg = spi->reg;

    PRINTDBG ("spi%d [%d]", spi->channel+1, nelem);
    spi_select (spi, 1);
    while (nelem-- > 0) {
        PRINTDBG (" %02x", *data);
        reg->buf = *data;
        while (! (reg->stat & PIC32_SPISTAT_SPIRBF))
            continue;
        *data = reg->buf;
        PRINTDBG ("-%02x", *data);
        data++;
    }
    spi_select (spi, 0);
    PRINTDBG ("\n");
}

/*
 * SPI control operations:
 * - SPICTL_SETMODE   - set clock polarity and phase
 * - SPICTL_SETRATE   - set data rate in kHz
 * - SPICTL_SETSELPIN - set select pin
 * - SPICTL_IO8(n)    - n*8 bit transaction
 * - SPICTL_IO16(n)   - n*16 bit transaction
 * - SPICTL_IO32(n)   - n*32 bit transaction
 */
int spi_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag)
{
    spi_t *spi;
    int channel = minor (dev);
    int nelem;

    //PRINTDBG ("spi%d: ioctl (cmd=%08x, addr=%08x)\n", channel+1, cmd, addr);
    if (channel >= NSPI)
        return ENXIO;
    spi = &spi_data[channel];

    switch (cmd & ~(IOCPARM_MASK << 16)) {
    default:
        return ENODEV;

    case SPICTL_SETMODE:        /* set SPI mode */
        /*      --- Clock ----
         * Mode Polarity Phase
         *   0     0       0
         *   1     0       1
         *   2     1       0
         *   3     1       1
         */
        spi->mode = (unsigned) addr & 3;
        spi_setup (spi);
        return 0;

    case SPICTL_SETRATE:        /* set clock rate, kHz */
        spi->khz = (unsigned) addr;
        spi_setup (spi);
        return 0;

    case SPICTL_SETSELPIN:      /* set select pin */
        spi->select_pin = (unsigned) addr;
        spi_select (spi, -1);
        return 0;

    case SPICTL_IO8(0):         /* transfer n*8 bits */
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (baduaddr (addr) || baduaddr (addr + nelem - 1))
            return EFAULT;
        spi_io8 (spi, (unsigned char*) addr, nelem);
        break;

    case SPICTL_IO16(0):        /* transfer n*16 bits */
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 1) ||
            baduaddr (addr) || baduaddr (addr + nelem*2 - 1))
            return EFAULT;
        spi_io16 (spi, (unsigned short*) addr, nelem);
        break;

    case SPICTL_IO32(0):        /* transfer n*32 bits */
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 3) ||
            baduaddr (addr) || baduaddr (addr + nelem*4 - 1))
            return EFAULT;
        spi_io32 (spi, (unsigned int*) addr, nelem);
        break;
    }
    return 0;
}
