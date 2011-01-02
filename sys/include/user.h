/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifdef KERNEL
#include "machine/fperr.h"
#include "dir.h"
#include "exec.h"
#include "time.h"
#include "resource.h"
#else
#include <machine/fperr.h>
#include <sys/dir.h>
#include <sys/exec.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

/*
 * data that doesn't need to be referenced while the process is swapped.
 * The user block is USIZE*64 bytes long; resides at virtual kernel loc
 * 0140000; contains the system stack (and possibly network stack) per
 * user; is cross referenced with the proc structure for the same process.
 */
#define	MAXCOMLEN	MAXNAMLEN	/* <= MAXNAMLEN, >= sizeof(ac_comm) */

struct	pcb {			/* fake pcb structure */
	int	(*pcb_sigc)();	/* pointer to trampoline code in user space */
};

struct user {
	struct	pcb u_pcb;
	struct	proc *u_procp;		/* pointer to proc structure */
	int	*u_ar0;			/* address of users saved R0 */
	char	u_comm[MAXCOMLEN + 1];

/* syscall parameters, results and catches */
	int	u_arg[6];		/* arguments to current system call */
	int	*u_ap;			/* pointer to arglist */
	label_t	u_qsave;		/* for non-local gotos on interrupts */
	union {				/* syscall return values */
		struct	{
			int	R_val1;
			int	R_val2;
		} u_rv;
#define	r_val1	u_rv.R_val1
#define	r_val2	u_rv.R_val2
		long	r_long;
		off_t	r_off;
		time_t	r_time;
	} u_r;
	int	u_error;		/* return error code */

/* 1.1 - processes and protection */
	uid_t	u_uid;			/* effective user id */
	uid_t	u_svuid;		/* saved user id */
	uid_t	u_ruid;			/* real user id */
	gid_t	u_svgid;		/* saved group id */
	gid_t	u_rgid;			/* real group id */
	gid_t	u_groups[NGROUPS];	/* groups, 0 terminated */

/* 1.2 - memory management */
	size_t	u_tsize;		/* text size (clicks) */
	size_t	u_dsize;		/* data size (clicks) */
	size_t	u_ssize;		/* stack size (clicks) */
	label_t	u_ssave;		/* label variable for swapping */
	label_t	u_rsave;		/* save info when exchanging stacks */

/* 1.3 - signal management */
	int	(*u_signal[NSIG])();	/* disposition of signals */
	long	u_sigmask[NSIG];	/* signals to be blocked */
	long	u_sigonstack;		/* signals to take on sigstack */
	long	u_sigintr;		/* signals that interrupt syscalls */
	long	u_oldmask;		/* saved mask from before sigpause */
	int	u_code;			/* ``code'' to trap */
	int	u_psflags;		/* Process Signal flags */
	struct	sigaltstack u_sigstk;	/* signal stack info */

/* 1.4 - descriptor management */
	struct	file *u_ofile[NOFILE];	/* file structures for open files */
	char	u_pofile[NOFILE];	/* per-process flags of open files */
	int	u_lastfile;		/* high-water mark of u_ofile */
#define	UF_EXCLOSE 	0x1		/* auto-close on exec */
#define	UF_MAPPED 	0x2		/* mapped from device */
	struct	inode *u_cdir;		/* current directory */
	struct	inode *u_rdir;		/* root directory of current process */
	struct	tty *u_ttyp;		/* controlling tty pointer */
	dev_t	u_ttyd;			/* controlling tty dev */
	int	u_cmask;		/* mask for file creation */

/* 1.5 - timing and statistics */
	struct	k_rusage u_ru;		/* stats for this proc */
	struct	k_rusage u_cru;		/* sum of stats for reaped children */
	struct	k_itimerval u_timer[2];	/* profile/virtual timers */
	long	u_start;
	int	u_dupfd;		/* XXX - see kern_descrip.c/fdopen */

	struct uprof {			/* profile arguments */
		unsigned *pr_base;	/* buffer base */
		unsigned pr_size;	/* buffer size */
		unsigned pr_off;	/* pc offset */
		unsigned pr_scale;	/* pc scaling */
	} u_prof;

/* 1.6 - resource controls */
	struct	rlimit u_rlimit[RLIM_NLIMITS];

/* namei & co. */
	struct	nameicache {		/* last successful directory search */
		off_t nc_prevoffset;	/* offset at which last entry found */
		ino_t nc_inumber;	/* inum of cached directory */
		dev_t nc_dev;		/* dev of cached directory */
	} u_ncache;
	int	u_stack[1];		/* kernel stack per user
					 * extends from u + USIZE*64
					 * backward not to reach here */
};

#include <sys/errno.h>

#ifdef KERNEL
extern	struct user u;
#endif
