/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "ioctl.h"
#include "proc.h"
#include "kernel.h"
#include "file.h"
#include "inode.h"
#include "sysctl.h"
#include "cpu.h"
#include "tty.h"
#include "systm.h"

/*
 * ucall allows user level code to call various kernel functions.
 * Autoconfig uses it to call the probe and attach routines of the
 * various device drivers.
 */
void
ucall()
{
	register struct a {
		int priority;
		int (*routine)();
		int arg1;
		int arg2;
	} *uap = (struct a *)u.u_arg;
	int s;

	if (!suser())
		return;
	switch (uap->priority) {
	case 0:
		s = spl0();
		break;
	default:
		s = splhigh();
		break;
	}
	u.u_rval = (*uap->routine) (uap->arg1, uap->arg2);
	splx(s);
}

/*
 * Lock user into core as much as possible.  Swapping may still
 * occur if core grows.
 */
void
lock()
{
	struct a {
		int	flag;
	};

	if (!suser())
		return;
	if (((struct a *)u.u_arg)->flag)
		u.u_procp->p_flag |= SULOCK;
	else
		u.u_procp->p_flag &= ~SULOCK;
}

/*
 * fetch the word at iaddr from flash memory.  This system call is
 * required on PIC32 because in user mode the access to flash memory
 * region is not allowed.
 */
void
fetchi()
{
	u.u_rval = *(unsigned*) u.u_arg;
}

/*
 * This was moved here when the TMSCP portion was added.  At that time it
 * became (even more) system specific and didn't belong in kern_sysctl.c
 */
int
cpu_sysctl (name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{

	switch (name[0]) {
	case CPU_CONSDEV:
		if (namelen != 1)
			return (ENOTDIR);
		return (sysctl_rdstruct (oldp, oldlenp, newp,
				&cnttys[0].t_dev, sizeof &cnttys[0].t_dev));
#if NTMSCP > 0
	case CPU_TMSCP:
		/* All sysctl names at this level are terminal */
		if (namelen != 2)
			return(ENOTDIR);
		switch (name[1]) {
		case TMSCP_CACHE:
			return (sysctl_int (oldp, oldlenp, newp,
				newlen, &tmscpcache));
		case TMSCP_PRINTF:
			return (sysctl_int (oldp, oldlenp, newp,
				newlen, &tmscpprintf));
		default:
			return (EOPNOTSUPP);
		}
#endif
#if NRAC > 0
	case CPU_MSCP:
		/* All sysctl names at this level are terminal */
		if (namelen != 2)
			return(ENOTDIR);
		switch (name[1]) {
			extern	int mscpprintf;
		case MSCP_PRINTF:
			return (sysctl_int(oldp, oldlenp, newp,
					newlen, &mscpprintf));
		default:
			return (EOPNOTSUPP);
		}
#endif
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
