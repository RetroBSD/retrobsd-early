/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifndef _SEG_
#define _SEG_

/*
 * Access abilities
 */
#define RO	02		/* Read only */
#define RW	06		/* Read and write */
#define NOACC	0		/* Abort all accesses */
#define ACCESS	07		/* Mask for access field */
#define ED	010		/* Extension direction */
#define TX	020		/* Software: text segment */
#define ABS	040		/* Software: absolute address */

#ifdef KERNEL
/* TODO */
#endif /* KERNEL */
#endif /* _SEG_ */
