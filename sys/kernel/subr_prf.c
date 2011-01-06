/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "buf.h"
#include "msgbuf.h"
#include "conf.h"
#include "ioctl.h"
#include "tty.h"
#include "reboot.h"
#include "systm.h"
#include "syslog.h"

#define TOCONS	0x1
#define TOTTY	0x2
#define TOLOG	0x4

/*
 * In case console is off,
 * panicstr contains argument to last
 * call to panic.
 */
char	*panicstr;

/*
 * Print a character on console or users terminal.
 * If destination is console then the last MSGBUFS characters
 * are saved in msgbuf for inspection later.
 */
static void
putchar (c, flags, tp)
	int c, flags;
	register struct tty *tp;
{

	if (flags & TOTTY) {
		register int s = spltty();

		if (tp && (tp->t_state & (TS_CARR_ON | TS_ISOPEN)) ==
			(TS_CARR_ON | TS_ISOPEN)) {
			if (c == '\n')
				(void) ttyoutput('\r', tp);
			(void) ttyoutput(c, tp);
			ttstart(tp);
		}
		splx(s);
	}
	if ((flags & TOLOG) && c != '\0' && c != '\r' && c != 0177) {
		char sym = c;
		logwrt (&sym, 1, logMSG);
	}
	if ((flags & TOCONS) && c != '\0')
		cnputc(c);
}

/*
 * Printn prints a number n in base b.
 * We don't use recursion to avoid deep kernels stacks.
 */
static void
printn (n, b, flags, ttyp)
	long n;
	u_int b;
	struct tty *ttyp;
{
	char prbuf[12];
	register char *cp = prbuf;
	register int offset = 0;

	if (n<0)
		switch(b) {
		case 8:		/* unchecked, but should be like hex case */
		case 16:
			offset = b-1;
			n++;
			break;
		case 10:
			putchar('-', flags, ttyp);
			n = -n;
			break;
		}
	do {
		*cp++ = "0123456789ABCDEF"[offset + n%b];
	} while ((n /= b));
	do {
		putchar(*--cp, flags, ttyp);
	} while (cp > prbuf);
}

static void
prf (fmt, adx, flags, ttyp)
	register char *fmt;
	register u_int *adx;
	int flags;
	struct tty *ttyp;
{
	register int c;
	u_int b;
	char *s;
	int	i, any;
loop:
	while ((c = *fmt++) != '%') {
		if (c == '\0')
			return;
		putchar(c, flags, ttyp);
	}
	c = *fmt++;
	switch (c) {

	case 'l':
		c = *fmt++;
		switch(c) {
			case 'x':
				b = 16;
				goto lnumber;
			case 'd':
				b = 10;
				goto lnumber;
			case 'o':
				b = 8;
				goto lnumber;
			default:
				putchar('%', flags, ttyp);
				putchar('l', flags, ttyp);
				putchar(c, flags, ttyp);
		}
		break;
	case 'X':
		b = 16;
		goto lnumber;
	case 'D':
		b = 10;
		goto lnumber;
	case 'O':
		b = 8;
lnumber:	printn(*(long *)adx, b, flags, ttyp);
		adx += (sizeof(long) / sizeof(int)) - 1;
		break;
	case 'x':
		b = 16;
		goto number;
	case 'd':
	case 'u':		/* what a joke */
		b = 10;
		goto number;
	case 'o':
		b = 8;
number:		printn((long)*adx, b, flags, ttyp);
		break;
	case 'c':
		putchar(*adx, flags, ttyp);
		break;
	case 'b':
		b = *adx++;
		s = (char *)*adx;
		printn((long)b, *s++, flags, ttyp);
		any = 0;
		if (b) {
			while ((i = *s++)) {
				if (b & (1 << (i - 1))) {
					putchar(any? ',' : '<', flags, ttyp);
					any = 1;
					for (; (c = *s) > 32; s++)
						putchar(c, flags, ttyp);
				} else
					for (; *s > 32; s++)
						;
			}
			if (any)
				putchar('>', flags, ttyp);
		}
		break;
	case 's':
		s = (char *)*adx;
		while ((c = *s++))
			putchar(c, flags, ttyp);
		break;
	case '%':
		putchar(c, flags, ttyp);
		break;
	default:
		putchar('%', flags, ttyp);
		putchar(c, flags, ttyp);
		break;
	}
	adx++;
	goto loop;
}

