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
#include "systm.h"
#include "msgbuf.h"
#include "namei.h"
#include "disklabel.h"
#include "mount.h"

size_t	physmem;	/* total amount of physical memory (for savecore) */

#define	NLABELS	6

memaddr	_dlabelbase;
int	_dlabelnum = NLABELS;

int waittime = -1;

static void
initdisklabels()
{
	_dlabelbase = malloc (coremap, NLABELS * sizeof (struct disklabel));
}

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
	 * Allocate the initial disklabels.
	 */
	(void) initdisklabels();

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i=1; i<ncallout; i++)
		callout[i-1].c_next = &callout[i];
}

/*
 * Dumpsys takes a dump of memory by calling (*dump)(), which must
 * correspond to dumpdev.  *(dump)() should dump from dumplo blocks
 * to the end of memory or to the end of the logical device.
 */
void
dumpsys()
{
	extern int (*dump)();
	register int error;

	if (dumpdev != NODEV) {
		printf("\ndumping to dev %o off %D\ndump ",dumpdev,dumplo);
		error = (*dump)(dumpdev);
		switch(error) {

		case EFAULT:
			printf("dev !ready:EFAULT\n");
			break;
		case EINVAL:
			printf("args:EINVAL\n");
			break;
		case EIO:
			printf("err:EIO\n");
			break;
		default:
			printf("unknown err:%d\n",error);
			break;
		case 0:
			printf("succeeded\n");
			break;
		}
	}
}

void
boot(dev, howto)
	register dev_t dev;
	register int howto;
{
	register struct fs *fp;

	/*
	 * Force the root filesystem's superblock to be updated,
	 * so the date will be as current as possible after
	 * rebooting.
	 */
	fp = getfs(rootdev);
	if (fp)
		fp->fs_fmod = 1;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0 && bfreelist[0].b_forw) {
		waittime = 0;
		printf("syncing disks... ");
		(void) _splnet();
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
			printf("%d ", nbusy);
			delay(40000L * iter);
		  }
		}
		printf("done\n");
	}
	(void) _splhigh();
	if (howto & RB_HALT) {
		printf("halting\n");
		halt();
		/*NOTREACHED*/
	} else {
		if (howto & RB_DUMP) {
			/*
			 * save the registers in low core.
			 */
			saveregs();
			dumpsys();
		}
		doboot(dev, howto);
		/*NOTREACHED*/
	}
}

memaddr
disklabelalloc()
{
	register memaddr base;

	if (--_dlabelnum) {
		base = _dlabelbase;
		_dlabelbase += sizeof (struct disklabel);
		return(base);
	}
	base = malloc (coremap, sizeof (struct disklabel));
	return(base);
}
