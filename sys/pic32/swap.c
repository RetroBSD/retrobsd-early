/*
 * Driver for external RAM-based swap device.
 *
 * Interface:
 *  data[7:0] - connected to PORTx
 *  rd        - fetch a byte from memory to data[7:0], increment address
 *  wr        - write a byte data[7:0] to memory, increment address
 *  ldaddr    - write address from data[7:0] in 3 steps: low-middle-high
 *
 * Signals rd, wr, ldadr are active LOW and idle HIGH.
 * To activate, you need to pulse it high-low-high.
 * CHANGE: IM 23.12.2011 - signals active LOW
 * CHANGE: IM 24.12.2011 - finetuning for 55ns SRAM and 7ns CPLD
 *                       - some nops removed
 *                       - nops marked 55ns are required !!!
 *                       - MAX performance settings for 55ns SRAM
 */
#include "param.h"
#include "systm.h"
#include "buf.h"
#include "errno.h"
#include "dk.h"

int sw_dkn = -1;                /* Statistics slot number */

/*
 * Set data output value.
 */
static inline void data_set (unsigned char byte)
{
        LAT_CLR(SW_DATA_PORT) = 0xff << SW_DATA_PIN;
        LAT_SET(SW_DATA_PORT) = byte << SW_DATA_PIN;
}

/*
 * Switch data bus to input.
 */
static inline void data_switch_input ()
{
        LAT_CLR(SW_DATA_PORT) = 0xff << SW_DATA_PIN;    // !!! PIC32 errata
        TRIS_SET(SW_DATA_PORT) = 0xff << SW_DATA_PIN;
	asm volatile ("nop");
}

/*
 * Switch data bus to output.
 */
static inline void data_switch_output ()
{
        TRIS_CLR(SW_DATA_PORT) = 0xff << SW_DATA_PIN;
	asm volatile ("nop");
}

/*
 * Get data input value.
 */
static inline unsigned char data_get ()
{
	asm volatile ("nop");  // 55ns
        return PORT_VAL(SW_DATA_PORT) >> SW_DATA_PIN;
}

/*
 * Send LDA pulse: low-high.
 */
static inline void lda_pulse ()
{
	LAT_CLR(SW_LDA_PORT) = 1 << SW_LDA_PIN;
	asm volatile ("nop");
	asm volatile ("nop");
	LAT_SET(SW_LDA_PORT) = 1 << SW_LDA_PIN;
}

/*
 * Set RD low.
 * Minimal time between falling edge of RD to data valid is 30ns.
 */
static inline void rd_low ()
{
        LAT_CLR(SW_RD_PORT) = 1 << SW_RD_PIN;

#if BUS_KHZ > 33000
        asm volatile ("nop"); // 55ns
 	asm volatile ("nop"); // 55ns
#endif
#if BUS_KHZ > 66000
        asm volatile ("nop"); // 55ns
#endif
}

/*
 * Set RD high.
 */
static inline void rd_high ()
{
        LAT_SET(SW_RD_PORT) = 1 << SW_RD_PIN;
}

/*
 * Send WR pulse: high-low-high.
 * It shall be minimally 40ns.
 */
static inline void wr_pulse ()
{
        LAT_CLR(SW_WR_PORT) = 1 << SW_WR_PIN;

#if BUS_KHZ > 35000
        asm volatile ("nop");  // 55ns
#endif
#if BUS_KHZ > 75000
        asm volatile ("nop");  // 55ns
#endif
        LAT_SET(SW_WR_PORT) = 1 << SW_WR_PIN;
}

/*
 * Load the 24 bit address to ramdisk.
 * Leave data bus in output mode.
 */
static void
dev_load_address (addr)
        unsigned addr;
{
	/* Toggle rd: make one dummy read - this clears cpld addr pointer */
        rd_low ();
	rd_high ();

	data_switch_output();           /* switch data bus to output */

        data_set (addr);                /* send lower 8 bits */
        lda_pulse();                    /* pulse ldaddr */

        data_set (addr >> 8);           /* send middle 8 bits */
        lda_pulse();                    /* pulse ldaddr */

        data_set (addr >> 16);          /* send high 8 bits */
        lda_pulse();                    /* pulse ldaddr */

	data_switch_input();
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

        data_switch_input();            /* switch data bus to input */

	/* Read data. */
        for (i=0; i<nbytes; i++) {
                rd_low();              /* set rd LOW */

                *data++ = data_get();   /* read a byte of data */

                rd_high();               /* set rd HIGH */
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

	data_switch_output();           /* switch data bus to output */

        for (i=0; i<nbytes; i++) {
                data_set (*data);       /* send a byte of data */
                data++;

                wr_pulse();             /* pulse wr */
        }

        /* Switch data bus to input. */
        data_switch_input();
}

int
swopen (dev, flag, mode)
	dev_t dev;
	int flag, mode;
{
	if (TRIS_VAL(SW_LDA_PORT) & (1 << SW_LDA_PIN)) {
		/* Initialize hardware.
                 * Switch data bus to input. */
                data_switch_input();

                /* Set idle HIGH rd, wr and ldaddr as output pins. */
		LAT_SET(SW_RD_PORT) = 1 << SW_RD_PIN;
		LAT_SET(SW_WR_PORT) = 1 << SW_WR_PIN;
		LAT_SET(SW_LDA_PORT) = 1 << SW_LDA_PIN;
		TRIS_CLR(SW_RD_PORT) = 1 << SW_RD_PIN;
		TRIS_CLR(SW_WR_PORT) = 1 << SW_WR_PIN;
		TRIS_CLR(SW_LDA_PORT) = 1 << SW_LDA_PIN;

                /* Toggle rd: make one dummy read. */
                rd_low();              /* set rd low */
                rd_high();             /* set rd high */
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
	printf ("    %02x", (unsigned char) bp->b_addr[0]);
        int i;
	for (i=1; i<bp->b_bcount; i++)
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
