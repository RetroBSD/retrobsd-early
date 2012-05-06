/*
 * SecureDigital flash drive on SPI port.
 *
 * These cards are known to work:
 * 1) NCP SD 256Mb       - type 1, 249856 kbytes,  244 Mbytes
 * 2) Patriot SD 2Gb     - type 2, 1902592 kbytes, 1858 Mbytes
 * 3) Wintec microSD 2Gb - type 2, 1969152 kbytes, 1923 Mbytes
 * 4) Transcend SDHC 4Gb - type 3, 3905536 kbytes, 3814 Mbytes
 * 5) Verbatim SD 2Gb    - type 2, 1927168 kbytes, 1882 Mbytes
 * 6) SanDisk SDHC 4Gb   - type 3, 3931136 kbytes, 3833 Mbytes
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
#include "errno.h"
#include "dk.h"

/*
 * Two SD/MMC disks on SPI.
 * Signals for SPI1:
 *	D0  - SDO1
 *	D10 - SCK1
 *	C4  - SDI1
 */
#define NSD		2
#define SECTSIZE	512
#define SPI_ENHANCED            /* use SPI fifo */
#ifndef SD_MHZ
#define SD_MHZ          13      /* speed 13.33 MHz */
#endif

#define TIMO_WAIT_WDONE 400000
#define TIMO_WAIT_WIDLE 200000
#define TIMO_WAIT_CMD   100000
#define TIMO_WAIT_WDATA 30000
#define TIMO_READ       90000
#define TIMO_SEND_OP    8000
#define TIMO_CMD        7000
#define TIMO_SEND_CSD   6000
#define TIMO_WAIT_WSTOP 5000

int sd_type[NSD];               /* Card type */
int sd_dkn = -1;                /* Statistics slot number */

int sd_timo_cmd;                /* Max timeouts, for sysctl */
int sd_timo_send_op;
int sd_timo_send_csd;
int sd_timo_read;
int sd_timo_wait_cmd;
int sd_timo_wait_wdata;
int sd_timo_wait_wdone;
int sd_timo_wait_wstop;
int sd_timo_wait_widle;

/*
 * Definitions for MMC/SDC commands.
 */
#define CMD_GO_IDLE		0	/* CMD0 */
#define CMD_SEND_OP_MMC		1	/* CMD1 (MMC) */
#define CMD_SEND_IF_COND	8
#define CMD_SEND_CSD		9
#define CMD_SEND_CID		10
#define CMD_STOP		12
#define CMD_SEND_STATUS		13	/* CMD13 */
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

#define DATA_START_BLOCK        0xFE    /* start data for single block */
#define STOP_TRAN_TOKEN         0xFD    /* stop token for write multiple */
#define WRITE_MULTIPLE_TOKEN    0xFC    /* start data for write multiple */

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
static inline unsigned
spi_io (byte)
	unsigned byte;
{
	register struct	sdreg *reg = (struct sdreg*) &SD_PORT;
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
        switch (unit) {
        case 0:
                if (on)
                        LAT_CLR(SD_CS0_PORT) = 1 << SD_CS0_PIN;
                else
                        LAT_SET(SD_CS0_PORT) = 1 << SD_CS0_PIN;
                break;
#ifdef SD_CS1_PORT
        case 1:
                if (on)
                        LAT_CLR(SD_CS1_PORT) = 1 << SD_CS1_PIN;
                else
                        LAT_SET(SD_CS1_PORT) = 1 << SD_CS1_PIN;
                break;
#endif
        }
        if (! on) {
                /* Need additional SPI clocks after deselect. */
                spi_io (0xFF);
        }
}

/*
 * Wait while busy, up to 300 msec.
 */
static void
spi_wait_ready (int limit, int *maxcount)
{
        int i;

        spi_io (0xFF);
	for (i=0; i<limit; i++) {
                if (spi_io (0xFF) == 0xFF) {
                        if (*maxcount < i)
                                *maxcount = i;
                        return;
                }
        }
        printf ("sd: wait_ready(%d) failed\n", limit);
}

/*
 * Read 512 bytes of data from SPI.
 * Use 32-bit mode and 4-word deep FIFO.
 */
static void spi_get_sector (char *data)
{
        register struct sdreg *reg = (struct sdreg*) &SD_PORT;
        int *data32 = (int*) data;
        int i;

#ifdef SPI_ENHANCED
        int r = 0;
        i = SECTSIZE/4;
        reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
        while (r < SECTSIZE/4) {
                if (i > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF)) {
                        reg->buf = ~0;
                        i--;
                }
                if (! (reg->stat & PIC32_SPISTAT_SPIRBE)) {
                        *data32++ = mips_bswap (reg->buf);
                        r++;
                }
        }
        reg->conclr = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
