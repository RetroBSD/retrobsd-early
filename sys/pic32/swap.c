/*
 * Simple proxy for swap partition.  Forwards requests for /dev/swap on to the
 * device specified by swapdev
 */

#include "param.h"
#include "systm.h"
#include "buf.h"
#include "errno.h"
#include "dk.h"
#include "uio.h"
#include "conf.h"
#include "fcntl.h"
#include "map.h"
#include "swap.h"

extern struct buf *getnewbuf();

static unsigned int tdpos;
static unsigned int tdsize;	// Number of blocks allocated
static unsigned int tdstart = 0;

extern dev_t swapdev;
extern int physio(void (*strat) (struct buf*),
			register struct buf *bp,
			dev_t dev,
			int rw,
			register struct uio *uio);

extern void swap(size_t blkno, size_t coreaddr, register int count, int rdflg);

void swstrategy(register struct buf *bp)
{
	dev_t od = bp->b_dev;
	bp->b_dev = swapdev;
	bdevsw[major(od)].d_strategy(bp);
}

int swopen(dev_t dev, int mode, int flag)
{
	return bdevsw[major(swapdev)].d_open(swapdev,mode,flag);
}

int swclose(dev_t dev, int mode, int flag)
{
	return bdevsw[major(swapdev)].d_close(swapdev,mode,flag);
}

daddr_t swsize(dev_t dev)
{
	return bdevsw[major(dev)].d_psize(dev);
}

int swcopen(dev_t dev, int mode, int flag)
{
	printf("temp open with modes ");
	if(mode & FREAD) printf("FREAD ");
	if(mode & FWRITE) printf("FWRITE ");
	if(mode & O_NONBLOCK) printf("O_NONBLOCK ");
	if(mode & O_APPEND) printf("O_APPEND ");
	if(mode & O_SHLOCK) printf("O_SHLOCK ");
	if(mode & O_EXLOCK) printf("O_EXLOCK ");
	if(mode & O_ASYNC) printf("O_ASYNC ");
	if(mode & O_FSYNC) printf("O_FSYNC ");
	if(mode & O_CREAT) printf("O_CREAT ");
	if(mode & O_TRUNC) printf("O_TRUNC ");
	if(mode & O_EXCL) printf("O_EXCL ");
	if(mode & FMARK) printf("FMARK ");
	if(mode & FDEFER) printf("FDEFER ");
	printf("and flag %04X\n",flag);

	if(mode & O_TRUNC)
	{
		tdpos = 0;
	}

	return 0;
}

int swcclose(dev_t dev, int mode, int flag)
{
	printf("temp close with mode %d and flag %d\n");
	return 0;
}

int swcread(dev_t dev, register struct uio *uio, int flag)
{
	unsigned int block;
	unsigned int boff;
	struct buf *bp;
	unsigned int rsize;
	unsigned int rlen;

	if(tdstart == 0)
		return EIO;

	if(tdpos+uio->uio_offset >= tdsize<<10)
		return EIO;

	tdpos += uio->uio_offset;

	bp = getnewbuf();

	block = tdpos >> 10;
	boff = tdpos - (block << 10);

	rsize = 1024 - boff;
	rlen = uio->uio_iov->iov_len;

	while(rlen>0 && (block < tdsize))
	{
		swap(tdstart + block,(size_t)bp->b_addr,1024,B_READ);
		uiomove(bp->b_addr+boff,rsize,uio);
		boff = 0;
		block++;
		tdpos += rsize;
		rlen -= rsize;
		rsize = rlen >= 1024 ? 1024 : rlen;
	}
	
	printf("temp read %u\n",uio->uio_offset);
	brelse(bp);
	return 0;
}

int swcwrite(dev_t dev, register struct uio *uio, int flag)
{
	//unsigned int block;
	//unsigned int boff;

	if(tdstart == 0)
		return EIO;

	if(tdpos+uio->uio_offset > tdsize<<10)
		return EIO;
	tdpos += uio->uio_offset;

	//block = tdpos >> 10;
	//boff = tdpos - (block << 10);

	printf("temp write %u\n",uio->uio_offset);
	return 0;
}

int swcioctl (dev_t dev, register u_int cmd, caddr_t addr, int flag)
{
	printf("temp ioctl %c %d with address %p\n",
	(cmd>>8) & 0xFF,
	cmd & 0xFF,
	addr);

	unsigned int *uival;

	uival = (unsigned int *)addr;

	switch(cmd)
	{
		case TFALLOC:
			if(tdstart>0)
				mfree(swapmap,tdsize,tdstart);
			tdstart = malloc(swapmap,*uival);
			if(tdstart>0)
			{
				tdsize = *uival;
				printf("%u blocks allocated from block %d\n",
					uival,tdstart);
			}
	}
	return 0;
}
