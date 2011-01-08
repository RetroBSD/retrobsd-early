/*
 * Machine dependent constants for MIPS32.
 *
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifndef ENDIAN
#include "machine/io.h"

/*
 * Definitions for byte order,
 * according to byte significance from low address to high.
 */
#define	LITTLE		1234		/* least-significant byte first (vax) */
#define	BIG		4321		/* most-significant byte first */
#define	PDP		3412		/* LSB first in word, MSW first in long (pdp) */
#define	ENDIAN		LITTLE		/* byte order on pic32 */

#define	CHAR_BIT	NBBY
#define	CHAR_MAX	0x7f
#define	CHAR_MIN	0x80
#define	INT_MAX		0x7fffffff
#define	INT_MIN		0x80000000
#define	LONG_MAX	0x7fffffff
#define	LONG_MIN	0x80000000
#define	SCHAR_MAX	0x7f
#define	SCHAR_MIN	0x80
#define	SHRT_MAX	0x7fff
#define	SHRT_MIN	0x8000
#define	UCHAR_MAX	0xff
#define	UINT_MAX	((unsigned int)0xffffffff)
#define	ULONG_MAX	0x7fffffff
#define	USHRT_MAX	((unsigned short)0xffff)

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ``long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */
#define	MAXSLP 		20

/*
 * Disk blocks.
 */
#define	DEV_BSIZE	1024		/* the same as MAXBSIZE */
#define	DEV_BSHIFT	10		/* log2(DEV_BSIZE) */
#define	DEV_BMASK	(DEV_BSIZE-1)

/* Bytes to disk blocks */
#define	btod(x)		(((x) + DEV_BSIZE-1) >> DEV_BSHIFT)

/*
 * On PIC32, there are total 512 kbytes of flash and 128 kbytes of RAM.
 * We reserve for kernel 192 kbytes of flash and 32 kbytes of RAM.
 */
#define FLASH_SIZE		(512*1024)
#define DATA_SIZE		(128*1024)

#define KERNEL_FLASH_SIZE	(128*1024)
#define KERNEL_DATA_SIZE	(32*1024)

#define USER_FLASH_START	(0x1d000000 + KERNEL_FLASH_SIZE)
#define USER_FLASH_END		(0x1d000000 + FLASH_SIZE)

#define USER_DATA_START		(0x00000000 + KERNEL_DATA_SIZE)
#define USER_DATA_END		(0x00000000 + DATA_SIZE)

/*
 * User area: a user structure, followed by a stack for the networking code
 * (running in supervisor space on the PDP-11), followed by the kernel
 * stack.  The number for KERN_SSIZE is determined empirically.
 *
 * Note that the SBASE and STOP constants are only used by the assembly code,
 * but are defined here to localize information about the user area's
 * layout (see pdp/genassym.c).  Note also that a networking stack is always
 * allocated even for non-networking systems.  This prevents problems with
 * applications having to be recompiled for networking versus non-networking
 * systems.
 */
#define	KERN_SSIZE	2048		/* size of the kernel stack (bytes) */
#define	USIZE		(sizeof(struct user) + KERN_SSIZE)

#define	SSIZE		2048		/* initial stack size (bytes) */
#define	SINCR		2048		/* increment of stack (bytes) */

/*
 * Macros to decode processor status word.
 */
#define	SUPVMODE(ps)	(((ps) & PSL_CURMOD) == PSL_CURSUP)
#define	USERMODE(ps)	(((ps) & PSL_USERSET) == PSL_USERSET)
#define	BASEPRI(ps)	(((ps) & PSL_IPL) == 0)

#define	splbio()	mips_intr_disable ()
#define	spltty()	mips_intr_disable ()
#define	splclock()	mips_intr_disable ()
#define	splhigh()	mips_intr_disable ()
#define	splnet()	mips_intr_enable ()
#define	splsoftclock()	mips_intr_enable ()
#define	spl0()		mips_intr_enable ()
#define	splx(s)		mips_intr_restore (s)

#define	noop()		asm volatile ("nop")
#define	idle()		asm volatile ("wait")

#ifdef KERNEL
/*
 * Microsecond delay routine.
 */
void udelay (unsigned usec);

/*
 * Setup system timer for `hz' timer interrupts per second.
 */
void clkstart (void);

#endif

#endif /* ENDIAN */