#else
        reg->conset = PIC32_SPICON_MODE32;
        for (i=0; i<SECTSIZE/4; i+=4) {
                reg->buf = ~0;
                while (! (reg->stat & PIC32_SPISTAT_SPIRBF))
                        continue;
                *data32++ = mips_bswap (reg->buf);
                reg->buf = ~0;
                while (! (reg->stat & PIC32_SPISTAT_SPIRBF))
                        continue;
                *data32++ = mips_bswap (reg->buf);
                reg->buf = ~0;
                while (! (reg->stat & PIC32_SPISTAT_SPIRBF))
                        continue;
                *data32++ = mips_bswap (reg->buf);
                reg->buf = ~0;
                while (! (reg->stat & PIC32_SPISTAT_SPIRBF))
                        continue;
                *data32++ = mips_bswap (reg->buf);
        }
        reg->conclr = PIC32_SPICON_MODE32;
#endif
}

/*
 * Send 512 bytes of data to SPI.
 * Use 32-bit mode and 4-word deep FIFO.
 */
static void spi_send_sector (char *data)
{
	register struct	sdreg *reg = (struct sdreg*) &SD_PORT;
        int *data32 = (int*) data;
        int i;

#ifdef SPI_ENHANCED
        int r = 0;
        i = SECTSIZE/4;
        reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
        while (r < SECTSIZE/4) {
                if (i > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF)) {
                        reg->buf = mips_bswap (*data32++);
                        i--;
                }
                if (! (reg->stat & PIC32_SPISTAT_SPIRBE)) {
                        (void) reg->buf;
                        r++;
                }
        }
        reg->conclr = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
#else
        reg->conset = PIC32_SPICON_MODE32;
        for (i=0; i<SECTSIZE/4; i+=4) {
                reg->buf = mips_bswap (*data32++);
                while (! (reg->stat & PIC32_SPISTAT_SPIRBF))
                        continue;
                (void) reg->buf;
                reg->buf = mips_bswap (*data32++);
                while (! (reg->stat & PIC32_SPISTAT_SPIRBF))
                        continue;
                (void) reg->buf;
                reg->buf = mips_bswap (*data32++);
                while (! (reg->stat & PIC32_SPISTAT_SPIRBF))
                        continue;
                (void) reg->buf;
                reg->buf = mips_bswap (*data32++);
                while (! (reg->stat & PIC32_SPISTAT_SPIRBF))
                        continue;
                (void) reg->buf;
        }
        reg->conclr = PIC32_SPICON_MODE32;
#endif
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

        /* Wait for not busy, up to 300 msec. */
        if (cmd != CMD_GO_IDLE)
            spi_wait_ready (TIMO_WAIT_CMD, &sd_timo_wait_cmd);

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
        else if (cmd == CMD_SEND_IF_COND)
                spi_io (0x87);
        else
                spi_io (0xFF);

	/* Wait for a response. */
	for (i=0; i<TIMO_CMD; i++) {
		reply = spi_io (0xFF);
		if (! (reply & 0x80)) {
                        if (sd_timo_cmd < i)
                                sd_timo_cmd = i;
		        return reply;
                }
	}
	if (cmd != CMD_GO_IDLE)
                printf ("sd: card_cmd timeout, cmd=%02x, addr=%08x, reply=%02x\n",
                        cmd, addr, reply);
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
        unsigned char response[4];
	register struct	sdreg *reg = (struct sdreg*) &SD_PORT;

        /* Slow speed: 250 kHz */
        reg->brg = (BUS_KHZ / 250 + 1) / 2 - 1;

	/* Unselect the card. */
	spi_select (unit, 0);
        sd_type[unit] = 0;

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

        /* Check SD version. */
	spi_select (unit, 1);
        reply = card_cmd (CMD_SEND_IF_COND, 0x1AA);
        if (reply & 4) {
                /* Illegal command: card type 1. */
                spi_select (unit, 0);
                sd_type[unit] = 1;
        } else {
                response[0] = spi_io (0xFF);
                response[1] = spi_io (0xFF);
                response[2] = spi_io (0xFF);
                response[3] = spi_io (0xFF);
                spi_select (unit, 0);
                if (response[3] != 0xAA) {
                        printf ("sd%d: cannot detect card type, response=%02x-%02x-%02x-%02x\n",
                                unit, response[0], response[1], response[2], response[3]);
                        return 0;
                }
                sd_type[unit] = 2;
        }

	/* Send repeatedly SEND_OP until Idle terminates. */
	for (i=0; ; i++) {
		spi_select (unit, 1);
		card_cmd (CMD_APP, 0);
		reply = card_cmd (CMD_SEND_OP_SDC,
                        (sd_type[unit] == 2) ? 0x40000000 : 0);
		spi_select (unit, 0);
		if (reply == 0)
			break;
		if (i >= TIMO_SEND_OP) {
			/* Init timed out. */
                        printf ("card_init: SEND_OP timed out, reply = %d\n",
                                reply);
			return 0;
		}
	}
        if (sd_timo_send_op < i)
                sd_timo_send_op = i;

        /* If SD2 read OCR register to check for SDHC card. */
        if (sd_type[unit] == 2) {
		spi_select (unit, 1);
                reply = card_cmd (CMD_READ_OCR, 0);
                if (reply != 0) {
                        spi_select (unit, 0);
                        printf ("sd%d: READ_OCR failed, reply=%02x\n",
                                unit, reply);
                        return 0;
                }
                response[0] = spi_io (0xFF);
                response[1] = spi_io (0xFF);
                response[2] = spi_io (0xFF);
                response[3] = spi_io (0xFF);
                spi_select (unit, 0);
                if ((response[0] & 0xC0) == 0xC0) {
                        sd_type[unit] = 3;
                }
        }
        /* Fast speed. */
        reg->brg = (BUS_KHZ / SD_MHZ / 1000 + 1) / 2 - 1;
	return 1;
}

