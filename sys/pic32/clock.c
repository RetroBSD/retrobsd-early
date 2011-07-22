/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "machine/pic32mx.h"

/*
 * Setup core timer for `hz' timer interrupts per second.
 */
void
clkstart()
{
	unsigned count = mips_read_c0_register (C0_COUNT);

	mips_write_c0_register (C0_COMPARE, count + (KHZ * 1000 / HZ + 1) / 2);

	IECSET(0) = 1 << PIC32_IRQ_CT;
}
