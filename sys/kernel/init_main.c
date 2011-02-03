/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "fs.h"
#include "mount.h"
#include "map.h"
#include "proc.h"
#include "ioctl.h"
#include "inode.h"
#include "conf.h"
#include "buf.h"
#include "fcntl.h"
#include "vm.h"
#include "clist.h"
#include "reboot.h"
#include "systm.h"
#include "kernel.h"
#include "namei.h"
#include "disklabel.h"
#include "stat.h"

/*
 * Initialize hash links for buffers.
 */
static void
bhinit()
{
	register int i;
	register struct bufhd *bp;

	for (bp = bufhash, i = 0; i < BUFHSZ; i++, bp++)
		bp->b_forw = bp->b_back = (struct buf *)bp;
}

/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 */
static void
binit()
{
	register struct buf *bp;
	register int i;
	caddr_t paddr;

	for (bp = bfreelist; bp < &bfreelist[BQUEUES]; bp++)
		bp->b_forw = bp->b_back = bp->av_forw = bp->av_back = bp;
	paddr = bufdata;
	for (i = 0; i < NBUF; i++, paddr += MAXBSIZE) {
		bp = &buf[i];
		bp->b_dev = NODEV;
		bp->b_bcount = 0;
		bp->b_addr = paddr;
		binshash(bp, &bfreelist[BQ_AGE]);
		bp->b_flags = B_BUSY|B_INVAL;
		brelse(bp);
	}
}

/*
 * Initialize clist by freeing all character blocks, then count
 * number of character devices. (Once-only routine)
 */
static void
cinit()
{
	register int ccp;
	register struct cblock *cp;

	ccp = (int)cfree;
	ccp = (ccp + CROUND) & ~CROUND;
	for (cp = (struct cblock *)ccp; cp <= &cfree[NCLIST - 1]; cp++) {
		cp->c_next = cfreelist;
		cfreelist = cp;
		cfreecount += CBSIZE;
	}
}

/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *	clear and free user core
 *	turn on clock
 *	hand craft 0th process
 *	call all initialization routines
 *	fork - process 0 to schedule
 *	     - process 1 execute bootstrap
 */
int
main()
{
	register struct proc *p;
	register int i;
	register struct fs *fs;
	daddr_t swsize;
	struct partinfo dpart;
	int (*ioctl) (dev_t, u_int, caddr_t, int);

	startup();
	printf ("\n%s", version);
	printf ("phys mem  = %u kbytes\n", physmem / 1024);
	printf ("user mem  = %u kbytes\n", MAXMEM / 1024);

	/*
	 * Set up system process 0 (swapper).
	 */
	p = &proc[0];
	p->p_addr = (memaddr) &u;
	p->p_stat = SRUN;
	p->p_flag |= SLOAD | SSYS;
	p->p_nice = NZERO;

	u.u_procp = p;			/* init user structure */
	u.u_ap = u.u_arg;
	u.u_cmask = CMASK;
	u.u_lastfile = -1;
	for (i = 1; i < NGROUPS; i++)
		u.u_groups[i] = NOGROUP;
	for (i = 0; i < sizeof(u.u_rlimit)/sizeof(u.u_rlimit[0]); i++)
		u.u_rlimit[i].rlim_cur = u.u_rlimit[i].rlim_max =
		    RLIM_INFINITY;

	/* Initialize signal state for process 0 */
	siginit (p);

	/*
	 * Initialize tables, protocols, and set up well-known inodes.
	 */
	loginit();
	coutinit();
	cinit();
	pqinit();
	ihinit();
	bhinit();
	binit();
	nchinit();
	clkstart();

	/*
	 * Now we find out how much swap space is available.
	 * We toss away/ignore 1 sector of swap space (because a 0 value
	 * can not be placed in a resource map).
	 */
	printf ("swap dev  = (%d,%d)\n", major(swapdev), minor(swapdev));
	if ((*bdevsw[major(swapdev)].d_open) (swapdev, FREAD | FWRITE, S_IFBLK) != 0)
		panic ("cannot open swapdev");
	swsize = (*bdevsw[major(swapdev)].d_psize) (swapdev);
	printf ("swap size = %u kbytes\n", swsize * DEV_BSIZE / 1024);
	if (swsize <= 0)
		panic ("zero swap size");	/* don't want to panic, but what ? */

	/*
	 * Next we make sure that we do not swap on a partition unless it is of
	 * type FS_SWAP.  If the driver does not have an ioctl entry point or if
	 * retrieving the partition information fails then the driver does not
	 * support labels and we proceed normally, otherwise the partition must be
	 * a swap partition (so that we do not swap on top of a filesystem by mistake).
	 */
	ioctl = cdevsw[blktochr(swapdev)].d_ioctl;
	if (ioctl && (*ioctl) (swapdev, DIOCGPART, (caddr_t) &dpart, FREAD) == 0) {
		if (dpart.part->p_fstype != FS_SWAP)
			panic ("invalid swap partition");
	}
	nswap = swsize;
	mfree (swapmap, --nswap, 1);

	/* Mount a root filesystem. */
	printf ("root dev  = (%d,%d)\n", major(rootdev), minor(rootdev));
	fs = mountfs (rootdev, (boothowto & RB_RDONLY) ? MNT_RDONLY : 0,
			(struct inode*) 0);
	if (! fs)
		panic ("no root filesystem");
	mount[0].m_inodp = (struct inode*) 1;	/* XXX */
	mount_updname (fs, "/", "root", 1, 4);
	time.tv_sec = fs->fs_time;
	boottime = time;

	/* Kick off timeout driven events by calling first time. */
	schedcpu (0);

	/* Set up the root file system. */
	rootdir = iget (rootdev, &mount[0].m_filsys, (ino_t) ROOTINO);
	iunlock (rootdir);
	u.u_cdir = iget (rootdev, &mount[0].m_filsys, (ino_t) ROOTINO);
	iunlock (u.u_cdir);
	u.u_rdir = NULL;

	/*
	 * Make init process.
	 */
	if (newproc (0)) {
		expand (icodeend - icode, S_DATA);
		expand (1024, S_STACK);			/* one kbyte of stack */
		estabur (0, icodeend - icode, 1024, 0);
		copyout ((caddr_t) icode, (caddr_t) USER_DATA_START, icodeend - icode);
		/*
		 * return goes to location 0 of user init code
		 * just copied out.
		 */
		return 0;
	}
	sched();
	return 0;
}
