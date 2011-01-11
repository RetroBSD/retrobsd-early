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
#include "file.h"
#include "clist.h"
#include "callout.h"
#include "reboot.h"
#include "msgbuf.h"
#include "namei.h"
#include "disklabel.h"
#include "mount.h"
#include "systm.h"

int	hz = HZ;
int	usechz = (1000000L + HZ - 1) / HZ;
struct	timezone tz = { 11*60, 1 };

struct	namecache namecache [NNAMECACHE];
char	bufdata [NBUF * MAXBSIZE];
struct	inode inode [NINODE];
struct	callout callout [NCALL];
struct	mount mount [NMOUNT];
struct	buf buf [NBUF], bfreelist [BQUEUES];
struct	bufhd bufhash [BUFHSZ];
struct	cblock cfree [NCLIST];
struct proc	proc [NPROC];
struct file	file [NFILE];

/* TODO: use linker script*/
struct user u0, u1;
struct user *uarea = &u0;

/*
 * Remove the ifdef/endif to run the kernel in unsecure mode even when in
 * a multiuser state.  Normally 'init' raises the security level to 1
 * upon transitioning to multiuser.  Setting the securelevel to -1 prevents
 * the secure level from being raised by init.
 */
#ifdef	PERMANENTLY_INSECURE
int	securelevel = -1;
#endif

