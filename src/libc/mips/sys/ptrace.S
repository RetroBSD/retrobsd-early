/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "SYS.h"

ENTRY(ptrace)
		sw	$zero, errno		# -1 is a legitimate return
						# value so we must clear errno
		.set	noreorder		# so the caller may disambiguate
		syscall	SYS_ptrace
		lui	$t1, %hi(errno)		# return here on error
		sw      $t0, %lo(errno)($t1)
		.set	reorder

		jr	$ra			# return here on success
