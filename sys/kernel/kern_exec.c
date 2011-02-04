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
#include "buf.h"
#include "inode.h"
#include "namei.h"
#include "fs.h"
#include "mount.h"
#include "file.h"
#include "signalvar.h"

/*
 * exec system call, with and without environments.
 */
struct execa {
	char	*fname;
	char	**argp;
	char	**envp;
};

/*
 * Reset signals for an exec of the specified process.  In 4.4 this function
 * was in kern_sig.c but since in 2.11 kern_sig and kern_exec will likely be
 * in different overlays placing this here potentially saves a kernel overlay
 * switch.
 */
static void
execsigs(p)
	register struct proc *p;
{
	register int nc;
	unsigned long mask;

	/*
	 * Reset caught signals.  Held signals remain held
	 * through p_sigmask (unless they were caught,
	 * and are now ignored by default).
	 */
	while (p->p_sigcatch) {
		nc = ffs(p->p_sigcatch);
		mask = sigmask(nc);
		p->p_sigcatch &= ~mask;
		if (sigprop[nc] & SA_IGNORE) {
			if (nc != SIGCONT)
				p->p_sigignore |= mask;
			p->p_sig &= ~mask;
		}
		u.u_signal[nc] = SIG_DFL;
	}
	/*
	 * Reset stack state to the user stack (disable the alternate stack).
	 */
	u.u_sigstk.ss_flags = SA_DISABLE;
	u.u_sigstk.ss_size = 0;
	u.u_sigstk.ss_base = 0;
	u.u_psflags = 0;
}

/*
 * Read in and set up memory for executed file.
 * u.u_error set on error
 */
static void
getxfile (ip, ep, nargc, uid, gid)
	struct inode *ip;
	register struct exec *ep;
	int nargc, uid, gid;
{
	long lsize;
	off_t offset;
	u_int ds, ts, ss;

	switch (ep->a_magic) {
	case A_MAGIC1:
		lsize = (long) ep->a_data + ep->a_text;
		ep->a_data = (u_int) lsize;
		if (lsize != ep->a_data) {	/* check overflow */
			u.u_error = ENOMEM;
			return;
		}
		ep->a_text = 0;
		break;
	}

	if (ep->a_text != 0 && (ip->i_flag & ITEXT) == 0 &&
	    ip->i_count != 1) {
		register struct file *fp;

		for (fp = file; fp < file+NFILE; fp++) {
			if (fp->f_type == DTYPE_INODE &&
			    fp->f_count > 0 &&
			    (struct inode*)fp->f_data == ip &&
			    (fp->f_flag & FWRITE)) {
				u.u_error = ETXTBSY;
				return;
			}
		}
	}

	/*
	 * find text and data sizes try; them out for possible
	 * overflow of max sizes
	 */
	ts = ep->a_text;
	lsize = (long) ep->a_data + ep->a_bss;
	if (lsize != (u_int) lsize) {
		u.u_error = ENOMEM;
		return;
	}
	ds = lsize;
	ss = SSIZE + nargc;

	if (estabur (ts, ds, ss, 0)) {
		return;
	}

	/*
	 * allocate and clear core at this point, committed
	 * to the new image
	 */
	u.u_prof.pr_scale = 0;
	if (u.u_procp->p_flag & SVFORK)
		endvfork();
	expand (ds, S_DATA);
	{
		register u_int numc, startc;

		startc = ep->a_data;		/* clear BSS only */
		if (startc != 0)
			startc--;
		numc = ds - startc;
		bzero ((void*) (u.u_procp->p_daddr + startc), numc);
	}
	expand (ss, S_STACK);
	bzero ((void*) u.u_procp->p_saddr, ss);

	/*
	 * read in data segment
	 */
	estabur (0, ds, 0, 0);
	offset = sizeof(struct exec);
	offset += ep->a_text;
	rdwri (UIO_READ, ip, (caddr_t) 0, ep->a_data, offset,
		IO_UNIT, (int*) 0);

	/*
	 * set SUID/SGID protections, if no tracing
	 */
	if ((u.u_procp->p_flag & P_TRACED) == 0) {
		u.u_uid = uid;
		u.u_procp->p_uid = uid;
		u.u_groups[0] = gid;
	} else
		psignal (u.u_procp, SIGTRAP);
	u.u_svuid = u.u_uid;
	u.u_svgid = u.u_groups[0];

	u.u_tsize = ts;
	u.u_dsize = ds;
	u.u_ssize = ss;
	estabur (ts, ds, ss, 0);
}

void
execv()
{
	((struct execa *)u.u_ap)->envp = NULL;
	execve();
}

