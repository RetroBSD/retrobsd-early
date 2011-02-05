/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)SYS.h	1.5 (2.11BSD GTE) 1995/05/08
 */
#include <syscall.h>

			.globl	x_norm, x_error

//#define	SYSCALL(s, r)	.globl _/**/x; \
//		_/**/x:	syscall	SYS_/**/s; \
//			EXIT_/**/r

#define	SYSCALL_norm(s) \
			.globl s; \
		     s:	syscall	SYS_##s; \
			j	x_norm;
			nop

#define	SYSCALL_error(s) \
			.globl s; \
		     s:	syscall	SYS_##s; \
			j	x_error;
			nop

#define	SYSCALL_noerror(s) \
			.globl s; \
		     s:	syscall	SYS_##s; \
			j	pc; \
			nop
