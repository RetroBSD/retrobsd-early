/*
 * SecureDigital flash drive on SPI port.
 *
 * Copyright (C) 2010 Serge Vakulenko, <serge@vak.ru>
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
#include "systm.h"
#include "buf.h"
#include "conf.h"
#include "user.h"
#include "dk.h"
#include "syslog.h"
#include "map.h"

/*
 * Two SD/MMC disks on SPI1.
 * Signals:
 *	D0  - SDO1
 *	D10 - SCK1
 *	C4  - SDI1
 *	A9  - /CS0
 *	A10 - /CS1
 */
#define SDADDR		((struct sdreg*) &SPI1CON)
#define NSD		2
#define SECTSIZE	512
#define SLOW		250     /* 250 kHz */
#define FAST		20000   /* 20 MHz */

#define PIN_CS0		9	/* port A9 */
#define PIN_CS1		10	/* port A10 */

/*
 * Definitions for MMC/SDC commands.
 */
#define CMD_GO_IDLE		0	/* CMD0 */
#define CMD_SEND_OP_MMC		1	/* CMD1 (MMC) */
#define CMD_SEND_IF		8
#define CMD_SEND_CSD		9
#define CMD_SEND_CID		10
#define CMD_STOP		12
#define CMD_STATUS_SDC 		13	/* ACMD13 (SDC) */
#define CMD_SET_BLEN		16
#define CMD_READ_SINGLE		17
#define CMD_READ_MULTIPLE	18
#define CMD_SET_BCOUNT		23	/* (MMC) */
#define	CMD_SET_WBECNT		23      /* ACMD23 (SDC) */
#define CMD_WRITE_SINGLE	24
#define CMD_WRITE_MULTIPLE	25
#define	CMD_SEND_OP_SDC		41      /* ACMD41 (SDC) */
#define CMD_APP			55      /* CMD55 */
#define CMD_READ_OCR		58

/*
 * SPI registers.
 */
