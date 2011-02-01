/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ttydev.h	7.1 (Berkeley) 6/4/86
 */

/*
 * Terminal definitions related to underlying hardware.
 */
#ifndef _TTYDEV_
#define	_TTYDEV_

/*
 * Speeds
 */
#define B0	0
#define B50	1
#define B75	2
#define B150	3
#define B200	4
#define B300	5
#define B600	6
#define B1200	7
#define	B1800	8
#define B2400	9
#define B4800	10
#define B9600	11
#define B19200	12
#define B38400	13
#define B57600	14
#define B115200	15

#ifdef KERNEL
/*
 * Modem control commands.
 */
#define	DMSET		0
#define	DMBIS		1
#define	DMBIC		2
#define	DMGET		3
#endif
#endif
