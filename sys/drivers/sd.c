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
 * Two SD/MMC disks on SPI2.
 * Signals:
 *	G6 - SCK2
 *	G7 - SDI2
 *	G8 - SDO2
 *	C1 - /CS0
 *	C2 - CD0
 *	C3 - WE0
 *	E5 - /CS1
 *	E6 - CD1
 *	E7 - WE1
 */
#define SDADDR		(struct sdreg*) &SPI2CON
#define NSD		2
#define	SLOW		250
#define	FAST		20000

#define PIN_CS0		1	/* port C */
#define PIN_CD0		2	/* port C */
#define PIN_WE0		3	/* port C */
#define PIN_CS1		5	/* port E */
#define PIN_CD1		6	/* port E */
#define PIN_WE1		7	/* port E */

/*
 * Number of blocks per drive: assume 16 Mbytes.
 */
#define	NSDBLK		(16*1024*1024/DEV_BSIZE)

/*
 * Definitions for MMC/SDC commands.
 */
#define CMD_GO_IDLE		0		/* CMD0 */
#define CMD_SEND_OP_MMC		1		/* CMD1 (MMC) */
#define	CMD_SEND_OP_SDC		(0x80+41)	/* ACMD41 (SDC) */
#define CMD_SEND_IF		8
#define CMD_SEND_CSD		9
#define CMD_SEND_CID		10
#define CMD_STOP		12
#define CMD_STATUS_SDC 		(0x80+13)	/* ACMD13 (SDC) */
#define CMD_SET_BLEN		16
#define CMD_READ_SINGLE		17
#define CMD_READ_MULTIPLE	18
#define CMD_SET_BCOUNT		23		/* (MMC) */
#define	CMD_SET_WBECNT		(0x80+23)	/* ACMD23 (SDC) */
#define CMD_WRITE_SINGLE	24
#define CMD_WRITE_MULTIPLE	25
#define CMD_APP			55
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

static inline void
spi_select (unit, on)
	int unit, on;
{
	if (unit == 0) {
		if (on)
			PORTCCLR = 1 << PIN_CS0;
		else
			PORTCSET = 1 << PIN_CS0;
	} else {
		if (on)
			PORTECLR = 1 << PIN_CS1;
		else
			PORTESET = 1 << PIN_CS1;
	}
}

/*
 * Send one byte of data and receive one back at the same time.
 */
static unsigned
spi_io (byte)
	unsigned byte;
{
	register struct	sdreg *reg = SDADDR;

	reg->buf = byte;
	while (! (reg->stat & PIC32_SPISTAT_SPIRBF))
		continue;
	return reg->buf;
}

/*
 * SD card connector detection switch.
 * Returns nonzero if the card is present.
 */
static int
card_detect (unit)
	int unit;
{
	if (unit == 0) {
		return ! (PORTC & (1 << PIN_CD0));
	} else {
		return ! (PORTE & (1 << PIN_CD1));
	}
}

/*
 * SD card write protect detection switch.
 * Returns nonzero if the card is writable.
 */
