/*
 * Driver for external RAM-based swap device.
 *
 * Interface:
 *  data[7:0] - connected to PORTx
 *  rd        - fetch a byte from memory to data[7:0], increment address
 *  wr        - write a byte data[7:0] to memory, increment address
 *  ldaddr    - write address from data[7:0] in 3 steps: low-middle-high
 *
 * Signals rd, wr, ldadr are low idle.
 * To activate, you need to toggle it low-high-low.
 */
#include "param.h"
#include "systm.h"
#include "buf.h"
#include "errno.h"
#include "dk.h"

int sw_dkn = -1;                /* Statistics slot number */

/*
 * Load the 24 bit address to ramdisk.
 * Leave data bus in output mode.
 */
static void
dev_load_address (addr)
        unsigned addr;
{
        /* Switch data bus to output. */
        TRIS_CLR(SW_DATA_PORT) = 0xff << SW_DATA_PIN;

        /* Send lower 8 bits, toggle ldaddr. */
        PORT_CLR(SW_DATA_PORT) = 0xff << SW_DATA_PIN;
        PORT_SET(SW_DATA_PORT) = (addr & 0xff) << SW_DATA_PIN;
        PORT_SET(SW_LDA_PORT) = 1 << SW_LDA_PIN;
        PORT_CLR(SW_LDA_PORT) = 1 << SW_LDA_PIN;

        /* Send middle 8 bits, toggle ldaddr. */
        PORT_CLR(SW_DATA_PORT) = 0xff << SW_DATA_PIN;
        PORT_SET(SW_DATA_PORT) = ((addr >> 8) & 0xff) << SW_DATA_PIN;
        PORT_SET(SW_LDA_PORT) = 1 << SW_LDA_PIN;
        PORT_CLR(SW_LDA_PORT) = 1 << SW_LDA_PIN;

        /* Send high 8 bits, toggle ldaddr. */
        PORT_CLR(SW_DATA_PORT) = 0xff << SW_DATA_PIN;
        PORT_SET(SW_DATA_PORT) = ((addr >> 16) & 0xff) << SW_DATA_PIN;
        PORT_SET(SW_LDA_PORT) = 1 << SW_LDA_PIN;
        PORT_CLR(SW_LDA_PORT) = 1 << SW_LDA_PIN;
}

/*
 * Read a block of data.
 */
static void
dev_read (blockno, data, nbytes)
	unsigned blockno, nbytes;
	char *data;
{
	int i;
#if 0
	printf ("sw0: read block %u, length %u bytes, addr %p\n",
		blockno, nbytes, data);
#endif
        dev_load_address (blockno * DEV_BSIZE);

        /* Switch data bus to input. */
        TRIS_SET(SW_DATA_PORT) = 0xff << SW_DATA_PIN;

	/* Read data. */
        for (i=0; i<nbytes; i++) {
                /* Toggle rd. */
                PORT_SET(SW_RD_PORT) = 1 << SW_RD_PIN;
                PORT_CLR(SW_RD_PORT) = 1 << SW_RD_PIN;

                /* Read a byte of data. */
                *data++ = PORT_VAL(SW_DATA_PORT) >> SW_DATA_PIN;
        }
}

/*
 * Write a block of data.
 */
void
dev_write (blockno, data, nbytes)
	unsigned blockno, nbytes;
	char *data;
{
	unsigned i;
#if 0
	printf ("sw0: write block %u, length %u bytes, addr %p\n",
		blockno, nbytes, data);
#endif
        dev_load_address (blockno * DEV_BSIZE);

        for (i=0; i<nbytes; i++) {
                /* Send a byte of data. */
                PORT_CLR(SW_DATA_PORT) = 0xff << SW_DATA_PIN;
                PORT_SET(SW_DATA_PORT) = (unsigned char) *data << SW_DATA_PIN;
                data++;

                /* Toggle wr. */
                PORT_SET(SW_WR_PORT) = 1 << SW_WR_PIN;
                PORT_CLR(SW_WR_PORT) = 1 << SW_WR_PIN;
        }

        /* Switch data bus to input. */
        TRIS_SET(SW_DATA_PORT) = 0xff << SW_DATA_PIN;
}

int
swopen (dev, flag, mode)
	dev_t dev;
	int flag, mode;
{
	if (TRIS_VAL(SW_LDA_PORT) & (1 << SW_LDA_PIN)) {
		/* Initialize hardware.
                 * Switch data bus to input. */
		TRIS_SET(SW_DATA_PORT) = 0xff << SW_DATA_PIN;

                /* Set rd, wr and ldaddr as output pins. */
		PORT_CLR(SW_RD_PORT) = 1 << SW_RD_PIN;
		PORT_CLR(SW_WR_PORT) = 1 << SW_WR_PIN;
		PORT_CLR(SW_LDA_PORT) = 1 << SW_LDA_PIN;
		TRIS_CLR(SW_RD_PORT) = 1 << SW_RD_PIN;
		TRIS_CLR(SW_WR_PORT) = 1 << SW_WR_PIN;
		TRIS_CLR(SW_LDA_PORT) = 1 << SW_LDA_PIN;

                /* Toggle rd: make one dummy read. */
		PORT_INV(SW_RD_PORT) = 1 << SW_RD_PIN;
#ifdef UCB_METER
                /* Allocate statistics slot */
                dk_alloc (&sw_dkn, 1, "sw");
#endif
        }
	return 0;
}

void
swstrategy (bp)
	register struct buf *bp;
{
	int s;

#if 0
	printf ("sw0: %s block %u, length %u bytes, addr %p\n",
		(bp->b_flags & B_READ) ? "read" : "write",
		bp->b_blkno, bp->b_bcount, bp->b_addr);
#endif
        led_control (LED_AUX, 1);
        s = splbio();
#ifdef UCB_METER
	if (sw_dkn >= 0) {
                dk_busy |= 1 << sw_dkn;
		dk_xfer[sw_dkn]++;
		dk_bytes[sw_dkn] += bp->b_bcount;
	}
#endif
        if (bp->b_flags & B_READ) {
                dev_read (bp->b_blkno, bp->b_addr, bp->b_bcount);
        } else {
                dev_write (bp->b_blkno, bp->b_addr, bp->b_bcount);
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
	biodone (bp);
        led_control (LED_AUX, 0);
        splx (s);
#ifdef UCB_METER
	if (sw_dkn >= 0)
                dk_busy &= ~(1 << sw_dkn);
#endif
}