static void
logpri (level)
	int level;
{

	putchar('<', TOLOG, (struct tty *)0);
	printn((u_long)level, 10, TOLOG, (struct tty *)0);
	putchar('>', TOLOG, (struct tty *)0);
}

/*
 * Scaled down version of C Library printf.
 * Used to print diagnostic information directly on console tty.
 * Since it is not interrupt driven, all system activities are
 * suspended.  Printf should not be used for chit-chat.
 *
 * One additional format: %b is supported to decode error registers.
 * Usage is:
 *	printf("reg=%b\n", regval, "<base><arg>*");
 * Where <base> is the output base expressed as a control character,
 * e.g. \10 gives octal; \20 gives hex.  Each arg is a sequence of
 * characters, the first of which gives the bit number to be inspected
 * (origin 1), and the next characters (up to a control character, i.e.
 * a character <= 32), give the name of the register.  Thus
 *	printf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 * would produce output:
 *	reg=3<BITTWO,BITONE>
 */
void
printf(char *fmt, ...)
{
	prf(fmt, &fmt + 1, TOCONS | TOLOG, (struct tty *)0);
}

/*
 * Uprintf prints to the current user's terminal,
 * guarantees not to sleep (so could be called by interrupt routines;
 * but prints on the tty of the current process)
 * and does no watermark checking - (so no verbose messages).
 * NOTE: with current kernel mapping scheme, the user structure is
 * not guaranteed to be accessible at interrupt level (see seg.h);
 * a savemap/restormap would be needed here or in putchar if uprintf
 * was to be used at interrupt time.
 */
void
uprintf (char *fmt, ...)
{
	register struct tty *tp;

	tp = u.u_ttyp;
	if (tp == NULL)
		return;

	if (ttycheckoutq (tp, 1))
		prf (fmt, &fmt+1, TOTTY, tp);
}

/*
 * tprintf prints on the specified terminal (console if none)
 * and logs the message.  It is designed for error messages from
 * single-open devices, and may be called from interrupt level
 * (does not sleep).
 */
void
tprintf (register struct tty *tp, char *fmt, ...)
{
	int flags = TOTTY | TOLOG;
	extern struct tty cons;

	logpri(LOG_INFO);
	if (tp == (struct tty *)NULL)
		tp = &cons;
	if (ttycheckoutq(tp, 0) == 0)
		flags = TOLOG;
	prf(fmt, &fmt + 1, flags, tp);
	logwakeup(logMSG);
}

/*
 * Log writes to the log buffer,
 * and guarantees not to sleep (so can be called by interrupt routines).
 * If there is no process reading the log yet, it writes to the console also.
 */
/*VARARGS2*/
void
log (int level, char *fmt, ...)
{
	register int s = splhigh();

	logpri(level);
	prf(fmt, &fmt + 1, TOLOG, (struct tty *)0);
	splx(s);
	if (! logisopen(logMSG))
		prf(fmt, &fmt + 1, TOCONS, (struct tty *)0);
	logwakeup(logMSG);
}

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then reboots.
 * If we are called twice, then we avoid trying to
 * sync the disks as this often leads to recursive panics.
 */
void
panic(s)
	char *s;
{
	int bootopt = RB_AUTOBOOT | RB_DUMP;

	if (panicstr)
		bootopt |= RB_NOSYNC;
	else {
		panicstr = s;
	}
	printf("panic: %s\n", s);
	boot(rootdev, bootopt);
}

/*
 * Warn that a system table is full.
 */
void
tablefull (tab)
	char *tab;
{
	log(LOG_ERR, "%s: table full\n", tab);
}

/*
 * Hard error is the preface to plaintive error messages
 * about failing disk tranfers.
 */
void
harderr (bp, cp)
	struct buf *bp;
	char *cp;
{
	printf("%s%d%c: hard error sn%D ", cp,
		minor(bp->b_dev) >> 3, 'a'+(minor(bp->b_dev) & 07), bp->b_blkno);
}
