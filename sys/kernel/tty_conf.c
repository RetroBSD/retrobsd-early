/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty_conf.c	1.2 (2.11BSD GTE) 11/30/94
 */

#include "param.h"
#include "machine/seg.h"

#include "file.h"
#include "ioctl.h"
#include "tty.h"
#include "errno.h"
#include "conf.h"

int	nodev();
int	nulldev();

int	ttyopen(),ttylclose(),ttread(),ttwrite(),nullioctl(),ttstart();
int	ttymodem(), nullmodem(), ttyinput();

/* #include "bk.h" */
#if NBK > 0
int	bkopen(),bkclose(),bkread(),bkinput(),bkioctl();
#endif

/* #include "tb.h" */
#if NTB > 0
int	tbopen(),tbclose(),tbread(),tbinput(),tbioctl();
#endif
#include "sl.h"
#if NSL > 0
int	SLOPEN(),SLCLOSE(),SLINPUT(),SLTIOCTL(),SLSTART();
#endif


struct	linesw linesw[] =
{
	ttyopen, ttylclose, ttread, ttwrite, nullioctl,	/* 0- OTTYDISC */
	ttyinput, nodev, nulldev, ttstart, ttymodem,
#if NBK > 0
	bkopen, bkclose, bkread, ttwrite, bkioctl,	/* 1- NETLDISC */
	bkinput, nodev, nulldev, ttstart, nullmodem,
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
	ttyopen, ttylclose, ttread, ttwrite, nullioctl,	/* 2- NTTYDISC */
	ttyinput, nodev, nulldev, ttstart, ttymodem,
#if NTB > 0
	tbopen, tbclose, tbread, nodev, tbioctl,
	tbinput, nodev, nulldev, ttstart, nullmodem,	/* 3- TABLDISC */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
#if NSL > 0
	SLOPEN, SLCLOSE, nodev, nodev, SLTIOCTL,
	SLINPUT, nodev, nulldev, SLSTART, nulldev,	/* 4- SLIPDISC */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
};

int	nldisp = sizeof (linesw) / sizeof (linesw[0]);

/*
 * Do nothing specific version of line
 * discipline specific ioctl command.
 */
/*ARGSUSED*/
nullioctl(tp, cmd, data, flags)
	struct tty *tp;
	u_int cmd;
	char *data;
	int flags;
{

#ifdef lint
	tp = tp; data = data; flags = flags;
#endif
	return (-1);
}

#if NSL > 0
SLOPEN(dev, tp)
	dev_t dev;
	struct tty *tp;
{
	int error, slopen();

	if (!suser())
		return (EPERM);
	if (tp->t_line == SLIPDISC)
		return (EBUSY);
	error = slopen (dev, tp);
	if (!error)
		ttyflush(tp, FREAD | FWRITE);
	return(error);
}

SLCLOSE(tp, flag)
	struct tty *tp;
	int flag;
{
	int slclose();

	ttywflush(tp);
	tp->t_line = 0;
	slclose (tp);
}

SLTIOCTL(tp, cmd, data, flag)
	struct tty *tp;
	int cmd, flag;
	caddr_t data;
{
	int sltioctl();

	return sltioctl (tp, cmd, data, flag));
}

SLSTART(tp)
	struct tty *tp;
{
	mapinfo map;
	void slstart();

	savemap(map);
	slstart (tp);
	restormap(map);
}

SLINPUT(c, tp)
	int c;
	struct tty *tp;
{
	mapinfo map;
	void slinput();

	savemap(map);
	slinput (c, tp);
	restormap(map);
}
#endif
