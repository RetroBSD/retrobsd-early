/*
 * UART driver for PIC32.
 *
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "conf.h"
#include "user.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "systm.h"

/*
 * PIC32 UART registers.
 */
struct uartreg {
        volatile unsigned mode;		/* Mode */
        volatile unsigned modeclr;
        volatile unsigned modeset;
        volatile unsigned modeinv;
        volatile unsigned sta;		/* Status and control */
        volatile unsigned staclr;
        volatile unsigned staset;
        volatile unsigned stainv;
        volatile unsigned txreg;	/* Transmit */
        volatile unsigned unused1;
        volatile unsigned unused2;
        volatile unsigned unused3;
        volatile unsigned rxreg;	/* Receive */
        volatile unsigned unused4;
        volatile unsigned unused5;
        volatile unsigned unused6;
        volatile unsigned brg;		/* Baud rate */
        volatile unsigned brgclr;
        volatile unsigned brgset;
        volatile unsigned brginv;
};

struct tty cnttys [NKL];

void cnstart (struct tty *tp);

/*ARGSUSED*/
int
cnopen (dev, flag, mode)
	dev_t dev;
{
	register struct uartreg *reg;
	register struct tty *tp;
	register int d;

	d = minor(dev);
	tp = &cnttys[d];
	if (d==0 && ! tp->t_addr)
		tp->t_addr = (caddr_t) &U1MODE;
	if (d >= NKL || ! (reg = (struct uartreg*)tp->t_addr))
		return (ENXIO);
	tp->t_oproc = cnstart;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		if (tp->t_ispeed == 0) {
			tp->t_ispeed = B115200;
			tp->t_ospeed = B115200;
		}
		ttychars(tp);
		tp->t_state = TS_ISOPEN | TS_CARR_ON;
		tp->t_flags = ECHO | XTABS | CRMOD | CRTBS | CRTERA | CTLECH | CRTKIL;
	}
	if ((tp->t_state & TS_XCLUDE) && u.u_uid != 0)
		return (EBUSY);

	reg->brg = PIC32_BRG_BAUD (KHZ * 1000, tp->t_ospeed);
	reg->sta = 0;
	reg->mode = PIC32_UMODE_PDSEL_8NPAR | PIC32_UMODE_ON;
	reg->staset = PIC32_USTA_URXEN | PIC32_USTA_UTXEN;

	/* Enable receive interrupt. */
	if (&reg->mode == &U1MODE)
		IECSET(0) = 1 << PIC32_IRQ_U1RX;
	else
		IECSET(1) = 1 << (PIC32_IRQ_U2RX-32);

	if (! linesw[tp->t_line].l_open)
		return (ENODEV);
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*ARGSUSED*/
int
cnclose (dev, flag, mode)
	dev_t dev;
{
	register struct tty *tp = &cnttys[minor(dev)];

	if (linesw[tp->t_line].l_close)
		(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);
	return(0);
}

/*ARGSUSED*/
int
cnread (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = &cnttys[minor(dev)];

	if (! linesw[tp->t_line].l_read)
		return (ENODEV);
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

/*ARGSUSED*/
int
cnwrite (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = &cnttys[minor(dev)];

	if (! linesw[tp->t_line].l_write)
		return (ENODEV);
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

int
cnselect (dev, rw)
	register dev_t dev;
	int rw;
{
	register struct tty *tp = &cnttys[minor(dev)];

	return (ttyselect (tp, rw));
}

/*ARGSUSED*/
int
cnioctl (dev, cmd, addr, flag)
	dev_t dev;
	register u_int cmd;
	caddr_t addr;
{
	register struct tty *tp = &cnttys[minor(dev)];
	register int error;

	if (linesw[tp->t_line].l_ioctl) {
		error = (*linesw[tp->t_line].l_ioctl) (tp, cmd, addr, flag);
		if (error >= 0)
			return (error);
	}
	error = ttioctl(tp, cmd, addr, flag);
	if (error < 0)
		error = ENOTTY;
	return (error);
}

void
cnintr (dev)
	dev_t dev;
{
	register int c;
	register struct tty *tp = &cnttys[minor(dev)];
	register struct uartreg *reg = (struct uartreg *)tp->t_addr;

        /* Receive */
	if (reg->sta & PIC32_USTA_URXDA) {
                c = reg->rxreg;
                if (linesw[tp->t_line].l_rint)
                        (*linesw[tp->t_line].l_rint) (c, tp);
        }
	if (reg->sta & PIC32_USTA_OERR)
		reg->staclr = PIC32_USTA_OERR;

        /* Transmit */
        if ((reg->sta & PIC32_USTA_TRMT) && (tp->t_state & TS_BUSY)) {
                tp->t_state &= ~TS_BUSY;
                if (linesw[tp->t_line].l_start)
                        (*linesw[tp->t_line].l_start) (tp);
        }
}

void
cnstart (tp)
	register struct tty *tp;
{
	register struct uartreg *reg;
	register int c, s;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
out:		/* Disable transmit_interrupt. */
		if (&reg->mode == &U1MODE)
			IECCLR(0) = 1 << PIC32_IRQ_U1TX;
		else
			IECCLR(1) = 1 << (PIC32_IRQ_U2TX-32);
		splx (s);
		return;
	}
	ttyowake(tp);
	if (tp->t_outq.c_cc == 0)
		goto out;
	reg = (struct uartreg*) tp->t_addr;
	if (reg->sta & PIC32_USTA_TRMT) {
		c = getc (&tp->t_outq);
		reg->txreg = c & 0xff;
		tp->t_state |= TS_BUSY;
	}
	/* Enable transmit interrupt. */
	if (&reg->mode == &U1MODE)
		IECSET(0) = 1 << PIC32_IRQ_U1TX;
	else
		IECSET(1) = 1 << (PIC32_IRQ_U2TX-32);
	splx (s);
}

/*
 * Put a symbol on console terminal.
 */
void
cnputc (c)
	char c;
{
	register struct tty *tp = &cnttys[0];
	register int s, timo;

	s = spltty();
again:
	/*
	 * Try waiting for the console tty to come ready,
	 * otherwise give up after a reasonable time.
	 */
	timo = 30000;
	while ((U1STA & PIC32_USTA_TRMT) == 0)
		if (--timo == 0)
			break;
        if (tp->t_state & TS_BUSY) {
                cnintr (0);
		goto again;
        }
	U1TXREG = c;
	if (c == '\n')
		cnputc('\r');

	timo = 30000;
	while ((U1STA & PIC32_USTA_TRMT) == 0)
		if (--timo == 0)
			break;

        /* Clear TX interrupt. */
	IECCLR(0) = 1 << PIC32_IRQ_U1TX;
	splx (s);
}
