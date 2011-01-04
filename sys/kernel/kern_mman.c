/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "proc.h"
#include "vm.h"
#include "text.h"
#include "systm.h"

void
sbrk()
{
	struct a {
		int nsiz;
	};
	register int n, d;

	/* set n to new data size */
	n = ((struct a*)u.u_ap)->nsiz;
	n -= u.u_tsize;
	if (n < 0)
		n = 0;
	if (estabur (u.u_tsize, n, u.u_ssize, 0))
		return;
	expand (n, S_DATA);
	/* set d to (new - old) */
	d = n - u.u_dsize;
	if (d > 0)
		bzero ((void*) (u.u_procp->p_daddr + u.u_dsize), d);
	u.u_dsize = n;
}

/*
 * Grow the stack to include the SP.
 * Return true if successful.
 */
int
grow(sp)
	register unsigned sp;
{
	register int si;

	if (sp >= -u.u_ssize)
		return (0);
	si = (-sp) - u.u_ssize + SINCR;
	/*
	 * Round the increment back to a segment boundary if necessary.
	 */
	if (si <= 0)
		return (0);
	if (estabur(u.u_tsize, u.u_dsize, u.u_ssize + si, 0))
		return (0);
	/*
	 *  expand will put the stack in the right place;
	 *  no copy required here.
	 */
	expand (u.u_ssize + si, S_STACK);
	u.u_ssize += si;
	bzero ((void*) u.u_procp->p_saddr, si);
	return (1);
}

/*
 * Set up software prototype segmentation registers to implement the 3
 * pseudo text, data, stack segment sizes passed as arguments.
 * The last argument determines whether the text segment is
 * read-write or read-only.
 */
int
estabur(nt, nd, ns, wflag)
	u_int nt, nd, ns;
	int wflag;
{
	if (nt + nd + ns + USIZE > maxmem) {
		u.u_error = ENOMEM;
		return (-1);
	}
	/* TODO */
	sureg();
	return(0);
}

/*
 * Load the user hardware segmentation registers from the software prototype.
 * The software registers must have been setup prior by estabur.
 */
void
sureg()
{
	int taddr, daddr, saddr;
	struct text *tp;

	daddr = u.u_procp->p_daddr;
	saddr = u.u_procp->p_saddr;
	tp = u.u_procp->p_textp;
	if (tp != NULL)
		taddr = tp->x_caddr;
	else
		taddr = daddr;
	/* TODO */
}
