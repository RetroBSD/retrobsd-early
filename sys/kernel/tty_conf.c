/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "file.h"
#include "ioctl.h"
#include "tty.h"
#include "errno.h"
#include "conf.h"

struct	linesw linesw[] = {
{ ttyopen,	ttylclose,	ttread,		ttwrite,	/* 0- NTTYDISC */
  0,		ttyinput,	ttstart,	ttymodem },
};

int	nldisp = sizeof (linesw) / sizeof (linesw[0]);
