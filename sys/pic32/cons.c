/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * KL/DL-11 driver
 */
#include "param.h"
#include "conf.h"
#include "user.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "systm.h"

/*
 *  KL11/DL11 registers and bits
 */
struct dldevice
{
	short	dlrcsr;
	short	dlrbuf;
	short	dlxcsr;
	short	dlxbuf;
};

/* bits in dlrcsr */
#define	DL_DSC		0100000		/* data status change (RO) */
#define	DL_RNG		0040000		/* ring indicator (RO) */
#define	DL_CTS		0020000		/* clear to send (RO) */
#define	DL_CD		0010000		/* carrier detector (RO) */
#define	DL_RA		0004000		/* receiver active (RO) */
#define	DL_SRD		0002000		/* secondary received data (RO) */
/* bits 9-8 are unused */
#define	DL_RDONE	0000200		/* receiver done (RO) */
#define	DL_RIE		0000100		/* receiver interrupt enable */
#define	DL_DIE		0000040		/* dataset interrupt enable */
/* bit 4 is unused */
#define	DL_STD		0000010		/* secondary transmitted data */
#define	DL_RTS		0000004		/* request to send */
#define	DL_DTR		0000002		/* data terminal ready */
#define	DL_RE		0000001		/* reader enable (write only) */
#define	DL_BITS		\
"\10\20DSC\17RNG\16CTS\15CD\14RA\13SRD\10RDONE\7RIE\6DIE\4STD\3RTS\2DTR\1RE"

/* bits in dlrbuf */
#define	DLRBUF_ERR	0100000		/* error (RO) */
#define	DLRBUF_OVR	0040000		/* overrun (RO) */
#define	DLRBUF_FRE	0020000		/* framing error (RO) */
#define	DLRBUF_RDPE	0010000		/* receive data parity error (RO) */
#define	DLRBUF_BITS	\
"\10\20ERR\17OVR\16FRE\15RDPE"

/* bits in dlxcsr */
/* bits 15-8 are unused */
#define	DLXCSR_TRDY	0000200		/* transmitter ready (RO) */
#define	DLXCSR_TIE	0000100		/* transmitter interrupt enable */
/* bits 5-3 are unused */
#define	DLXCSR_MM	0000004		/* maintenance */
/* bit 1 is unused */
#define	DLXCSR_BRK	0000001		/* break */
#define	DLXCSR_BITS	\
"\10\10TRDY\7TIE\3MM\1BRK"

/*
 * Normal addressing:
 * minor 0 addresses KLADDR
 * minor 1 thru n-1 address from KLBASE (0176600),
 *    where n is the number of additional KL11's
 * minor n on address from DLBASE (0176500)
 */
struct dldevice *cnaddr = (struct dldevice*) 0177560;

int	nkl11 = NKL;			/* for pstat */
struct	tty cnttys [NKL];
extern char	partab[];

void cnstart (struct tty *tp);

int
cnattach(addr, unit)
	struct dldevice *addr;
{
	if ((u_int)unit <= NKL) {
		cnttys[unit].t_addr = (caddr_t)addr;
		return (1);
	}
	return (0);
}

/*ARGSUSED*/
int
cnopen(dev, flag, mode)
	dev_t dev;
{
	register struct dldevice *addr;
	register struct tty *tp;
	register int d;

	d = minor(dev);
	tp = &cnttys[d];
	if (!d && !tp->t_addr)
		tp->t_addr = (caddr_t)cnaddr;
	if (d >= NKL || !(addr = (struct dldevice *)tp->t_addr))
		return (ENXIO);
	tp->t_oproc = cnstart;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_state = TS_ISOPEN|TS_CARR_ON;
		tp->t_flags = EVENP|ECHO|XTABS|CRMOD;
	}
	if (tp->t_state&TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);
	addr->dlrcsr |= DL_RIE|DL_DTR|DL_RE;
	addr->dlxcsr |= DLXCSR_TIE;
	if (! linesw[tp->t_line].l_open)
		return (ENODEV);
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*ARGSUSED*/
int
cnclose(dev, flag, mode)
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
cnread(dev, uio, flag)
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
cnwrite(dev, uio, flag)
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
cnselect(dev, rw)
	register dev_t dev;
	int rw;
{
	register struct tty *tp = &cnttys[minor(dev)];

	return (ttyselect (tp, rw));
}

/*ARGSUSED*/
void
cnrint(dev)
	dev_t dev;
{
	register int c;
	register struct dldevice *addr;
	register struct tty *tp = &cnttys[minor(dev)];

	addr = (struct dldevice *)tp->t_addr;
	c = addr->dlrbuf;
	addr->dlrcsr |= DL_RE;
	if (linesw[tp->t_line].l_rint)
		(*linesw[tp->t_line].l_rint) (c, tp);
}

/*ARGSUSED*/
int
cnioctl(dev, cmd, addr, flag)
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
cnxint(dev)
	dev_t dev;
{
	register struct tty *tp = &cnttys[minor(dev)];

	tp->t_state &= ~TS_BUSY;
	if (linesw[tp->t_line].l_start)
		(*linesw[tp->t_line].l_start) (tp);
}

void
cnstart(tp)
	register struct tty *tp;
{
	register struct dldevice *addr;
	register int c, s;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))
		goto out;
	ttyowake(tp);
	if (tp->t_outq.c_cc == 0)
		goto out;
	addr = (struct dldevice*) tp->t_addr;
	if ((addr->dlxcsr & DLXCSR_TRDY) == 0)
		goto out;
	c = getc (&tp->t_outq);
	if (tp->t_flags & (RAW | LITOUT))
		addr->dlxbuf = c & 0xff;
	else
		addr->dlxbuf = c | (partab[c] & 0200);
	tp->t_state |= TS_BUSY;
out:
	splx (s);
}

/* copied, for supervisory networking, to sys_sup.c */
void
cnputc(c)
	char c;
{
	register int s, timo;

	timo = 30000;
	/*
	 * Try waiting for the console tty to come ready,
	 * otherwise give up after a reasonable time.
	 */
	while ((cnaddr->dlxcsr & DLXCSR_TRDY) == 0)
		if (--timo == 0)
			break;
	if (c == 0)
		return;
	s = cnaddr->dlxcsr;
	cnaddr->dlxcsr = 0;
	cnaddr->dlxbuf = c;
	if (c == '\n')
		cnputc('\r');
	cnputc(0);
	cnaddr->dlxcsr = s;
}
