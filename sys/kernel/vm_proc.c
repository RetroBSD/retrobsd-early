/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "proc.h"
#include "map.h"
#include "kernel.h"
#include "systm.h"

/*
 * Change the size of the data+stack regions of the process.
 * The data and stack segments are separated from each
 * other.  The second argument to expand specifies which to change.  The
 * stack segment will not have to be copied again after expansion.
 */
void
expand (newsize, segment)
	int newsize, segment;
{
	/*
	 * We have a fixed memory mapping with only one process in-core.
	 * So there is no need to reallocate and copy a memory.
	 */
	register struct proc *p;

	p = u.u_procp;
	if (segment == S_DATA) {
		p->p_dsize = newsize;
		p->p_daddr = USER_DATA_START;
	} else {
		p->p_ssize = newsize;
		p->p_saddr = USER_DATA_END - newsize;
	}
	sureg();
}
