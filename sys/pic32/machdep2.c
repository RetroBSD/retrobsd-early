/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "dir.h"
#include "inode.h"
#include "user.h"
#include "proc.h"
#include "fs.h"
#include "map.h"
#include "buf.h"
#include "text.h"
#include "file.h"
#include "clist.h"
#include "callout.h"
#include "reboot.h"
#include "msgbuf.h"
#include "namei.h"
#include "disklabel.h"
#include "mount.h"
#include "systm.h"

int waittime = -1;

static int
nodump (dev)
	dev_t dev;
{
	printf ("\ndumping to dev %o off %D: not implemented\n", dumpdev, dumplo);
	return (0);
}

int (*dump) (dev_t) = nodump;

dev_t	rootdev = makedev(0,0),
	swapdev = makedev(0,1),
	pipedev = makedev(0,0);

dev_t	dumpdev = NODEV;
daddr_t	dumplo = (daddr_t) 1024;

/*
 * Machine dependent startup code
 */
void
startup()
{
	int i;

	printf("\n%s\n", version);

	/* TODO */
	physmem = MAXMEM;

	procNPROC = proc + nproc;
	textNTEXT = text + ntext;
	inodeNINODE = inode + ninode;
	fileNFILE = file + nfile;

	bpaddr = malloc (coremap, (size_t) ((long) nbuf * MAXBSIZE));
	if (bpaddr == 0)
		panic("buffers");

	/*
	 * Now initialize the log driver (kernel logger, error logger and accounting)
	 */
	loginit();

	for (i=0; i<NMOUNT; i++)
		mount[i].m_extern = (memaddr) malloc (coremap, sizeof(struct xmount));

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i=1; i<ncallout; i++)
		callout[i-1].c_next = &callout[i];
}

void
boot (dev, howto)
	register dev_t dev;
	register int howto;
{
	register struct fs *fp;

	/*
	 * Force the root filesystem's superblock to be updated,
	 * so the date will be as current as possible after
	 * rebooting.
	 */
	fp = getfs (rootdev);
	if (fp)
		fp->fs_fmod = 1;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0 && bfreelist[0].b_forw) {
		waittime = 0;
		printf("syncing disks... ");
		(void) splnet();
		/*
		 * Release inodes held by texts before update.
		 */
		xumount(NODEV);
		sync();
		{ register struct buf *bp;
		  int iter, nbusy;

		  for (iter = 0; iter < 20; iter++) {
			nbusy = 0;
			for (bp = &buf[nbuf]; --bp >= buf; )
				if (bp->b_flags & B_BUSY)
					nbusy++;
			if (nbusy == 0)
				break;
			printf ("%d ", nbusy);
			udelay (40000L * iter);
		  }
		}
		printf("done\n");
	}
	(void) splhigh();
	if (howto & RB_HALT) {
		printf ("halting\n");
		for (;;) {
			asm volatile ("wait");
		}
		/*NOTREACHED*/
	}
	if ((howto & RB_DUMP) && dumpdev != NODEV) {
		/*
		 * Take a dump of memory by calling (*dump)(),
		 * which must correspond to dumpdev.
		 * It should dump from dumplo blocks to the end
		 * of memory or to the end of the logical device.
		 */
		(*dump) (dumpdev);
	}
	/* TODO: doboot (dev, howto); */
	for (;;) {
		asm volatile ("wait");
	}
	/*NOTREACHED*/
}

/*
 * Microsecond delay routine for MIPS processor.
 */
void
udelay (usec)
	u_int usec;
{
	unsigned now = mips_read_c0_register (C0_COUNT);
	unsigned final = now + usec * (KHZ / 1000);

	for (;;) {
		now = mips_read_c0_register (C0_COUNT);

		/* This comparison is valid only when using a signed type. */
		if ((int) (now - final) >= 0)
			break;
	}
}

/*
 * Increment user profiling counters.
 */
void addupc (caddr_t pc, struct uprof *pbuf, int ticks)
{
	/* TODO */
}

/*
 * Save the process' current register context.
 */
int setjmp (label_t *env)
{
	/* TODO */
	return 0;
}

/*
 * Map in a user structure and jump to a saved context.
 */
void longjmp (memaddr u, label_t *env)
{
	/* TODO */
}
