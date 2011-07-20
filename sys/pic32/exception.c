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

	printf ("\n*** 0x%08x: exception ", frame [FRAME_PC]);

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
		frame [FRAME_R8], frame [FRAME_R16],
		frame [FRAME_R24], frame [FRAME_LO]);
	printf ("at = %8x   t1 = %8x   s1 = %8x   t9 = %8x   hi = %8x\n",
		frame [FRAME_R1], frame [FRAME_R9], frame [FRAME_R17],
		frame [FRAME_R25], frame [FRAME_HI]);
	printf ("v0 = %8x   t2 = %8x   s2 = %8x               status = %8x\n",
		frame [FRAME_R2], frame [FRAME_R10],
		frame [FRAME_R18], frame [FRAME_STATUS]);
	printf ("v1 = %8x   t3 = %8x   s3 = %8x                cause = %8x\n",
		frame [FRAME_R3], frame [FRAME_R11],
		frame [FRAME_R19], cause);
	printf ("a0 = %8x   t4 = %8x   s4 = %8x   gp = %8x  epc = %8x\n",
		frame [FRAME_R4], frame [FRAME_R12],
		frame [FRAME_R20], frame [FRAME_GP], frame [FRAME_PC]);
	printf ("a1 = %8x   t5 = %8x   s5 = %8x   sp = %8x\n",
		frame [FRAME_R5], frame [FRAME_R13],
		frame [FRAME_R21], frame [FRAME_SP]);
	printf ("a2 = %8x   t6 = %8x   s6 = %8x   fp = %8x\n",
		frame [FRAME_R6], frame [FRAME_R14],
		frame [FRAME_R22], frame [FRAME_FP]);
	printf ("a3 = %8x   t7 = %8x   s7 = %8x   ra = %8x\n",
		frame [FRAME_R7], frame [FRAME_R15],
		frame [FRAME_R23], frame [FRAME_RA]);
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
	register int i;
	register struct proc *p;
	time_t syst;

#ifdef UCB_METER
	cnt.v_trap++;
#endif
	unsigned status = frame [FRAME_STATUS];
	unsigned cause = mips_read_c0_register (C0_CAUSE);
//printf ("exception: cause %08x, status %08x\n", cause, status);

	cause &= CA_EXC_CODE;
	if (USERMODE (status))
		cause |= USER;

	/* Switch to kernel mode, enable interrupts. */
	mips_write_c0_register (C0_STATUS, status & ~(ST_UM | ST_EXL));
//void printmem (unsigned addr, int nbytes);
//printmem (0x7f010380, 0x40);
//dumpregs (frame);

	syst = u.u_ru.ru_stime;
	p = u.u_procp;
	u.u_frame = frame;
	switch (cause) {

	/*
	 * Exception not expected.  Usually a kernel mode bus error.
	 */
	default:
		i = splhigh();
		dumpregs (frame);
		splx (i);
		panic ("unexpected exception");
		/*NOTREACHED*/
#if 0
	case CA_IBE + USER:			/* Bus error, instruction fetch */
	case CA_DBE + USER:			/* Bus error, load or store */
		i = SIGBUS;
		break;

	case CA_RI + USER:		/* Reserved instruction */
		i = SIGILL;
		u.u_code = frame [FRAME_PC];
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
		u.u_code = frame [FRAME_PC];
		break;

	case CA_AdEL + USER:		/* Address error, load or instruction fetch */
	case CA_AdES + USER:		/* Address error, store */
		i = SIGSEGV;
		break;
#endif
	/*
	 * Hardware interrupt.
	 */
	case CA_Int:			/* Interrupt */
	case CA_Int + USER:
		/* TODO */
printf ("*** interrupt\n");
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
		int opc = frame [FRAME_PC];		/* opc now points at syscall */
		frame [FRAME_PC] = opc + 3*NBPW;        /* no errors - skip 2 next instructions */

		const struct sysent *callp;
		int code = (*(u_int*) opc >> 6) & 0377;	/* bottom 8 bits are index */
		if (code >= nsysent)
			callp = &sysent[0];		/* indir (illegal) */
		else
			callp = &sysent[code];
printf ("*** syscall: %s at %08x\n", syscallnames [code >= nsysent ? 0 : code], opc);
		if (callp->sy_narg) {
			u.u_arg[0] = frame [FRAME_R4];		/* $a0 */
			u.u_arg[1] = frame [FRAME_R5];		/* $a1 */
			u.u_arg[2] = frame [FRAME_R6];		/* $a2 */
			u.u_arg[3] = frame [FRAME_R7];		/* $a3 */
			if (callp->sy_narg > 4) {
				unsigned addr = (frame [FRAME_SP] + 16) & ~3;
				if (! baduaddr ((caddr_t) addr))
					u.u_arg[4] = *(unsigned*) addr;
			}
			if (callp->sy_narg > 5) {
				unsigned addr = (frame [FRAME_SP] + 20) & ~3;
				if (! baduaddr ((caddr_t) addr))
					u.u_arg[5] = *(unsigned*) addr;
			}
		}
		u.u_rval = 0;
		if (setjmp (&u.u_qsave) == 0) {
			(*callp->sy_call) ();
		}
		frame [FRAME_R8] = u.u_error;		/* $t0 - errno */
		switch (u.u_error) {
		case 0:
printf ("*** syscall returned %u\n", u.u_rval);
			frame [FRAME_R2] = u.u_rval;	/* $v0 */
			break;
		case ERESTART:
			frame [FRAME_PC] = opc;		/* return to syscall */
			break;
		default:
printf ("*** syscall failed, errno %u\n", u.u_error);
			frame [FRAME_PC] = opc + NBPW;	/* return to next instruction */
			frame [FRAME_R2] = -1;		/* $v0 */
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
		addupc ((caddr_t) frame [FRAME_PC],
			&u.u_prof, (int) (u.u_ru.ru_stime - syst));
//dumpregs (frame);
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
