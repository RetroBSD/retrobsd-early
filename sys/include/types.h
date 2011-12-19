/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef _SYS_TYPES_
#define	_SYS_TYPES_

/*
 * Basic system types and major/minor device constructing/busting macros.
 */

/* major part of a device */
#define	major(x)	((int)(((int)(x)>>8)&0377))

/* minor part of a device */
#define	minor(x)	((int)((x)&0377))

/* make a device number */
#define	makedev(x,y)	((dev_t)(((x)<<8) | (y)))

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef unsigned long	u_long;		/* see this! unsigned longs at last! */
typedef	unsigned short	ushort;		/* sys III compat */

#ifdef pdp11
typedef	struct	label_t	{
	int	val[7];			/* regs 2-7 and super SP */
} label_t;
#endif
#ifdef __mips__
typedef	struct	label_t	{
	unsigned val[12];		/* regs S0-S8, RA, GP and SP */
} label_t;
#endif
typedef	long	daddr_t;
typedef	char *	caddr_t;
typedef	u_int	ino_t;
#ifndef _SIZE_T
typedef	u_int	size_t;
#endif
#ifndef __ssize_t_defined
typedef int     ssize_t;
#define __ssize_t_defined
#endif
typedef	long	time_t;
typedef	int	dev_t;
#ifndef _SIZE_T
typedef	long	off_t;
#endif
typedef	u_int	uid_t;
typedef	u_int	gid_t;
typedef	int	pid_t;
typedef	u_int	mode_t;
typedef int	bool_t;		/* boolean */

#define	NBBY	8		/* number of bits in a byte */

#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

#include <sys/select.h>

#ifdef KERNEL
/*
 * Save the process' current register context.
 */
int setjmp (label_t *env);

/*
 * Map in a user structure and jump to a saved context.
 */
void longjmp (size_t unew, label_t *env);

#endif /* KERNEL */

#endif