static int
card_writable (unit)
	int unit;
{
	if (unit == 0) {
		return ! (PORTC & (1 << PIN_WE0));
	} else {
		return ! (PORTE & (1 << PIN_WE1));
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

	/* Send cmd checksum. */
	if (cmd == CMD_GO_IDLE)
		spi_io (0x95);
	else if (cmd == CMD_SEND_IF)
		spi_io (0x87);
	else
		spi_io (0x01);

	/* Wait for a response, allow for up to 8 bytes delay. */
	for (i=0; i<8; i++) {
		reply = spi_io (0xFF);
		if (reply != 0xFF)
			return reply;
	}
	return 0xFF;
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
		return 0;
	}

	/* Send repeatedly SEND_OP until Idle terminates. */
	for (i=0; ; i++) {
		if (i >= 3 /*10000*/) {
			/* Init timed out. */
			return 0;
		}
		spi_select (unit, 1);
		reply = card_cmd (CMD_SEND_OP_SDC, 0);
		spi_select (unit, 0);
		if (reply <= 1)
			break;
	}

	/* Set block length. */
	spi_select (unit, 1);
	reply = card_cmd (CMD_SET_BLEN, DEV_BSIZE);
	spi_select (unit, 0);
	if (reply != 0) {
		/* Bad block size. */
		return 0;
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
card_read (unit, bno, data)
	int unit;
	unsigned bno;
	char *data;
{
	int reply, i;

	/* Send READ command. */
	spi_select (unit, 1);
	reply = card_cmd (CMD_READ_SINGLE, bno * DEV_BSIZE);
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
	i = DEV_BSIZE;
	do {
		*data++ = spi_io (0xFF);
	} while (--i > 0);

	/* Ignore CRC. */
	spi_io (0xFF);
	spi_io (0xFF);

	/* Disable the card. */
	spi_select (unit, 0);
	return 1;
}

/*
 * Write a block of data.
 * Return nonzero if successful.
 */
int
card_write (unit, bno, data)
	int unit;
	unsigned bno;
	char *data;
{
	unsigned reply, i;

	/* Check Write Protect. */
	if (! card_writable (unit))
		return 0;

	/* Send WRITE command. */
	spi_select (unit, 1);
	reply = card_cmd (CMD_WRITE_SINGLE, bno * DEV_BSIZE);
	if (reply != 0) {
		/* Command rejected. */
		spi_select (unit, 0);
		return 0;
	}

	/* Send data. */
	spi_io (0xFE);
	for (i=0; i<DEV_BSIZE; i++)
		spi_io (*data++);

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
		TRISCCLR = 1 << PIN_CS0;	// make Card select an output pin
		TRISECLR = 1 << PIN_CS1;

		reg->con = PIC32_SPICON_MSTEN | PIC32_SPICON_CKE |
			   PIC32_SPICON_ON;
	}
	/* Slow speed: max 40 kbit/sec. */
	reg->brg = (KHZ / SLOW + 1) / 2 - 1;

	if (! card_detect (unit)) {
		/* No card present. */
printf ("sd%d: no SD/MMC card present\n", unit);
		return ENODEV;
	}
	if (! card_init (unit)) {
		/* Initialization failed. */
printf ("sd%d: init failed\n", unit);
		return ENODEV;
	}

	/* Fast speed: up to 25 Mbit/sec allowed. */
	reg->brg = (KHZ / FAST + 1) / 2 - 1;
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
	unsigned bcount, blkno;
	char *addr;

	unit = minor (bp->b_dev);
	if (unit >= NSD) {
		bp->b_error = ENXIO;
bad:		bp->b_flags |= B_ERROR;
		iodone (bp);
		return;
	}
	if (bp->b_blkno >= NSDBLK) {
		bp->b_error = EINVAL;
		goto bad;
	}
	/* printf ("sd%d: %s block %u, length %u bytes, addr %p\n",
		unit, (bp->b_flags & B_READ) ? "read" : "write",
		bp->b_blkno, bp->b_bcount, bp->b_addr); */
	bcount = bp->b_bcount;
	addr = bp->b_addr;
	blkno = bp->b_blkno;
	s = splbio();
next:
	for (retry=0; retry<3; retry++) {
		if (bp->b_flags & B_READ) {
			if (card_read (unit, blkno, addr)) {
ok:				if (bcount <= DEV_BSIZE)
					goto done;
				bcount -= DEV_BSIZE;
				addr += DEV_BSIZE;
				blkno++;
				goto next;
			}
		} else {
			if (card_write (unit, blkno, addr)) {
				goto ok;
			}
		}
	}
	printf ("sd%d: hard error, %s block %u\n", unit,
		(bp->b_flags & B_READ) ? "reading" : "writing", blkno);
	bp->b_flags |= B_ERROR;
done:
	iodone (bp);
	splx (s);
}