/*
 * Get number of sectors on the disk.
 * Return nonzero if successful.
 */
int
card_size (unit, nsectors)
	int unit;
	unsigned *nsectors;
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
		reply = spi_io (0xFF);
		if (reply == DATA_START_BLOCK)
			break;
		if (i >= TIMO_SEND_CSD) {
			/* Command timed out. */
			spi_select (unit, 0);
                        printf ("card_size: SEND_CSD timed out, reply = %d\n",
                                reply);
			return 0;
		}
	}
        if (sd_timo_send_csd < i)
                sd_timo_send_csd = i;

	/* Read data. */
	for (i=0; i<sizeof(csd); i++) {
		csd [i] = spi_io (0xFF);
	}
	/* Ignore CRC. */
	spi_io (0xFF);
	spi_io (0xFF);

	/* Disable the card. */
	spi_select (unit, 0);

        /* CSD register has different structure
         * depending upon protocol version. */
	switch (csd[0] >> 6) {
        case 1:                 /* SDC ver 2.00 */
		csize = csd[9] + (csd[8] << 8) + 1;
		*nsectors = csize << 10;
		break;
	case 0:                 /* SDC ver 1.XX or MMC. */
		n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
		csize = (csd[8] >> 6) + (csd[7] << 2) + ((csd[6] & 3) << 10) + 1;
		*nsectors = csize << (n - 9);
		break;
        default:                /* Unknown version. */
                return 0;
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
	printf ("sd%d: read offset %u, length %u bytes, addr %p\n",
		unit, offset, bcount, data);
#endif
	/* Send read-multiple command. */
	spi_select (unit, 1);
	reply = card_cmd (CMD_READ_MULTIPLE, offset);
	if (reply != 0) {
		/* Command rejected. */
                printf ("card_read: bad READ_MULTIPLE reply = %d, offset = %08x\n",
                        reply, offset);
		spi_select (unit, 0);
		return 0;
	}
again:
	/* Wait for a response. */
	for (i=0; ; i++) {
	        int x = spl0();
		reply = spi_io (0xFF);
		splx(x);
		if (reply == DATA_START_BLOCK)
			break;
		if (i >= TIMO_READ) {
			/* Command timed out. */
                        printf ("card_read: READ_MULTIPLE timed out, reply = %d\n",
                                reply);
			spi_select (unit, 0);
			return 0;
		}
	}
        if (sd_timo_read < i)
                sd_timo_read = i;

	/* Read data. */
        if (bcount >= SECTSIZE) {
                spi_get_sector (data);
                data += SECTSIZE;
        } else {
                for (i=0; i<bcount; i++)
                        *data++ = spi_io (0xFF);
                for (; i<SECTSIZE; i++)
                        spi_io (0xFF);
        }
	/* Ignore CRC. */
        spi_io (0xFF);
        spi_io (0xFF);

        if (bcount > SECTSIZE) {
                /* Next sector. */
                bcount -= SECTSIZE;
                goto again;
        }

        /* Stop a read-multiple sequence. */
	card_cmd (CMD_STOP, 0);
	spi_select (unit, 0);
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
	printf ("sd%d: write offset %u, length %u bytes, addr %p\n",
		unit, offset, bcount, data);
#endif
	/* Send pre-erase count. */
	spi_select (unit, 1);
        card_cmd (CMD_APP, 0);
	reply = card_cmd (CMD_SET_WBECNT,
                (bcount + SECTSIZE - 1) / SECTSIZE);
	if (reply != 0) {
		/* Command rejected. */
		spi_select (unit, 0);
                printf ("card_write: bad SET_WBECNT reply = %02x, count = %u\n",
                        reply, (bcount + SECTSIZE - 1) / SECTSIZE);
		return 0;
	}

	/* Send write-multiple command. */
	reply = card_cmd (CMD_WRITE_MULTIPLE, offset);
	if (reply != 0) {
		/* Command rejected. */
		spi_select (unit, 0);
                printf ("card_write: bad WRITE_MULTIPLE reply = %02x\n", reply);
		return 0;
	}
	spi_select (unit, 0);
again:
        /* Select, wait while busy. */
	spi_select (unit, 1);
        spi_wait_ready (TIMO_WAIT_WDATA, &sd_timo_wait_wdata);

	/* Send data. */
	spi_io (WRITE_MULTIPLE_TOKEN);
        if (bcount >= SECTSIZE) {
                spi_send_sector (data);
                data += SECTSIZE;
        } else {
                for (i=0; i<bcount; i++)
                        spi_io (*data++);
                for (; i<SECTSIZE; i++)
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
                printf ("card_write: data rejected, reply = %02x\n", reply);
		return 0;
	}

	/* Wait for write completion. */
       	int x = spl0();
        spi_wait_ready (TIMO_WAIT_WDONE, &sd_timo_wait_wdone);
        splx(x);
	spi_select (unit, 0);

        if (bcount > SECTSIZE) {
                /* Next sector. */
                bcount -= SECTSIZE;
                goto again;
        }

        /* Stop a write-multiple sequence. */
	spi_select (unit, 1);
        spi_wait_ready (TIMO_WAIT_WSTOP, &sd_timo_wait_wstop);
	spi_io (STOP_TRAN_TOKEN);
        spi_wait_ready (TIMO_WAIT_WIDLE, &sd_timo_wait_widle);
	spi_select (unit, 0);
	return 1;
}

