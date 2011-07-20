/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "signalvar.h"
#include "user.h"
#include "proc.h"
#include "buf.h"
#include "map.h"
#include "syslog.h"

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow trampoline code stored at u.u_sigtramp (as
 * specified by the user process) to call the user's real signal catch
 * routine, followed by sys sigreturn to the sigreturn routine below (see
 * /usr/src/lib/libc/pdp/sys/sigvec.s).  After sigreturn resets the signal
 * mask, the stack, and the frame pointer, it returns to the user specified
 * pc, ps.
 */
void
sendsig (p, sig, mask)
	sig_t p;
	int sig;
	long mask;
{
	struct sigframe {
		int	sf_signum;
		int	sf_code;
		struct	sigcontext *sf_scp;
		struct	sigcontext sf_sc;
	};
	struct sigframe sf;
	register struct sigframe *sfp = &sf;
	register struct sigcontext *scp = &sf.sf_sc;
	register int *regs;
	int oonstack;
	caddr_t n;

//#ifdef DIAGNOSTIC
	printf("sendsig %d to %d mask=%O action=%o\n", sig, u.u_procp->p_pid,
		mask, p);
//#endif
	regs = u.u_frame;
	oonstack = u.u_sigstk.ss_flags & SA_ONSTACK;

	/*
	 * Allocate and validate space for the signal frame.
	 */
	if ((u.u_psflags & SAS_ALTSTACK) &&
	     ! (u.u_sigstk.ss_flags & SA_ONSTACK) &&
	    (u.u_sigonstack & sigmask(sig))) {
		n = u.u_sigstk.ss_base + u.u_sigstk.ss_size - sizeof (sf);
		u.u_sigstk.ss_flags |= SA_ONSTACK;
	} else
		n = (caddr_t) regs [FRAME_SP] - sizeof (sf);

	if (! (u.u_sigstk.ss_flags & SA_ONSTACK) &&
	    n < (caddr_t) -u.u_ssize && ! grow ((unsigned ) n)) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction violation to halt it in its tracks.
		 */
		fatalsig(SIGILL);
		return;
	}

	/*
	 * Build the argument list for the signal handler.
	 */
	sfp->sf_signum = sig;
	if (sig == SIGILL || sig == SIGFPE) {
		sfp->sf_code = u.u_code;
		u.u_code = 0;
	} else
		sfp->sf_code = 0;

	sfp->sf_scp = (struct sigcontext *)
			(n + (u_int)&((struct sigframe *)0)->sf_sc);
	/*
	 * Build the signal context to be used by sigreturn.
	 */
	scp->sc_onstack = oonstack;
	scp->sc_mask = mask;
	scp->sc_sp = regs [FRAME_SP];
	scp->sc_fp = regs [FRAME_FP];
	scp->sc_r1 = regs [FRAME_R3];
	scp->sc_r0 = regs [FRAME_R2];
	scp->sc_pc = regs [FRAME_PC];
	scp->sc_ps = regs [FRAME_STATUS];

	copyout ((caddr_t) sfp, n, sizeof (*sfp));

	regs [FRAME_R2] = (int) p;
	regs [FRAME_SP] = (int) n;
	regs [FRAME_PC] = (int) u.u_sigtramp;
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and ps as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * ps to gain improper priviledges or to cause
 * a machine fault.
 */
void
sigreturn()
{
	struct a {
		struct sigcontext *scp;
	};
	struct sigcontext sc;
	register struct sigcontext *scp = &sc;
	register int *regs = u.u_frame;

	u.u_error = copyin ((caddr_t) ((struct a*)u.u_arg)->scp, (caddr_t) scp, sizeof (*scp));
	if (u.u_error)
		return;
	if (! USERMODE (scp->sc_ps)) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = EJUSTRETURN;
	if	(scp->sc_onstack & SA_ONSTACK)
		u.u_sigstk.ss_flags |= SA_ONSTACK;
	else
		u.u_sigstk.ss_flags &= ~SA_ONSTACK;
	u.u_procp->p_sigmask = scp->sc_mask & ~sigcantmask;

	regs [FRAME_SP] = scp->sc_sp;
	regs [FRAME_FP] = scp->sc_fp;
	regs [FRAME_R3] = scp->sc_r1;		/* $v1 */
	regs [FRAME_R2] = scp->sc_r0;		/* $v0 */
	regs [FRAME_PC] = scp->sc_pc;
	regs [FRAME_STATUS] = scp->sc_ps;
}
