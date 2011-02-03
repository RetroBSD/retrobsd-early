/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "signalvar.h"
#include "systm.h"
#include "user.h"
#include "proc.h"
#include "vm.h"

static void
dumpregs (frame)
	int *frame;
{
	unsigned int cause;
	const char *code = 0;

	printf ("\n*** 0x%08x: exception ", frame [CONTEXT_PC]);

	cause = mips_read_c0_register (C0_CAUSE);
	switch (cause & CA_EXC_CODE) {
	case CA_Int:	code = "Interrupt"; break;
	case CA_AdEL:	code = "Address Load"; break;
	case CA_AdES:	code = "Address Save"; break;
	case CA_IBE:	code = "Bus fetch"; break;
	case CA_DBE:	code = "Bus load/store"; break;
	case CA_Sys:	code = "Syscall"; break;
	case CA_Bp:	code = "Breakpoint"; break;
	case CA_RI:	code = "Reserved Instruction"; break;
	case CA_CPU:	code = "Coprocessor Unusable"; break;
	case CA_Ov:	code = "Arithmetic Overflow"; break;
	case CA_Tr:	code = "Trap"; break;
	}
	if (code)
		printf ("'%s'\n", code);
	else
		printf ("%d\n", cause >> 2 & 31);

	printf ("*** badvaddr = 0x%08x\n",
		mips_read_c0_register (C0_BADVADDR));

	printf ("                t0 = %8x   s0 = %8x   t8 = %8x   lo = %8x\n",
		frame [CONTEXT_R8], frame [CONTEXT_R16],
		frame [CONTEXT_R24], frame [CONTEXT_LO]);
	printf ("at = %8x   t1 = %8x   s1 = %8x   t9 = %8x   hi = %8x\n",
		frame [CONTEXT_R1], frame [CONTEXT_R9], frame [CONTEXT_R17],
		frame [CONTEXT_R25], frame [CONTEXT_HI]);
	printf ("v0 = %8x   t2 = %8x   s2 = %8x               status = %8x\n",
		frame [CONTEXT_R2], frame [CONTEXT_R10],
		frame [CONTEXT_R18], frame [CONTEXT_STATUS]);
	printf ("v1 = %8x   t3 = %8x   s3 = %8x                cause = %8x\n",
		frame [CONTEXT_R3], frame [CONTEXT_R11],
		frame [CONTEXT_R19], cause);
	printf ("a0 = %8x   t4 = %8x   s4 = %8x   gp = %8x  epc = %8x\n",
		frame [CONTEXT_R4], frame [CONTEXT_R12],
		frame [CONTEXT_R20], frame [CONTEXT_GP], frame [CONTEXT_PC]);
	printf ("a1 = %8x   t5 = %8x   s5 = %8x   sp = %8x\n",
		frame [CONTEXT_R5], frame [CONTEXT_R13],
		frame [CONTEXT_R21], frame [CONTEXT_SP]);
	printf ("a2 = %8x   t6 = %8x   s6 = %8x   fp = %8x\n",
		frame [CONTEXT_R6], frame [CONTEXT_R14],
		frame [CONTEXT_R22], frame [CONTEXT_FP]);
	printf ("a3 = %8x   t7 = %8x   s7 = %8x   ra = %8x\n",
		frame [CONTEXT_R7], frame [CONTEXT_R15],
		frame [CONTEXT_R23], frame [CONTEXT_RA]);
}

/*
 * User mode flag added to cause code if exception is from user space.
 */
#define	USER		1

/*
 * Called from startup.S when a processor exception occurs.
 * The argument is the array of registers saved on the system stack
 * by the hardware and software during the exception processing.
 */