void
execve()
{
	int nc;
	register char *cp;
	register struct buf *bp;
	struct execa *uap = (struct execa *)u.u_ap;
	int na, ne, ucp, ap;
	register int cc;
	unsigned len;
	int indir, uid, gid;
	char *sharg;
	struct inode *ip;
	memaddr bno;
	char cfname [MAXCOMLEN + 1];
#define	SHSIZE	32
	char cfarg [SHSIZE];
	union {
		char	ex_shell [SHSIZE];	/* #! and name of interpreter */
		struct	exec ex_exec;
	} exdata;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;
	int resid, error;

	NDINIT (ndp, LOOKUP, FOLLOW, uap->fname);
	ip = namei (ndp);
	if (ip == NULL)
		return;
	bno = 0;
	bp = 0;
	indir = 0;
	uid = u.u_uid;
	gid = u.u_groups[0];
	if (ip->i_fs->fs_flags & MNT_NOEXEC) {
		u.u_error = EACCES;
		goto bad;
	}
	if ((ip->i_fs->fs_flags & MNT_NOSUID) == 0) {
		if (ip->i_mode & ISUID)
			uid = ip->i_uid;
		if (ip->i_mode & ISGID)
			gid = ip->i_gid;
	}
again:
	if (access (ip, IEXEC))
		goto bad;
	if ((u.u_procp->p_flag & P_TRACED) && access (ip, IREAD))
		goto bad;
	if ((ip->i_mode & IFMT) != IFREG ||
	    (ip->i_mode & (IEXEC | (IEXEC>>3) | (IEXEC>>6))) == 0) {
		u.u_error = EACCES;
		goto bad;
	}

	/*
	 * Read in first few bytes of file for segment sizes, magic number:
	 *	407 = plain executable
	 *	410 = RO text
	 *	411 = separated I/D
	 *	405 = text overlay
	 *	430 = auto-overlay (nonseparate)
	 *	431 = auto-overlay (separate)
	 * Also an ASCII line beginning with #! is
	 * the file name of a ``shell'' and arguments may be prepended
	 * to the argument list if given here.
	 *
	 * SHELL NAMES ARE LIMITED IN LENGTH.
	 *
	 * ONLY ONE ARGUMENT MAY BE PASSED TO THE SHELL FROM
	 * THE ASCII LINE.
	 */
	exdata.ex_shell[0] = '\0';	/* for zero length files */
	u.u_error = rdwri (UIO_READ, ip, (caddr_t) &exdata, sizeof(exdata),
				(off_t) 0, IO_UNIT, &resid);
	if (u.u_error)
		goto bad;
	if (resid > sizeof(exdata) - sizeof(exdata.ex_exec) &&
	    exdata.ex_shell[0] != '#') {
		u.u_error = ENOEXEC;
		goto bad;
	}

	switch ((int) exdata.ex_exec.a_magic) {
	case A_MAGIC1:
	case A_MAGIC2:
	case A_MAGIC3:
	case A_MAGIC4:
	case A_MAGIC5:
	case A_MAGIC6:
		break;
	default:
		if (exdata.ex_shell[0] != '#' ||
		    exdata.ex_shell[1] != '!' ||
		    indir) {
			u.u_error = ENOEXEC;
			goto bad;
		}
		/*
		 * If setuid/gid scripts were to be disallowed this is where it would
		 * have to be done.
		 * u.u_uid = uid;
		 * u.u_gid = u_groups[0];
		 */
		cp = &exdata.ex_shell[2];		/* skip "#!" */
		while (cp < &exdata.ex_shell[SHSIZE]) {
			if (*cp == '\t')
				*cp = ' ';
			else if (*cp == '\n') {
				*cp = '\0';
				break;
			}
			cp++;
		}
		if (*cp != '\0') {
			u.u_error = ENOEXEC;
			goto bad;
		}
		cp = &exdata.ex_shell[2];
		while (*cp == ' ')
			cp++;
		ndp->ni_dirp = cp;
		while (*cp && *cp != ' ')
			cp++;
		cfarg[0] = '\0';
		if (*cp) {
			*cp++ = '\0';
			while (*cp == ' ')
				cp++;
			if (*cp)
				bcopy ((caddr_t) cp, (caddr_t) cfarg, SHSIZE);
		}
		indir = 1;
		iput (ip);
		ndp->ni_nameiop = LOOKUP | FOLLOW;
		ip = namei (ndp);
		if (ip == NULL)
			return;
		bcopy ((caddr_t) ndp->ni_dent.d_name, (caddr_t) cfname, MAXCOMLEN);
		cfname [MAXCOMLEN] = '\0';
		goto again;
	}

	/*
	 * Collect arguments on "file" in swap space.
	 */
	na = 0;
	ne = 0;
	nc = 0;
	cc = 0;
	cp = 0;
	bno = malloc (swapmap, btod (NCARGS + MAXBSIZE));
	if (bno == 0) {
		swkill (u.u_procp, "exec");
		goto bad;
	}
	/*
	 * Copy arguments into file in argdev area.
	 */
	if (uap->argp) for (;;) {
		ap = NULL;
		sharg = NULL;
		if (indir && na == 0) {
			sharg = cfname;
			ap = (int) sharg;
			uap->argp++;		/* ignore argv[0] */
		} else if (indir && (na == 1 && cfarg[0])) {
			sharg = cfarg;
			ap = (int) sharg;
		} else if (indir && (na == 1 || (na == 2 && cfarg[0])))
			ap = (int) uap->fname;
		else if (uap->argp) {
			ap = *(int*) uap->argp;
			uap->argp++;
		}
		if (ap == NULL && uap->envp) {
			uap->argp = NULL;
			ap = *(int*) uap->envp;
			if (ap != NULL)
				uap->envp++, ne++;
		}
		if (ap == NULL)
			break;
		na++;
		if (ap == -1) {
			u.u_error = EFAULT;
			break;
		}
		do {
			if (cc <= 0) {
				/*
				 * We depend on NCARGS being a multiple of
				 * DEV_BSIZE.  This way we need only check
				 * overflow before each buffer allocation.
				 */
				if (nc >= NCARGS-1) {
					error = E2BIG;
					break;
				}
				if (bp) {
					bdwrite(bp);
				}
				cc = DEV_BSIZE;
				bp = getblk (swapdev, dbtofsb(bno) + lblkno(nc));
				cp = bp->b_addr;
			}
			if (sharg) {
				error = copystr (sharg, cp, (unsigned) cc, &len);
				sharg += len;
			} else {
				error = copystr ((caddr_t) ap, cp, (unsigned) cc, &len);
				ap += len;
			}
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENOENT);
		if (error) {
			u.u_error = error;
			if (bp) {
				bp->b_flags |= B_AGE;
				bp->b_flags &= ~B_DELWRI;
				brelse(bp);
			}
			bp = 0;
			goto badarg;
		}
	}
	if (bp) {
		bdwrite (bp);
	}
	bp = 0;
	nc = (nc + NBPW-1) & ~(NBPW-1);
	getxfile (ip, &exdata.ex_exec, nc + (na+4)*NBPW, uid, gid);
	if (u.u_error) {
badarg:
		for (cc = 0; cc < nc; cc += DEV_BSIZE) {
			daddr_t blkno;

			blkno = dbtofsb(bno) + lblkno(cc);
			if (incore (swapdev, blkno)) {
				bp = bread (swapdev, blkno);
				bp->b_flags |= B_AGE;		/* throw away */
				bp->b_flags &= ~B_DELWRI;	/* cancel io */
				brelse(bp);
				bp = 0;
			}
		}
		goto bad;
	}
	iput(ip);
	ip = NULL;

	/*
	 * Copy back arglist.
	 */
	ucp = -nc - NBPW;
	ap = ucp - na*NBPW - 3*NBPW;
	u.u_frame [FRAME_SP] = ap;
	*(int*) ap = na - ne;
	nc = 0;
	cc = 0;
	for (;;) {
		ap += NBPW;
		if (na == ne) {
			*(int*) ap = 0;
			ap += NBPW;
		}
		if (--na < 0)
			break;
		*(int*) ap = ucp;
		do {
			if (cc <= 0) {
				if (bp) {
					brelse(bp);
				}
				cc = DEV_BSIZE;
				bp = bread (swapdev, dbtofsb(bno) + lblkno(nc));
				bp->b_flags |= B_AGE;		/* throw away */
				bp->b_flags &= ~B_DELWRI;	/* cancel io */
				cp = bp->b_addr;
			}
			error = copystr (cp, (caddr_t) ucp, (unsigned) cc,
				&len);
			ucp += len;
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENOENT);
		if (error == EFAULT)
			panic ("exec: EFAULT");
	}
	*(int*) ap = 0;
	*(int*) (-NBPW) = 0;
	if (bp) {
		bp->b_flags |= B_AGE;
		brelse (bp);
		bp = NULL;
	}
	execsigs (u.u_procp);
	for (cp = u.u_pofile, cc = 0; cc <= u.u_lastfile; cc++, cp++) {
		if (*cp & UF_EXCLOSE) {
			(void) closef (u.u_ofile [cc]);
			u.u_ofile [cc] = NULL;
			*cp = 0;
		}
	}
	while (u.u_lastfile >= 0 && u.u_ofile [u.u_lastfile] == NULL)
		u.u_lastfile--;

	/*
	 * Clear registers.
	 */
	u.u_frame [FRAME_PC] = exdata.ex_exec.a_entry & ~01;

	/*
	 * Remember file name for accounting.
	 */
	if (indir)
		bcopy ((caddr_t) cfname, (caddr_t) u.u_comm, MAXCOMLEN);
	else
		bcopy ((caddr_t) ndp->ni_dent.d_name, (caddr_t) u.u_comm, MAXCOMLEN);
bad:
	if (bp) {
		bp->b_flags |= B_AGE;
		brelse (bp);
	}
	if (bno)
		mfree (swapmap, btod (NCARGS + MAXBSIZE), bno);
	if (ip)
		iput(ip);
}