static char *
spi_name (port)
        void *port;
{
        switch ((short) (int) port) {
        case (short) (int) &SPI1CON: return "SPI1";
        case (short) (int) &SPI2CON: return "SPI2";
        case (short) (int) &SPI3CON: return "SPI3";
        case (short) (int) &SPI4CON: return "SPI4";
        }
        /* Cannot happen */
        return "???";
}

static int
cs_name (unit)
        int unit;
{
        int port;

#ifdef SD_CS1_PORT
        if (unit == 1) {
                port = (int) &SD_CS1_PORT;
        } else
#endif
        port = (int) &SD_CS0_PORT;

        switch ((short) port) {
        case (short) (int) &TRISA: return 'A';
        case (short) (int) &TRISB: return 'B';
        case (short) (int) &TRISC: return 'C';
        case (short) (int) &TRISD: return 'D';
        case (short) (int) &TRISE: return 'E';
        case (short) (int) &TRISF: return 'F';
        case (short) (int) &TRISG: return 'G';
        }
        /* Cannot happen */
        return '?';
}

static int
cs_pin (unit)
        int unit;
{
#ifdef SD_CS1_PORT
        if (unit == 1) {
                return SD_CS1_PIN;
        }
#endif
        return SD_CS0_PIN;
}

int
sdopen (dev, flag, mode)
	dev_t dev;
	int flag, mode;
{
	register int unit = minor (dev);
	register struct	sdreg *reg = (struct sdreg*) &SD_PORT;
	unsigned nsectors;

	if (unit >= NSD)
		return ENXIO;

	if (! (reg->con & PIC32_SPICON_MSTEN)) {
		/* Initialize hardware. */
#ifdef SD_ENA_PORT
                /* On Duinomite Mega board, pin B13 set low
                 * enables a +3.3V power to SD card. */
		TRIS_CLR(SD_ENA_PORT) = 1 << SD_ENA_PIN;
		LAT_CLR(SD_ENA_PORT) = 1 << SD_ENA_PIN;
		udelay (1000);
#endif
		spi_select (unit, 0);		// initially keep the SD card disabled

                // make Card select an output pin
		TRIS_CLR(SD_CS0_PORT) = 1 << SD_CS0_PIN;
#ifdef SD_CS1_PORT
		TRIS_CLR(SD_CS1_PORT) = 1 << SD_CS1_PIN;
#endif
                reg->stat = 0;
                reg->brg = (BUS_KHZ / SD_MHZ / 1000 + 1) / 2 - 1;
                reg->con = PIC32_SPICON_MSTEN | PIC32_SPICON_CKE |
                        PIC32_SPICON_ON;

#ifdef SD_CS1_PORT
                printf ("sd: port %s, select pins %c%d, %c%d\n",
                        spi_name (&SD_PORT), cs_name(0), cs_pin(0),
                        cs_name(1), cs_pin(1));
#else
                printf ("sd0: port %s, select pin %c%d\n",
                        spi_name (&SD_PORT), cs_name(0), cs_pin(0));
#endif
#ifdef UCB_METER
                /* Allocate statistics slots */
                dk_alloc (&sd_dkn, NSD, "sd");
#endif
        }
        if (! sd_type[unit]) {
                /* Detect a card. */
                if (! card_init (unit)) {
                        printf ("sd%d: no SD/MMC card detected\n", unit);
                        return ENODEV;
                }
                if (! card_size (unit, &nsectors)) {
                        printf ("sd%d: cannot get card size\n", unit);
                        return ENODEV;
                }
                printf ("sd%d: type %s, size %u kbytes, speed %u Mbit/sec\n", unit,
                        sd_type[unit]==3 ? "SDHC" :
                        sd_type[unit]==2 ? "II" : "I",
                        nsectors / (DEV_BSIZE / SECTSIZE),
                        BUS_KHZ / (reg->brg + 1) / 2000);
	}
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
	unsigned nsectors;

	if (! card_size (unit, &nsectors)) {
		/* Cannot get disk size. */
		return 0;
	}
	printf ("sd%d: %u kbytes\n", unit, nsectors / (DEV_BSIZE / SECTSIZE));
	return nsectors / (DEV_BSIZE / SECTSIZE);
}