void
exception (frame)
	int *frame;
{
	static int once_thru = 0;
	register int i;
	register struct proc *p;
	time_t syst;

#ifdef UCB_METER
	cnt.v_trap++;
#endif
	/*
	 * In order to stop the system from destroying
	 * kernel data space any further, allow only one
	 * fatal exception. After once_thru is set, any
	 * futher exceptions will be handled by looping in place.
	 */
	if (once_thru) {
		(void) splhigh();
		for (;;)
			asm volatile ("wait");
	}

	unsigned cause = mips_read_c0_register (C0_CAUSE);
	cause &= CA_EXC_CODE;

	unsigned ps = frame [CONTEXT_STATUS];
	if (USERMODE(ps))
		cause |= USER;

	syst = u.u_ru.ru_stime;
	p = u.u_procp;
	u.u_ar0 = frame;
	switch (cause) {

	/*
	 * Exception not expected.  Usually a kernel mode bus error.
	 */
	default:
		once_thru = 1;
		i = splhigh();
		dumpregs (frame);
		splx (i);
		panic ("unexpected exception");
		/*NOTREACHED*/

	case CA_IBE + USER:			/* Bus error, instruction fetch */
	case CA_DBE + USER:			/* Bus error, load or store */
		i = SIGBUS;
		break;

	case CA_RI + USER:		/* Reserved instruction */
		i = SIGILL;
		u.u_code = frame [CONTEXT_PC];
		break;

	case CA_Bp + USER:		/* Breakpoint */
		i = SIGTRAP;
		break;

	case CA_Tr + USER:		/* Trap */
		i = SIGIOT;
		break;

	case CA_CPU + USER:		/* Coprocessor unusable */
		i = SIGEMT;
		break;

	case CA_Ov:			/* Arithmetic overflow */
	case CA_Ov + USER:
		i = SIGFPE;
		u.u_code = frame [CONTEXT_PC];
		break;

	/*
	 * If the user SP is below the stack segment, grow the stack
	 * automatically.
	 */
	case CA_AdEL + USER:		/* Address error, load or instruction fetch */
	case CA_AdES + USER:		/* Address error, store */
//		if (! (u.u_sigstk.ss_flags & SA_ONSTACK) && grow ((u_int) sp))
//			goto out;
		i = SIGSEGV;
		break;

	/*
	 * Hardware interrupt.
	 */
	case CA_Int:			/* Interrupt */
	case CA_Int + USER:
		/* TODO */
		goto out;

	/*
	 * System call.
	 */
	case CA_Sys + USER:		/* Syscall */
#ifdef UCB_METER
		cnt.v_syscall++;
#endif
		u.u_error = 0;

		/* original pc for restarting syscalls */
		int opc = frame [CONTEXT_PC] - 4;	/* opc now points at syscall */

		const struct sysent *callp;
		int code = *(u_int*) opc & 0377;	/* bottom 8 bits are index */
		if (code >= nsysent)
			callp = &sysent[0];		/* indir (illegal) */
		else
			callp = &sysent[code];

		if (callp->sy_narg)
			copyin ((caddr_t) (frame [CONTEXT_SP] + 4),
				(caddr_t) u.u_arg, callp->sy_narg*NBPW);
		u.u_r.r_val1 = 0;
		u.u_r.r_val2 = frame [CONTEXT_R3];		/* $v1 */
		if (setjmp (&u.u_qsave) == 0) {
			(*callp->sy_call) ();
		}
		frame [CONTEXT_R8] = u.u_error;			/* $t0 */
		switch (u.u_error) {
		case 0:
			frame [CONTEXT_R2] = u.u_r.r_val1;	/* $v0 */
			frame [CONTEXT_R3] = u.u_r.r_val2;	/* $v1 */
			break;
		case ERESTART:
			frame [CONTEXT_PC] = opc;
			break;
		}
		goto out;
	}
	psignal (p, i);
out:
	/* Process all received signals. */
	for (;;) {
		i = CURSIG (p);
		if (i <= 0)
			break;
		postsig (i);
	}
	curpri = setpri(p);
	if (runrun) {
		setrq (u.u_procp);
		u.u_ru.ru_nivcsw++;
		swtch();
	}
	if (u.u_prof.pr_scale)
		addupc ((caddr_t) frame [CONTEXT_PC],
			&u.u_prof, (int) (u.u_ru.ru_stime - syst));
}

/*
 * nonexistent system call-- signal process (may want to handle it)
 * flag error if process won't see signal immediately
 * Q: should we do that all the time ??
 */
void
nosys()
{
	if (u.u_signal[SIGSYS] == SIG_IGN || u.u_signal[SIGSYS] == SIG_HOLD)
		u.u_error = EINVAL;
	psignal (u.u_procp, SIGSYS);
}
