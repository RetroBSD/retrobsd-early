/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "machine/psl.h"
#include "machine/reg.h"
#include "signalvar.h"
#include "systm.h"
#include "user.h"
#include "proc.h"
#include "vm.h"

/*
 * Trap type values
 */
#define	T_BUSFLT	000		/* bus error */
#define	T_INSTRAP	001		/* illegal instruction */
#define	T_BPTTRAP	002		/* bpt/trace trap */
#define	T_IOTTRAP	003		/* iot trap */
#define	T_POWRFAIL	004		/* power failure */
#define	T_EMTTRAP	005		/* emt trap */
#define	T_SYSCALL	006		/* system call */
#define	T_PIRQ		007		/* program interrupt request */
#define	T_ARITHTRAP	010		/* floating point trap */
#define	T_SEGFLT	011		/* segmentation fault */
#define	T_PARITYFLT	012		/* parity fault */
#define	T_SWITCHTRAP	014		/* process switch */
/*
 * T_RANDOMTRAP is used by autoconfig/do_config.c when it substitutes
 * the trap routine for the standard device interrupt routines when
 * probing a device in case the device probe routine causes an interrupt.
 * Ignored in trap.c
 */
#define	T_RANDOMTRAP	016		/* random trap */
#define	T_ZEROTRAP	017		/* trap to zero */

/*
 * User mode flag added to trap code passed to trap routines
 * if trap is from user space.
 */
#define	USER		1

/*
 * Offsets of the user's registers relative to
 * the saved r0.  See sys/reg.h.
 */
const int regloc[] = {
	R0, R1, R2, R3, R4, R5, R6, R7, RPS
};

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
	 * fatal trap. After once_thru is set, any
	 * futher traps will be handled by looping in place.
	 */
	if (once_thru) {
		(void) splhigh();
		for (;;)
			continue;
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
	 * Trap not expected.  Usually a kernel mode bus error.
	 */
	default:
		once_thru = 1;
		i = splhigh();
		printf ("STATUS %08x\n", ps);
		printf ("EPC    %08x\n", frame [CONTEXT_PC]);
		printf ("CAUSE  %08x\n", mips_read_c0_register (C0_CAUSE));
		splx (i);
		panic ("trap");
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
