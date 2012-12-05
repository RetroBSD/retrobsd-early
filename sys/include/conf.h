/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
struct uio;
struct buf;
struct tty;

/*
 * Declaration of block device
 * switch. Each entry (row) is
 * the only link between the
 * main unix code and the driver.
 * The initialization of the
 * device switches is in the
 * file conf.c.
 */
struct bdevsw
{
	int	(*d_open) (dev_t, int, int);
	int	(*d_close) (dev_t, int, int);
	void	(*d_strategy) (struct buf*);
	void	(*d_root) (caddr_t);		/* root attach routine */
	daddr_t	(*d_psize) (dev_t);		/* query partition size */
	int	(*d_ioctl) (dev_t, u_int, caddr_t, int);
	int	d_flags;			/* tape flag */
};

/*
 * Character device switch.
 */
struct cdevsw
{
	int	(*d_open) (dev_t, int, int);
	int	(*d_close) (dev_t, int, int);
	int	(*d_read) (dev_t, struct uio*, int);
	int	(*d_write) (dev_t, struct uio*, int);
	int	(*d_ioctl) (dev_t, u_int, caddr_t, int);
	int	(*d_stop) (struct tty*, int);
	struct tty *d_ttys;
	int	(*d_select) (dev_t, int);
	void	(*d_strategy) (struct buf*);
};

/*
 * tty line control switch.
 */
struct linesw
{
	int	(*l_open) (dev_t, struct tty*);
	int	(*l_close) (struct tty*, int);
	int	(*l_read) (struct tty*, struct uio*, int);
	int	(*l_write) (struct tty*, struct uio*, int);
	int	(*l_ioctl) (struct tty*, u_int, caddr_t, int);
	void	(*l_rint) (int, struct tty*);
	void	(*l_start) (struct tty*);
	int	(*l_modem) (struct tty*, int);
};

#ifdef KERNEL
extern const struct	bdevsw bdevsw[];
extern const struct	cdevsw cdevsw[];
extern const struct	linesw linesw[];

extern int nulldev();
extern int norw(dev_t dev, struct uio *uio, int flag);
extern int noioctl(dev_t dev, u_int cmd, caddr_t data, int flag);
extern void noroot(caddr_t csr);

int rawrw (dev_t dev, struct uio *uio, int flag);
#endif
