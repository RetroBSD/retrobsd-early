/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
struct uio;

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
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_strategy)();
	int	(*d_root)();		/* XXX root attach routine */
	daddr_t	(*d_psize)();
	int	d_flags;
};

#if defined(KERNEL) && !defined(SUPERVISOR)
extern struct	bdevsw bdevsw[];
#endif

/*
 * Character device switch.
 */
struct cdevsw
{
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_read)();
	int	(*d_write)();
	int	(*d_ioctl)();
	int	(*d_stop)();
	struct tty *d_ttys;
	int	(*d_select)();
	int	(*d_strategy)();
};

#if defined(KERNEL) && !defined(SUPERVISOR)
extern struct	cdevsw cdevsw[];
#endif

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

#if defined(KERNEL) && !defined(SUPERVISOR)
extern struct	linesw linesw[];
#endif
