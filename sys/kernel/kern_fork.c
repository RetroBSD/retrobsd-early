/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "map.h"
#include "user.h"
#include "proc.h"
#include "inode.h"
#include "file.h"
#include "vm.h"
#include "kernel.h"

/*
 * Create a new process -- the internal version of system call fork.
 * It returns 1 in the new process, 0 in the old.
 */
int
newproc (isvfork)
	int isvfork;
{
	register struct proc *rpp, *rip;
	register int n;
	static int pidchecked = 0;
	struct file *fp;

	/*
	 * First, just locate a slot for a process
	 * and copy the useful info from this process into it.
	 * The panic "cannot happen" because fork has already
	 * checked for the existence of a slot.
	 */
	mpid++;
retry:
	if (mpid >= 30000) {
		mpid = 100;
		pidchecked = 0;
	}
	if (mpid >= pidchecked) {
		int doingzomb = 0;

		pidchecked = 30000;
		/*
		 * Scan the proc table to check whether this pid
		 * is in use.  Remember the lowest pid that's greater
		 * than mpid, so we can avoid checking for a while.
		 */
		rpp = allproc;
again:
		for (; rpp != NULL; rpp = rpp->p_nxt) {
			if (rpp->p_pid == mpid || rpp->p_pgrp == mpid) {
				mpid++;
				if (mpid >= pidchecked)
					goto retry;
			}
			if (rpp->p_pid > mpid && pidchecked > rpp->p_pid)
				pidchecked = rpp->p_pid;
			if (rpp->p_pgrp > mpid && pidchecked > rpp->p_pgrp)
				pidchecked = rpp->p_pgrp;
		}
		if (!doingzomb) {
			doingzomb = 1;
			rpp = zombproc;
			goto again;
		}
	}
	rpp = freeproc;
	if (rpp == NULL)
		panic("no procs");

	freeproc = rpp->p_nxt;			/* off freeproc */

	/*
	 * Make a proc table entry for the new process.
	 */
	rip = u.u_procp;
	rpp->p_stat = SIDL;
	rpp->p_realtimer.it_value = 0;
	rpp->p_flag = SLOAD;
	rpp->p_uid = rip->p_uid;
	rpp->p_pgrp = rip->p_pgrp;
	rpp->p_nice = rip->p_nice;
	rpp->p_pid = mpid;
	rpp->p_ppid = rip->p_pid;
	rpp->p_pptr = rip;
	rpp->p_time = 0;
	rpp->p_cpu = 0;
	rpp->p_sigmask = rip->p_sigmask;
	rpp->p_sigcatch = rip->p_sigcatch;
	rpp->p_sigignore = rip->p_sigignore;
	/* take along any pending signals like stops? */
#ifdef UCB_METER
	if (isvfork) {
		forkstat.cntvfork++;
		forkstat.sizvfork += rip->p_dsize + rip->p_ssize;
	} else {
		forkstat.cntfork++;
		forkstat.sizfork += rip->p_dsize + rip->p_ssize;
	}
#endif
	rpp->p_wchan = 0;
	rpp->p_slptime = 0;
	{
	struct proc **hash = &pidhash [PIDHASH (rpp->p_pid)];

	rpp->p_hash = *hash;
	*hash = rpp;
	}
	/*
	 * some shuffling here -- in most UNIX kernels, the allproc assign
	 * is done after grabbing the struct off of the freeproc list.  We
	 * wait so that if the clock interrupts us and vmtotal walks allproc
	 * the text pointer isn't garbage.
	 */
	rpp->p_nxt = allproc;			/* onto allproc */
	rpp->p_nxt->p_prev = &rpp->p_nxt;	/*   (allproc is never NULL) */
	rpp->p_prev = &allproc;
	allproc = rpp;

	/*
	 * Increase reference counts on shared objects.
	 */
	for (n = 0; n <= u.u_lastfile; n++) {
		fp = u.u_ofile[n];
		if (fp == NULL)
			continue;
		fp->f_count++;
	}
	u.u_cdir->i_count++;
	if (u.u_rdir)
		u.u_rdir->i_count++;

	/*
	 * When the longjmp is executed for the new process,
	 * here's where it will resume.
	 */
	if (setjmp (&u.u_ssave)) {
		return(1);
	}

	rpp->p_dsize = rip->p_dsize;
	rpp->p_ssize = rip->p_ssize;
	rpp->p_daddr = rip->p_daddr;
	rpp->p_saddr = rip->p_saddr;

	/*
	 * Partially simulate the environment of the new process so that
	 * when it is actually created (by copying) it will look right.
	 */
	u.u_procp = rpp;

	/*
	 * Swap out the current process to generate the copy.
	 */
	rip->p_stat = SIDL;
	rpp->p_addr = rip->p_addr;
	rpp->p_stat = SRUN;
	swapout (rpp, X_DONTFREE, X_OLDSIZE, X_OLDSIZE);
	rpp->p_flag |= SSWAP;
	rip->p_stat = SRUN;
	u.u_procp = rip;

	if (isvfork) {
		/*
		 * Wait for the child to finish with it.
		 */
		rip->p_dsize = 0;
		rip->p_ssize = 0;
		rpp->p_flag |= SVFORK;
		rip->p_flag |= SVFPRNT;
		while (rpp->p_flag & SVFORK)
			sleep ((caddr_t)rpp, PSWP+1);
		if ((rpp->p_flag & SLOAD) == 0)
			panic ("newproc vfork");
		u.u_dsize = rip->p_dsize = rpp->p_dsize;
		rip->p_daddr = rpp->p_daddr;
		rpp->p_dsize = 0;
		u.u_ssize = rip->p_ssize = rpp->p_ssize;
		rip->p_saddr = rpp->p_saddr;
		rpp->p_ssize = 0;
		rpp->p_flag |= SVFDONE;
		wakeup ((caddr_t) rip);
		rip->p_flag &= ~SVFPRNT;
	}
	return(0);
}

static void
fork1 (isvfork)
	int isvfork;
{
	register int a;
	register struct proc *p1, *p2;

	a = 0;
	if (u.u_uid != 0) {
		for (p1 = allproc; p1; p1 = p1->p_nxt)
			if (p1->p_uid == u.u_uid)
				a++;
		for (p1 = zombproc; p1; p1 = p1->p_nxt)
			if (p1->p_uid == u.u_uid)
				a++;
	}
	/*
	 * Disallow if
	 *  No processes at all;
	 *  not su and too many procs owned; or
	 *  not su and would take last slot.
	 */
	p2 = freeproc;
	if (p2==NULL)
		tablefull("proc");
	if (p2==NULL || (u.u_uid!=0 && (p2->p_nxt == NULL || a>MAXUPRC))) {
		u.u_error = EAGAIN;
		return;
	}
	p1 = u.u_procp;
	if (newproc (isvfork)) {
		/* Child */
		u.u_rval = 0;
		u.u_start = time.tv_sec;
		bzero(&u.u_ru, sizeof(u.u_ru));
		bzero(&u.u_cru, sizeof(u.u_cru));
		return;
	}
	/* Parent */
	u.u_rval = p2->p_pid;
}

/*
 * fork system call
 */
void
fork()
{
	fork1 (0);
}

/*
 * vfork system call, fast version of fork
 */
void
vfork()
{
	fork1 (1);
}