struct sdreg {
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

/*
 * Send one byte of data and receive one back at the same time.
 */
static unsigned
spi_io (byte)
	unsigned byte;
{
	register struct	sdreg *reg = SDADDR;
        register int count;

	reg->buf = (unsigned char) byte;
	for (count=0; count<1000; count++)
                if (reg->stat & PIC32_SPISTAT_SPIRBF)
                        return (unsigned char) reg->buf;
        return 0xFF;
}

static inline void
spi_select (unit, on)
	int unit, on;
{
	if (on) {
		PORTACLR = (unit == 0) ? 1 << PIN_CS0 : 1 << PIN_CS1;
	} else {
                PORTASET = (unit == 0) ? 1 << PIN_CS0 : 1 << PIN_CS1;

                /* Need additional SPI clocks after deselect */
                spi_io (0xFF);
	}
}

/*
 * Send a command and address to SD media.
 * Return response:
 *   FF - timeout
 *   00 - command accepted
 *   01 - command received, card in idle state
 *
 * Other codes:
 *   bit 0 = Idle state
 *   bit 1 = Erase Reset
 *   bit 2 = Illegal command
 *   bit 3 = Communication CRC error
 *   bit 4 = Erase sequence error
 *   bit 5 = Address error
 *   bit 6 = Parameter error
 *   bit 7 = Always 0
 */
static int
card_cmd (cmd, addr)
	unsigned cmd, addr;
{
	int i, reply;

	/* Send a comand packet (6 bytes). */
	spi_io (cmd | 0x40);
	spi_io (addr >> 24);
	spi_io (addr >> 16);
	spi_io (addr >> 8);
	spi_io (addr);

	/* Send cmd checksum for CMD_GO_IDLE.
         * For all other commands, CRC is ignored. */
        if (cmd == CMD_GO_IDLE)
                spi_io (0x95);
        else
                spi_io (0xFF);

	/* Wait for a response. */
	for (i=0; i<200; i++) {
		reply = spi_io (0xFF);
		if (! (reply & 0x80))
		        break;
	}
	return reply;
}

/*
 * Initialize a card.
 * Return nonzero if successful.
 */
static int
card_init (unit)
	int unit;
{
	int i, reply;

	/* Unselect the card. */
	spi_select (unit, 0);

	/* Send 80 clock cycles for start up. */
	for (i=0; i<10; i++)
		spi_io (0xFF);

	/* Select the card and send a single GO_IDLE command. */
	spi_select (unit, 1);
	reply = card_cmd (CMD_GO_IDLE, 0);
	spi_select (unit, 0);
	if (reply != 1) {
		/* It must return Idle. */
//printf ("sd%d: bad GO_IDLE reply = %d\n", unit, reply);
		return 0;
	}

	/* Send repeatedly SEND_OP until Idle terminates. */
	for (i=0; ; i++) {
		spi_select (unit, 1);
		card_cmd (CMD_APP, 0);
		reply = card_cmd (CMD_SEND_OP_SDC, 0);
		spi_select (unit, 0);
		if (reply == 0)
			break;
		if (i >= 10000) {
			/* Init timed out. */
//printf ("card_init: SEND_OP timed out, reply = %d\n", reply);
			return 0;
		}
	}
	return 1;
}

/*
 * Get number of sectors on the disk.
 * Return nonzero if successful.
 */
int
card_size (unit, nbytes)
	int unit;
	unsigned *nbytes;
{
	unsigned char csd [16];
	unsigned csize, n;
	int reply, i;

	spi_select (unit, 1);
	reply = card_cmd (CMD_SEND_CSD, 0);
	if (reply != 0) {
		/* Command rejected. */
		spi_select (unit, 0);
		return 0;
	}
	/* Wait for a response. */
	for (i=0; ; i++) {
		if (i >= 25000) {
			/* Command timed out. */
			spi_select (unit, 0);
			return 0;
		}
		reply = spi_io (0xFF);
		if (reply == 0xFE)
			break;
	}

	/* Read data. */
	for (i=0; i<sizeof(csd); i++) {
		csd [i] = spi_io (0xFF);
	}
	/* Ignore CRC. */
	spi_io (0xFF);
	spi_io (0xFF);

	/* Disable the card. */
	spi_select (unit, 0);

	if ((csd[0] >> 6) == 1) {
		/* SDC ver 2.00 */
		csize = csd[9] + (csd[8] << 8) + 1;
		*nbytes = csize << 10;
	} else {
		/* SDC ver 1.XX or MMC. */
		n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
		csize = (csd[8] >> 6) + (csd[7] << 2) + ((csd[6] & 3) << 10) + 1;
		*nbytes = csize << (n - 9);
	}
	return 1;
}

/*
 * Read a block of data.
 * Return nonzero if successful.
 */
int
card_read (unit, offset, data, bcount)
	int unit;
	unsigned offset, bcount;
	char *data;
{
	int reply, i;
#if 0
	printf ("sd%d: read block %u, length %u bytes, addr %p\n",
		unit, offset/DEV_BSIZE, bcount, data);
#endif
again:
	/* Send READ command. */
	spi_select (unit, 1);
	reply = card_cmd (CMD_READ_SINGLE, offset);
	if (reply != 0) {
		/* Command rejected. */
printf ("card_read: bad READ_SINGLE reply = %d, offset = %08x\n", reply, offset);
		spi_select (unit, 0);
		return 0;
	}

	/* Wait for a response. */
	for (i=0; ; i++) {
		if (i >= 25000) {
			/* Command timed out. */
printf ("card_read: READ_SINGLE timed out, reply = %d\n", reply);
			spi_select (unit, 0);
			return 0;
		}
		reply = spi_io (0xFF);
		if (reply == 0xFE)
			break;
//if (reply != 0xFF) printf ("card_read: READ_SINGLE reply = %d\n", reply);
	}

	/* Read data. */
	for (i=0; i<SECTSIZE; i++) {
                if (i < bcount)
                        *data++ = spi_io (0xFF);
                else
                        spi_io (0xFF);
        }
	/* Ignore CRC. */
        spi_io (0xFF);
        spi_io (0xFF);

	/* Disable the card. */
	spi_select (unit, 0);

        if (bcount > SECTSIZE) {
                bcount -= SECTSIZE;
                offset += SECTSIZE;
                goto again;
        }
	return 1;
}

/*
 * Write a block of data.
 * Return nonzero if successful.
 */
int
card_write (unit, offset, data, bcount)
	int unit;
	unsigned offset, bcount;
	char *data;
{
	unsigned reply, i;
#if 0
	printf ("sd%d: write block %u, length %u bytes, addr %p\n",
		unit, offset/DEV_BSIZE, bcount, data);
#endif
again:
	/* Send WRITE command. */
	spi_select (unit, 1);
	reply = card_cmd (CMD_WRITE_SINGLE, offset);
	if (reply != 0) {
		/* Command rejected. */
		spi_select (unit, 0);
		return 0;
	}

	/* Send data. */
	spi_io (0xFE);
	for (i=0; i<SECTSIZE; i++) {
	        if (i < bcount)
                        spi_io (*data++);
                else
                        spi_io (0xFF);
        }
	/* Send dummy CRC. */
	spi_io (0xFF);
	spi_io (0xFF);

	/* Check if data accepted. */
	reply = spi_io (0xFF);
	if ((reply & 0x1f) != 0x05) {
		/* Data rejected. */
		spi_select (unit, 0);
		return 0;
	}

	/* Wait for write completion. */
	for (i=0; ; i++) {
		if (i >= 250000) {
			/* Write timed out. */
			spi_select (unit, 0);
			return 0;
		}
		reply = spi_io (0xFF);
		if (reply != 0)
			break;
	}

	/* Disable the card. */
	spi_select (unit, 0);

        if (bcount > SECTSIZE) {
                bcount -= SECTSIZE;
                offset += SECTSIZE;
                goto again;
        }
	return 1;
}

int
sdopen (dev, flag, mode)
	dev_t dev;
	int flag, mode;
{
	register int unit = minor (dev);
	register struct	sdreg *reg = SDADDR;

	if (unit >= NSD)
		return ENXIO;

	if (! (reg->con & PIC32_SPICON_MSTEN)) {
		/* Initialize hardware. */
		spi_select (0, 0);		// initially keep the SD card disabled
		TRISACLR = 1 << PIN_CS0;	// make Card select an output pin
		TRISACLR = 1 << PIN_CS1;
	}
        /* Slow speed: max 40 kbit/sec. */
	reg->stat = 0;
        reg->brg = (KHZ / SLOW + 1) / 2 - 1;
	reg->con = PIC32_SPICON_MSTEN | PIC32_SPICON_CKE |
                PIC32_SPICON_ON;

	if (! card_init (unit)) {
		/* Initialization failed. */
                printf ("sd%d: no SD/MMC card detected\n", unit);
		return ENODEV;
	}

	/* Fast speed: up to 25 Mbit/sec allowed. */
	reg->stat = 0;
	reg->brg = (KHZ / FAST + 1) / 2 - 1;
	reg->con = PIC32_SPICON_MSTEN | PIC32_SPICON_CKE |
                PIC32_SPICON_ON;
	return 0;
}

/*
 * Query disk size, for swapping.
 */
daddr_t
sdsize (dev)
	dev_t	dev;
{
	register int unit = minor (dev);
	unsigned nbytes;

	if (! card_size (unit, &nbytes)) {
		/* Cannot get disk size. */
		return 0;
	}
	//printf ("sd%d: %u kbytes\n", unit, nbytes / 1024);
	return nbytes / DEV_BSIZE;
}

void
sdstrategy (bp)
	register struct buf *bp;
{
	int s, unit, retry;

	unit = minor (bp->b_dev);
#if 0
	printf ("sd%d: %s block %u, length %u bytes, addr %p\n",
		unit, (bp->b_flags & B_READ) ? "read" : "write",
		bp->b_blkno, bp->b_bcount, bp->b_addr);
#endif
        s = splbio();
	if (unit >= NSD) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		biodone (bp);
                splx (s);
		return;
	}
	for (retry=0; ; retry++) {
	        if (retry >= 3) {
                        bp->b_flags |= B_ERROR;
                        break;
		}
		if (bp->b_flags & B_READ) {
			if (card_read (unit, bp->b_blkno*DEV_BSIZE,
                            bp->b_addr, bp->b_bcount))
                                break;
		} else {
			if (card_write (unit, bp->b_blkno*DEV_BSIZE,
                            bp->b_addr, bp->b_bcount))
                                break;
		}
                printf ("sd%d: hard error, %s block %u\n", unit,
                        (bp->b_flags & B_READ) ? "reading" : "writing",
                        bp->b_blkno);
	}
	biodone (bp);
        splx (s);
#if 0
	printf ("    %02x-%02x-%02x-%02x-...-%02x-%02x\n",
		(unsigned char) bp->b_addr[0], (unsigned char) bp->b_addr[1],
                (unsigned char) bp->b_addr[2], (unsigned char) bp->b_addr[3],
                (unsigned char) bp->b_addr[bp->b_bcount-2],
                (unsigned char) bp->b_addr[bp->b_bcount-1]);
#endif
}