struct mapent	_swapmap[SMAPSIZ];
struct map	swapmap[1] = {
	{ _swapmap,
	  &_swapmap[SMAPSIZ],
	  "swapmap" },
};

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
	physmem = 128 * 1024;	/* TODO */
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
		register struct buf *bp;
		int iter, nbusy;

		waittime = 0;
		printf("syncing disks... ");
		(void) splnet();
		sync();
		for (iter = 0; iter < 20; iter++) {
			nbusy = 0;
			for (bp = &buf[NBUF]; --bp >= buf; )
				if (bp->b_flags & B_BUSY)
					nbusy++;
			if (nbusy == 0)
				break;
			printf ("%d ", nbusy);
			udelay (40000L * iter);
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
 * Find the index of the least significant set bit in the 32-bit word.
 * If LSB bit is set - return 1.
 * If only MSB bit is set - return 32.
 * Return 0 when no bit is set.
 */
int
ffs (i)
	u_long i;
{
	if (i != 0)
		i = 32 - mips_count_leading_zeroes (i & -i);
	return i;
}

/*
 * Copy a null terminated string from one point to another.
 * Returns zero on success, ENOENT if maxlength exceeded.
 * If lencopied is non-zero, *lencopied gets the length of the copy
 * (including the null terminating byte).
 */
int
copystr (src, dest, maxlength, lencopied)
	register caddr_t src, dest;
	register u_int maxlength, *lencopied;
{
	caddr_t dest0 = dest;
	int error = ENOENT;

	if (maxlength != 0) {
		while ((*dest++ = *src++) != '\0') {
			if (--maxlength == 0) {
				/* Failed. */
				goto done;
			}
		}
		/* Succeeded. */
		error = 0;
	}
done:	if (lencopied != 0)
		*lencopied = dest - dest0;
	return error;
}

/*
 * Calculate the length of a string.
 */
size_t
strlen (s)
	register const char *s;
{
	const char *s0 = s;

	while (*s++ != '\0')
		;
	return s - s0 - 1;
}

/*
 * Return 0 if a user address is valid.
 * There are two memory regions, allowed for user: flash and RAM.
 */
int
baduaddr (addr)
	register caddr_t addr;
{
	if (addr >= (caddr_t) USER_FLASH_START &&
	    addr < (caddr_t) USER_FLASH_END)
		return 0;
	if (addr >= (caddr_t) USER_DATA_START &&
	    addr < (caddr_t) USER_DATA_END)
		return 0;
	return 1;
}

/*
 * Insert the specified element into a queue immediately after
 * the specified predecessor element.
 */
void insque (void *element, void *predecessor)
{
	struct que {
		struct que *q_next;
		struct que *q_prev;
	};
	register struct que *e = (struct que *) element;
	register struct que *prev = (struct que *) predecessor;

	e->q_prev = prev;
	e->q_next = prev->q_next;
	prev->q_next->q_prev = e;
	prev->q_next = e;
}

/*
 * Remove the specified element from the queue.
 */
void remque (void *element)
{
	struct que {
		struct que *q_next;
		struct que *q_prev;
	};
	register struct que *e = (struct que *) element;

	e->q_prev->q_next = e->q_next;
	e->q_next->q_prev = e->q_prev;
}

/* Nonzero if pointer is not aligned on a "sz" boundary.  */
#define UNALIGNED(p, sz)	((unsigned) (p) & ((sz) - 1))

/*
 * Copy data from the memory region pointed to by src0 to the memory
 * region pointed to by dst0.
 * If the regions overlap, the behavior is undefined.
 */
void
bcopy (const void *src0, void *dst0, size_t nbytes)
{
	unsigned char *dst = dst0;
	const unsigned char *src = src0;
	unsigned *aligned_dst;
	const unsigned *aligned_src;

	/* If the size is small, or either SRC or DST is unaligned,
	 * then punt into the byte copy loop.  This should be rare.  */
	if (nbytes >= 4*sizeof(unsigned) &&
	    ! UNALIGNED (src, sizeof(unsigned)) &&
	    ! UNALIGNED (dst, sizeof(unsigned))) {
		aligned_dst = (unsigned*) dst;
		aligned_src = (const unsigned*) src;

		/* Copy 4X unsigned words at a time if possible.  */
		while (nbytes >= 4*sizeof(unsigned)) {
			*aligned_dst++ = *aligned_src++;
			*aligned_dst++ = *aligned_src++;
			*aligned_dst++ = *aligned_src++;
			*aligned_dst++ = *aligned_src++;
			nbytes -= 4*sizeof(unsigned);
		}

		/* Copy one unsigned word at a time if possible.  */
		while (nbytes >= sizeof(unsigned)) {
			*aligned_dst++ = *aligned_src++;
			nbytes -= sizeof(unsigned);
		}

		/* Pick up any residual with a byte copier.  */
		dst = (unsigned char*) aligned_dst;
		src = (const unsigned char*) aligned_src;
	}

	while (nbytes--)
		*dst++ = *src++;
}

/*
 * Fill the array with zeroes.
 */
void
bzero (void *dst0, size_t nbytes)
{
	unsigned char *dst;
	unsigned *aligned_dst;

	dst = (unsigned char*) dst0;
	while (UNALIGNED (dst, sizeof(unsigned))) {
		*dst++ = 0;
		if (--nbytes == 0)
			return;
	}
	if (nbytes >= sizeof(unsigned)) {
		/* If we get this far, we know that nbytes is large and dst is word-aligned. */
		aligned_dst = (unsigned*) dst;

		while (nbytes >= 4*sizeof(unsigned)) {
			*aligned_dst++ = 0;
			*aligned_dst++ = 0;
			*aligned_dst++ = 0;
			*aligned_dst++ = 0;
			nbytes -= 4*sizeof(unsigned);
		}
		while (nbytes >= sizeof(unsigned)) {
			*aligned_dst++ = 0;
			nbytes -= sizeof(unsigned);
		}
		dst = (unsigned char*) aligned_dst;
	}

	/* Pick up the remainder with a bytewise loop.  */
	while (nbytes--)
		*dst++ = 0;
}

/*
 * Compare not more than nbytes of data pointed to by m1 with
 * the data pointed to by m2. Return an integer greater than, equal to or
 * less than zero according to whether the object pointed to by
 * m1 is greater than, equal to or less than the object
 * pointed to by m2.
 */
int
bcmp (const void *m1, const void *m2, size_t nbytes)
{
	const unsigned char *s1 = (const unsigned char*) m1;
	const unsigned char *s2 = (const unsigned char*) m2;
	const unsigned *aligned1, *aligned2;

	/* If the size is too small, or either pointer is unaligned,
	 * then we punt to the byte compare loop.  Hopefully this will
	 * not turn up in inner loops.  */
	if (nbytes >= 4*sizeof(unsigned) &&
	    ! UNALIGNED (s1, sizeof(unsigned)) &&
	    ! UNALIGNED (s2, sizeof(unsigned))) {
		/* Otherwise, load and compare the blocks of memory one
		   word at a time.  */
		aligned1 = (const unsigned*) s1;
		aligned2 = (const unsigned*) s2;
		while (nbytes >= sizeof(unsigned)) {
			if (*aligned1 != *aligned2)
				break;
			aligned1++;
			aligned2++;
			nbytes -= sizeof(unsigned);
		}

		/* check remaining characters */
		s1 = (const unsigned char*) aligned1;
		s2 = (const unsigned char*) aligned2;
	}
	while (nbytes--) {
		if (*s1 != *s2)
			return *s1 - *s2;
		s1++;
		s2++;
	}
	return 0;
}

int
copyout (caddr_t from, caddr_t to, u_int nbytes)
{
	if (baduaddr (to) || baduaddr (to + nbytes - 1))
		return EFAULT;
	bcopy (from, to, nbytes);
	return 0;
}

int copyin (caddr_t from, caddr_t to, u_int nbytes)
{
	if (baduaddr (from) || baduaddr (from + nbytes - 1))
		return EFAULT;
	bcopy (from, to, nbytes);
	return 0;
}
