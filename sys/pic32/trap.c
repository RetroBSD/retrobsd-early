/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "machine/psl.h"
#include "machine/reg.h"
#include "machine/trap.h"
#include "signalvar.h"
#include "systm.h"
#include "user.h"
#include "proc.h"
#include "vm.h"

#ifdef DIAGNOSTIC
extern int hasmap;
static int savhasmap;
#endif

/*
 * Offsets of the user's registers relative to
 * the saved r0.  See sys/reg.h.
 */
const int regloc[] = {
	R0, R1, R2, R3, R4, R5, R6, R7, RPS
};

/*
 * Called from mch.s when a processor trap occurs.
 * The arguments are the words saved on the system stack
 * by the hardware and software during the trap processing.
 * Their order is dictated by the hardware and the details
 * of C's calling sequence. They are peculiar in that
 * this call is not 'by value' and changed user registers
 * get copied back on return.
 * Dev is the kind of trap that occurred.
 */
/*ARGSUSED*/
void
exception (dev, sp, r1, ov, nps, r0, pc, ps)
	dev_t dev;
	caddr_t sp, pc;
	int r1, ov, nps, r0, ps;
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

	if (USERMODE(ps))
		dev |= USER;

	syst = u.u_ru.ru_stime;
	p = u.u_procp;
	u.u_ar0 = &r0;
	switch(minor(dev)) {

	/*
	 * Trap not expected.  Usually a kernel mode bus error.  The numbers
	 * printed are used to find the hardware PS/PC as follows.  (all
	 * numbers in octal 18 bits)
	 *	address_of_saved_ps =
	 *		(ka6*0100) + aps - 0140000;
	 *	address_of_saved_pc =
	 *		address_of_saved_ps - 2;
	 */
	default:
		once_thru = 1;
#ifdef DIAGNOSTIC
		/*
		 * Clear hasmap if we attempt to sync the fs.
		 * Else it might fail if we faulted with a mapped
		 * buffer.
		 */
		savhasmap = hasmap;
		hasmap = 0;
#endif
		i = splhigh();
		printf("aps %o\npc %o ps %o\nov %d\n", &ps, pc, ps, ov);
		printf("trap type %o\n", dev);
		splx(i);
		panic("trap");
		/*NOTREACHED*/

	case T_BUSFLT + USER:
		i = SIGBUS;
		break;

	case T_INSTRAP + USER:
		i = SIGILL;
		u.u_code = 0;	/* TODO */
		break;

	case T_BPTTRAP + USER:
		i = SIGTRAP;
		ps &= ~PSL_T;
		break;

	case T_IOTTRAP + USER:
		i = SIGIOT;
		break;

	case T_EMTTRAP + USER:
		i = SIGEMT;
		break;

	/*
	 * Since the floating exception is an imprecise trap, a user
	 * generated trap may actually come from kernel mode.  In this
	 * case, a signal is sent to the current process to be picked
	 * up later.
	 */
	case T_ARITHTRAP:
	case T_ARITHTRAP + USER:
		i = SIGFPE;
		/* save error code and address
		 * TODO */
		u.u_code = 0;
		break;

	/*
	 * If the user SP is below the stack segment, grow the stack
	 * automatically.
	 */
	case T_SEGFLT + USER:
		if (! (u.u_sigstk.ss_flags & SA_ONSTACK) && grow ((u_int)sp))
			goto out;
		i = SIGSEGV;
		break;

	/*
	 * The code here is a half-hearted attempt to do something with
	 * all of the PDP11 parity registers.  In fact, there is little
	 * that can be done.
	 */
	case T_PARITYFLT:
	case T_PARITYFLT + USER:
		panic("parity");
		/*NOTREACHED*/

	/*
	 * Allow process switch
	 */
	case T_SWITCHTRAP + USER:
		goto out;

	/*
	 * Whenever possible, locations 0-2 specify this style trap, since
	 * DEC hardware often generates spurious traps through location 0.
	 * This is a symptom of hardware problems and may represent a real
	 * interrupt that got sent to the wrong place.  Watch out for hangs
	 * on disk completion if this message appears.  Uninitialized
	 * interrupt vectors may be set to trap here also.
	 */
	case T_ZEROTRAP:
	case T_ZEROTRAP + USER:
		printf("Trap to 0: ");
		/*FALL THROUGH*/
	case T_RANDOMTRAP:		/* generated by autoconfig */
	case T_RANDOMTRAP + USER:
		printf("Random intr ignored\n");
		return;
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
		addupc (pc, &u.u_prof, (int) (u.u_ru.ru_stime - syst));
}

/*
 * Called from mch.s when a system call occurs.  dev, ov and nps are
 * unused, but are necessitated by the values of the R[0-7] ... constants
 * in sys/reg.h (which, in turn, are defined by trap's stack frame).
 */
/*ARGSUSED*/
void
syscall (dev, sp, r1, ov, nps, r0, pc, ps)
	dev_t dev;
	caddr_t sp, pc;
	int r1, ov, nps, r0, ps;
{
	extern int nsysent;
	register int code;
	register const struct sysent *callp;
	time_t syst;
	register caddr_t opc;	/* original pc for restarting syscalls */
	int	i;

#ifdef UCB_METER
	cnt.v_syscall++;
#endif
	syst = u.u_ru.ru_stime;
	u.u_ar0 = &r0;
	u.u_error = 0;
	opc = pc - 4;			/* opc now points at syscall */
	code = *(u_int*)opc & 0377;	/* bottom 8 bits are index */
	if (code >= nsysent)
		callp = &sysent[0];	/* indir (illegal) */
	else
		callp = &sysent[code];
	if (callp->sy_narg)
		copyin(sp+2, (caddr_t)u.u_arg, callp->sy_narg*NBPW);
	u.u_r.r_val1 = 0;
	u.u_r.r_val2 = r1;
	if (setjmp(&u.u_qsave) == 0) {
		(*callp->sy_call)();
	}
	switch (u.u_error) {
	case 0:
		ps &= ~PSL_C;
		r0 = u.u_r.r_val1;
		r1 = u.u_r.r_val2;
		break;
	case ERESTART:
		pc = opc;
		break;
	case EJUSTRETURN:
		break;
	default:
		ps |= PSL_C;
		r0 = u.u_error;
		break;
	}
	/* Process all received signals. */
	for (;;) {
		i = CURSIG (u.u_procp);
		if (i <= 0)
			break;
		postsig (i);
	}
	curpri = setpri(u.u_procp);
	if (runrun) {
		setrq(u.u_procp);
		u.u_ru.ru_nivcsw++;
		swtch();
	}
	if (u.u_prof.pr_scale)
		addupc(pc, &u.u_prof, (int)(u.u_ru.ru_stime - syst));
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
	psignal(u.u_procp, SIGSYS);
}
