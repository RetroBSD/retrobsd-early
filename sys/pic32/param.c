/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "buf.h"
#include "time.h"
#include "resource.h"
#include "proc.h"
#include "text.h"
#include "file.h"
#include "dir.h"
#include "inode.h"
#include "fs.h"
#include "mount.h"
#include "callout.h"
#include "map.h"
#include "clist.h"
#include "namei.h"

/*
 * System parameter formulae.
 *
 * This file is copied into each directory where we compile
 * the kernel; it should be modified there to suit local taste
 * if necessary.
 */
#define	MAXUSERS	4
#define	NBUF		32

int	hz = 60;
u_short	mshz = (1000000L + 60 - 1) / 60;
struct	timezone tz = { 480, 1 };

#define	NPROC (10 + 7 * MAXUSERS)
int	nproc = NPROC;

#define NTEXT (26 + MAXUSERS)
int	ntext = NTEXT;

#define NINODE ((NPROC + 16 + MAXUSERS) + 22)
int	ninode = NINODE;

#define NNAMECACHE (NINODE * 11/10)
int	nchsize = NNAMECACHE;
struct	namecache namecache [NNAMECACHE];

#define NFILE ((8 * NINODE / 10) + 20)
int	nfile = NFILE;

#define NCALL (16 + MAXUSERS)
int	ncallout = NCALL;
int	nbuf = NBUF;

#define NCLIST (20 + 8 * MAXUSERS)

#if NCLIST > (8192 / 32)		/* 8K / sizeof(struct cblock) */
#   undef NCLIST
#   define NCLIST (8192 / 32)
#endif
int	nclist = NCLIST;

/*
 * These have to be allocated somewhere; allocating
 * them here forces loader errors if this file is omitted
 * (if they've been externed everywhere else; hah!).
 */
struct	proc *procNPROC;
struct	text *textNTEXT;
struct	inode inode [NINODE], *inodeNINODE;
struct	file *fileNFILE;
struct	callout callout [NCALL];
struct	mount mount [NMOUNT];
struct	buf buf [NBUF], bfreelist [BQUEUES];
struct	bufhd bufhash [BUFHSZ];

/*
 * Remove the ifdef/endif to run the kernel in unsecure mode even when in
 * a multiuser state.  Normally 'init' raises the security level to 1
 * upon transitioning to multiuser.  Setting the securelevel to -1 prevents
 * the secure level from being raised by init.
*/
#ifdef	PERMANENTLY_INSECURE
int	securelevel = -1;
#endif

struct cblock	cfree[NCLIST];
int ucb_clist = 0;

#define CMAPSIZ	NPROC			/* size of core allocation map */
#define SMAPSIZ	((9 * NPROC) / 10)	/* size of swap allocation map */

struct mapent	_coremap[CMAPSIZ];
struct map	coremap[1] = {
	{ _coremap,
	  &_coremap[CMAPSIZ],
	  "coremap" },
};

struct mapent	_swapmap[SMAPSIZ];
struct map	swapmap[1] = {
	{ _swapmap,
	  &_swapmap[SMAPSIZ],
	  "swapmap" },
};

struct proc	proc [NPROC];
struct file	file [NFILE];
struct text	text [NTEXT];

/* TODO: use linker script*/
#include "user.h"
struct user u;
