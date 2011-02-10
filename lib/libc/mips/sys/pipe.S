/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "SYS.h"

ENTRY(pipe)
		.set	noreorder
		syscall	SYS_pipe
		j	1f		# return here on error
		nop
		.set	reorder

		sw	$v0, ($a0)	# return here on success
		sw	$v1, ($a1)
		move	$v0, $zero
		jr	$ra
1:
		sw	$t0, errno
		jr	$ra