void
sdstrategy (bp)
	register struct buf *bp;
{
	int s, unit, retry;
	unsigned offset, led = LED_DISK;

	unit = minor (bp->b_dev);
#if 0
	printf ("sd%d: %s block %u, length %u bytes, addr %p\n",
		unit, (bp->b_flags & B_READ) ? "read" : "write",
		bp->b_blkno, bp->b_bcount, bp->b_addr);
#endif
        if (bp->b_blkno > swapstart && bp->b_blkno < swapstart + nswap) {
                /* Visualize swap i/o. */
                led = LED_AUX;
        }
        led_control (led, 1);
        s = splbio();
	if (unit >= NSD) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		goto done;
	}
#ifdef UCB_METER
	if (sd_dkn >= 0) {
                dk_busy |= 1 << (sd_dkn + unit);
		dk_xfer[sd_dkn + unit]++;
		dk_bytes[sd_dkn + unit] += bp->b_bcount;
	}
#endif
	for (retry=0; ; retry++) {
	        if (retry >= 3) {
                        bp->b_flags |= B_ERROR;
                        break;
		}
#if DEV_BSIZE == 1024
		offset = bp->b_blkno << 1;
#else
#error "DEV_BSIZE not supported"
#endif
                if (sd_type[unit] != 3)
                        offset <<= 9;
		if (bp->b_flags & B_READ) {
			if (card_read (unit, offset, bp->b_addr, bp->b_bcount))
                                break;
		} else {
			if (card_write (unit, offset, bp->b_addr, bp->b_bcount))
                                break;
		}
                printf ("sd%d: hard error, %s block %u\n", unit,
                        (bp->b_flags & B_READ) ? "reading" : "writing",
                        bp->b_blkno);
	}
#if 0
	printf ("    %02x-%02x-%02x-%02x-...-%02x-%02x-%02x-%02x\n",
		(unsigned char) bp->b_addr[0], (unsigned char) bp->b_addr[1],
                (unsigned char) bp->b_addr[2], (unsigned char) bp->b_addr[3],
                (unsigned char) bp->b_addr[bp->b_bcount-4],
                (unsigned char) bp->b_addr[bp->b_bcount-3],
                (unsigned char) bp->b_addr[bp->b_bcount-2],
                (unsigned char) bp->b_addr[bp->b_bcount-1]);
#endif
#if 0
	printf ("    %02x", (unsigned char) bp->b_addr[0]);
        int i;
	for (i=1; i<SECTSIZE && i<bp->b_bcount; i++)
                printf ("-%02x", (unsigned char) bp->b_addr[i]);
	printf ("\n");
#endif
done:
	biodone (bp);
        led_control (led, 0);
        splx (s);
#ifdef UCB_METER
	if (sd_dkn >= 0)
                dk_busy &= ~(1 << (sd_dkn + unit));
#endif
}
