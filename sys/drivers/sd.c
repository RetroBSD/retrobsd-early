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
 * Signals: SDO2, SDI2, SCK2, /SS2, WP, CP.
 * G6 - SCK2
 * G7 - SDI2
 * G8 - SDO2
 * C1 - /CS
 * C2 - CD
 * C3 - WD

//SPI Configuration
#define SPI_START_CFG_1     (PRI_PRESCAL_64_1 | SEC_PRESCAL_8_1 | MASTER_ENABLE_ON | SPI_CKE_ON | SPI_SMP_ON)
#define SPI_START_CFG_2     (SPI_ENABLE)

// Define the SPI frequency
#define SPI_FREQUENCY         (20000000)

#define SD_CS            PORTBbits.RB1
#define SD_CS_TRIS         TRISBbits.TRISB1

#define SD_CD            0//PORTFbits.RF0
#define SD_CD_TRIS         TRISFbits.TRISF0

#define SD_WE            0//PORTFbits.RF1
#define SD_WE_TRIS         TRISFbits.TRISF1

// Registers for the SPI module you want to use
#define SPICON1            SPI1CON
#define SPISTAT            SPI1STAT
#define SPIBUF            SPI1BUF
#define SPISTAT_RBF         SPI1STATbits.SPIRBF
#define SPICON1bits         SPI1CONbits
#define SPISTATbits         SPI1STATbits
#define SPIENABLE           SPICON1bits.ON
#define SPIBRG             SPI1BRG
// Tris pins for SCK/SDI/SDO lines
#define SPICLOCK         TRISDbits.TRISD10
#define SPIIN               TRISCbits.TRISC4
#define SPIOUT            TRISDbits.TRISD0
 */
#define SDADDR	(struct sdreg*) &SPI1CON
#define NSD	2

/*
 * Number of blocks per drive: assume 16 Mbytes.
 */
#define	NSDBLK	(16*1024*1024/DEV_BSIZE)

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
 * List of active i/o buffers.
 */
struct buf sdtab;

int
sdopen (dev, flag, mode)
	dev_t dev;
	int flag, mode;
{
	register int unit = minor (dev);

	if (unit >= NSD)
		return (ENXIO);
	return (0);
}

/*
 * Query disk size, for swapping.
 */
daddr_t
sdsize (dev)
	dev_t	dev;
{
	return NSDBLK;
}

static void
sdstart()
{
//	register struct	sdreg *reg = SDADDR;
	register struct buf *bp;
//	register int com;
	int unit;

	bp = sdtab.b_actf;
	if (bp == NULL)
		return;
	sdtab.b_active++;
	unit = minor (bp->b_dev);
//	reg->sdda = (unit << 13) | bp->b_blkno;
//	reg->sdba = bp->b_addr;
//	reg->sdwc = -(bp->b_bcount >> 1);
//	com = SDCS_IDE | SDCS_GO;
//	if (bp->b_flags & B_READ)
//		com |= SDCS_RCOM;
//	else
//		com |= SDCS_WCOM;
//	reg->sdcs = com;
}

void
sdstrategy (bp)
	register struct buf *bp;
{
	register int s, unit;

	unit = minor (bp->b_dev);
	if (unit >= NSD) {
		bp->b_error = ENXIO;
		goto bad;
	}
	if (bp->b_blkno >= NSDBLK) {
		bp->b_error = EINVAL;
bad:		bp->b_flags |= B_ERROR;
		iodone (bp);
		return;
	}
	bp->av_forw = 0;
	s = splbio();
	if (sdtab.b_actf == NULL)
		sdtab.b_actf = bp;
	else
		sdtab.b_actl->av_forw = bp;
	sdtab.b_actl = bp;
	if (! sdtab.b_active)
		sdstart ();
	splx (s);
}

void
sdintr()
{
//	register struct sdreg *reg = SDADDR;
	register struct buf *bp;

	if (! sdtab.b_active)
		return;
	bp = sdtab.b_actf;
	sdtab.b_active = 0;
//	if (reg->sdcs & SDCS_ERR) {
		harderr (bp, "sd");
//		log (LOG_NOTICE,"er=%b ds=%b\n", reg->sder, SDER_BITS, reg->sdds, SD_BITS);
//		reg->sdcs = SDCS_RESET | SDCS_GO;
		if (++sdtab.b_errcnt <= 10) {
			sdstart();
			return;
		}
		bp->b_flags |= B_ERROR;
//	}
	sdtab.b_errcnt = 0;
	sdtab.b_actf = bp->av_forw;
//	bp->b_resid = -(reg->sdwc << 1);
	iodone (bp);
	sdstart();
}
