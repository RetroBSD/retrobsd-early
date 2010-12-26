/*
 * Hardware register defines for MIPS32 architecture.
 *
 * Copyright (C) 2008-2010 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#ifdef PIC32MX
#   include "mch_pic32mx.h"
#endif

/*
 * Offsets of register values in saved context.
 */
#define CONTEXT_R1	0
#define CONTEXT_R2	1
#define CONTEXT_R3	2
#define CONTEXT_R4	3
#define CONTEXT_R5	4
#define CONTEXT_R6	5
#define CONTEXT_R7	6
#define CONTEXT_R8	7
#define CONTEXT_R9	8
#define CONTEXT_R10	9
#define CONTEXT_R11	10
#define CONTEXT_R12	11
#define CONTEXT_R13	12
#define CONTEXT_R14	13
#define CONTEXT_R15	14
#define CONTEXT_R16	15
#define CONTEXT_R17	16
#define CONTEXT_R18	17
#define CONTEXT_R19	18
#define CONTEXT_R20	19
#define CONTEXT_R21	20
#define CONTEXT_R22	21
#define CONTEXT_R23	22
#define CONTEXT_R24	23
#define CONTEXT_R25	24
#define CONTEXT_GP	25
#define CONTEXT_FP	26
#define CONTEXT_RA	27
#define CONTEXT_LO	28
#define CONTEXT_HI	29
#define CONTEXT_STATUS	30
#define CONTEXT_PC	31

#define CONTEXT_WORDS	32

/*
 * Exception handlers, written in C, require some space in stack
 * for local storage. Minimum requirement is 16 bytes.
 * To compute needed space, you should get disassembled code of
 * _arch_interrupt_() and look at the function entry instructions.
 * For example:
 *	addiu   sp,sp,-64
 * 	sw      ra,60(sp)
 *	...
 *	sw      s0,24(sp)
 * Number in the last instruction is the frame space, needed for
 * local storage. When it is greater than 16, you must put it
 * in a macro definition here.
 */
#ifdef ELVEES_NVCOM01
#   define MIPS_FSPACE		24	/* for Elvees NVCom-01 */
#endif
#ifdef ELVEES_NVCOM02
#   define MIPS_FSPACE		24	/* TODO: for Elvees NVCom-02 */
#endif
#ifndef MIPS_FSPACE
#   define MIPS_FSPACE		16	/* default minimum */
#endif

#ifndef __ASSEMBLER__

/*
 * Set value of stack pointer register.
 */
static void inline __attribute__ ((always_inline))
mips_set_stack_pointer (void *x)
{
	asm volatile (
	"move	$sp, %0"
	: : "r" (x) : "sp");
}

/*
 * Get value of stack pointer register.
 */
static inline __attribute__ ((always_inline))
void *mips_get_stack_pointer ()
{
	void *x;

	asm volatile (
	"move	%0, $sp"
	: "=r" (x));
	return x;
}

/*
 * Read C0 coprocessor register.
 */
#define mips_read_c0_register(reg)				\
({ int __value;							\
	asm volatile (						\
	"mfc0	%0, $%1"					\
	: "=r" (__value) : "K" (reg));				\
	__value;						\
})

/*
 * Write C0 coprocessor register.
 */
#define mips_write_c0_register(reg, value)			\
do {								\
	asm volatile (						\
	"mtc0	%z0, $%1 \n	nop \n	nop \n	nop"		\
	: : "r" ((unsigned int) (value)), "K" (reg));		\
} while (0)

/*
 * Read FPU (C1 coprocessor) register.
 */
#define mips_read_fpu_register(reg)				\
({ int __value;							\
	asm volatile (						\
	"mfc1	%0, $%1"					\
	: "=r" (__value) : "K" (reg));				\
	__value;						\
})

/*
 * Write FPU (C1 coprocessor) register.
 */
#define mips_write_fpu_register(reg, value)			\
do {								\
	asm volatile (						\
	"mtc1	%z0, $%1"					\
	: : "r" (value), "K" (reg));				\
} while (0)

/*
 * Read FPU control register.
 */
#define mips_read_fpu_control(reg)				\
({ int __value;							\
	asm volatile (						\
	"cfc1	%0, $%1"					\
	: "=r" (__value) : "K" (reg));				\
	__value;						\
})

/*
 * Write FPU control register.
 */
#define mips_write_fpu_control(reg, value)			\
do {								\
	asm volatile (						\
	"ctc1	%z0, $%1"					\
	: : "r" ((unsigned int) (value)), "K" (reg));		\
} while (0)

/*
 * Disable the hardware interrupts,
 * saving the interrupt state into the supplied variable.
 */
static void inline __attribute__ ((always_inline))
mips_intr_disable (int *x)
{
#if defined (ELVEES_MC24) || defined (ELVEES_NVCOM01) || defined (ELVEES_NVCOM02)
	/* This must be atomic operation.
	 * On MIPS1 this could be done only using system call exception. */
	asm volatile (
	"syscall \n"
"	move	%0, $a0"
	: "=r" (*x) : "K" (C0_STATUS) : "a0");
#else
	*x = mips_read_c0_register (C0_STATUS);
	asm volatile ("di");
#endif
}

/*
 * Restore the hardware interrupt mode using the saved interrupt state.
 */
static void inline __attribute__ ((always_inline))
mips_intr_restore (int x)
{
#if defined (ELVEES_MC24) || defined (ELVEES_NVCOM01) || defined (ELVEES_NVCOM02)
	int status;

	status = mips_read_c0_register (C0_STATUS);
	mips_write_c0_register (C0_STATUS, status | (x & ST_IE));
#else
	mips_write_c0_register (C0_STATUS, x);
#endif
}

/*
 * Enable hardware interrupts.
 */
static void inline __attribute__ ((always_inline))
mips_intr_enable ()
{
#if defined (ELVEES_MC24) || defined (ELVEES_NVCOM01) || defined (ELVEES_NVCOM02)
	int status;

	status = mips_read_c0_register (C0_STATUS);
	mips_write_c0_register (C0_STATUS, status | ST_IE);
#else
	asm volatile ("ei");
#endif
}

/*
 * Count a number of leading (most significant) zero bits in a word.
 */
static int inline __attribute__ ((always_inline))
mips_count_leading_zeroes (unsigned x)
{
	int n;

	asm volatile (
	"	.set	mips32 \n"
	"	clz	%0, %1"
	: "=r" (n) : "r" (x));
	return n;
}

/*
 * Translate virtual address to physical one.
 * Only for fixed mapping.
 */
static unsigned int inline
mips_virtual_addr_to_physical (unsigned int virt)
{
	unsigned segment_desc = virt >> 28;
	if (segment_desc <= 0x7) {
		// kuseg
		if (mips_read_c0_register(C0_STATUS) & ST_ERL) {
			// ERL == 1, no mapping
			return virt;
		} else {
			// Assume fixed-mapped
			return (virt + 0x40000000);
		}
	} else {
		// kseg0, or kseg1, or kseg2, or kseg3
		if (segment_desc <= 0xb) {
			// kseg0 или kseg1, cut bits A[31:29].
			return (virt & 0x1fffffff);
		} else {
			// Fixed-mapped - no mapping
			return virt;
		}
	}
}

#endif /* __ASSEMBLER__ */
