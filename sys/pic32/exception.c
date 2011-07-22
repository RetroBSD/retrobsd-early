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

/*
 * Translate interrupt vector number to IRQ mask.
 */
static const unsigned mask_by_vector[] = {
	(1 << PIC32_IRQ_CT),		/* 0  - Core Timer Interrupt */
	(1 << PIC32_IRQ_CS0),           /* 1  - Core Software Interrupt 0 */
	(1 << PIC32_IRQ_CS1),           /* 2  - Core Software Interrupt 1 */
	(1 << PIC32_IRQ_INT0),          /* 3  - External Interrupt 0 */
	(1 << PIC32_IRQ_T1),            /* 4  - Timer1 */
	(1 << PIC32_IRQ_IC1),           /* 5  - Input Capture 1 */
	(1 << PIC32_IRQ_OC1),           /* 6  - Output Compare 1 */
	(1 << PIC32_IRQ_INT1),          /* 7  - External Interrupt 1 */
	(1 << PIC32_IRQ_T2),            /* 8  - Timer2 */
	(1 << PIC32_IRQ_IC2),           /* 9  - Input Capture 2 */
	(1 << PIC32_IRQ_OC2),           /* 10 - Output Compare 2 */
	(1 << PIC32_IRQ_INT2),          /* 11 - External Interrupt 2 */
	(1 << PIC32_IRQ_T3),            /* 12 - Timer3 */
	(1 << PIC32_IRQ_IC3),           /* 13 - Input Capture 3 */
	(1 << PIC32_IRQ_OC3),           /* 14 - Output Compare 3 */
	(1 << PIC32_IRQ_INT3),          /* 15 - External Interrupt 3 */
	(1 << PIC32_IRQ_T4),            /* 16 - Timer4 */
	(1 << PIC32_IRQ_IC4),           /* 17 - Input Capture 4 */
	(1 << PIC32_IRQ_OC4),           /* 18 - Output Compare 4 */
	(1 << PIC32_IRQ_INT4),          /* 19 - External Interrupt 4 */
	(1 << PIC32_IRQ_T5),            /* 20 - Timer5 */
	(1 << PIC32_IRQ_IC5),           /* 21 - Input Capture 5 */
	(1 << PIC32_IRQ_OC5),           /* 22 - Output Compare 5 */
	(1 << PIC32_IRQ_SPI1E) |        /* 23 - SPI1 */
	(1 << PIC32_IRQ_SPI1TX) |
	(1 << PIC32_IRQ_SPI1RX),
	(1 << PIC32_IRQ_U1E) |          /* 24 - UART1 */
	(1 << PIC32_IRQ_U1RX) |
	(1 << PIC32_IRQ_U1TX),
	(1 << PIC32_IRQ_I2C1B) |        /* 25 - I2C1 */
	(1 << PIC32_IRQ_I2C1S) |
	(1 << PIC32_IRQ_I2C1M),
	(1 << (PIC32_IRQ_CN-32)),       /* 26 - Input Change Interrupt */
	(1 << (PIC32_IRQ_AD1-32)),      /* 27 - ADC1 Convert Done */
	(1 << (PIC32_IRQ_PMP-32)),      /* 28 - Parallel Master Port */
	(1 << (PIC32_IRQ_CMP1-32)),     /* 29 - Comparator Interrupt */
	(1 << (PIC32_IRQ_CMP2-32)),     /* 30 - Comparator Interrupt */
	(1 << (PIC32_IRQ_SPI2E-32)) |   /* 31 - SPI2 */
	(1 << (PIC32_IRQ_SPI2TX-32)) |
	(1 << (PIC32_IRQ_SPI2RX-32)),
	(1 << (PIC32_IRQ_U2E-32)) |     /* 32 - UART2 */
	(1 << (PIC32_IRQ_U2RX-32)) |
	(1 << (PIC32_IRQ_U2TX-32)),
	(1 << (PIC32_IRQ_I2C2B-32)) |   /* 33 - I2C2 */
	(1 << (PIC32_IRQ_I2C2S-32)) |
	(1 << (PIC32_IRQ_I2C2M-32)),
	(1 << (PIC32_IRQ_FSCM-32)),     /* 34 - Fail-Safe Clock Monitor */
	(1 << (PIC32_IRQ_RTCC-32)),     /* 35 - Real-Time Clock and Calendar */
	(1 << (PIC32_IRQ_DMA0-32)),     /* 36 - DMA Channel 0 */
	(1 << (PIC32_IRQ_DMA1-32)),     /* 37 - DMA Channel 1 */
	(1 << (PIC32_IRQ_DMA2-32)),     /* 38 - DMA Channel 2 */
	(1 << (PIC32_IRQ_DMA3-32)),     /* 39 - DMA Channel 3 */
	0,				/* 40 */
	0,				/* 41 */
	0,				/* 42 */
	0,				/* 43 */
	(1 << (PIC32_IRQ_FCE-32)),      /* 44 - Flash Control Event */
	(1 << (PIC32_IRQ_USB-32)),      /* 45 - USB */
};

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

//#define TRACE_EXCEPTIONS

#ifdef TRACE_EXCEPTIONS
static void
print_args (narg, arg0, arg1, arg2, arg3, arg4, arg5)
{
        void print_arg (val) {
                if (val & 0xff000000)
                        printf ("%08x", val);
                else
                        printf ("%u", val);
        }

        print_arg (arg0);
        if (narg > 1) { printf (", "); print_arg (arg1); }
        if (narg > 2) { printf (", "); print_arg (arg2); }
        if (narg > 3) { printf (", "); print_arg (arg3); }
        if (narg > 4) { printf (", "); print_arg (arg4); }
        if (narg > 5) { printf (", "); print_arg (arg5); }
}
#endif

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
	unsigned intstat, irq, compare;

#ifdef UCB_METER
	cnt.v_trap++;
#endif
	unsigned status = frame [FRAME_STATUS];
	unsigned cause = mips_read_c0_register (C0_CAUSE);
//printf ("exception: cause %08x, status %08x\n", cause, status);

	cause &= CA_EXC_CODE;
	if (USERMODE (status))
		cause |= USER;

	syst = u.u_ru.ru_stime;
	p = u.u_procp;
	u.u_frame = frame;
	u.u_code = frame [FRAME_PC];    /* For signal handler */
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
	case CA_IBE + USER:		/* Bus error, instruction fetch */
	case CA_DBE + USER:		/* Bus error, load or store */
		i = SIGBUS;
		break;

	case CA_RI + USER:		/* Reserved instruction */
		i = SIGILL;
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
		/* Get the current irq number */
		intstat = INTSTAT;
		if ((intstat & PIC32_INTSTAT_SRIPL_MASK) == 0) {
                        printf ("=== unexpected interrupt: INTSTAT %08x\n", intstat);
			return;
                }
		irq = PIC32_INTSTAT_VEC (intstat);

                /* Handle the interrupt. */
                switch (irq) {
                case 0:                 /* Core Timer */
                        /* Increment COMPARE register. */
                        compare = mips_read_c0_register (C0_COMPARE);
                        do {
                                compare += (KHZ * 1000 / HZ + 1) / 2;
                                mips_write_c0_register (C0_COMPARE, compare);
                        } while ((int) (compare - mips_read_c0_register (C0_COUNT)) < 0);

                        hardclock ((caddr_t) frame [FRAME_PC], status);
                        //IECSET(0) = 1 << PIC32_IRQ_CT;
                        break;
                case 24:                /* UART1 */
                        //printf ("=== uart1\n");
                        IECCLR(0) = 1 << PIC32_IRQ_U1TX;
                        cnintr (0);
                        break;
                default:
                        /* Disable the irq, to avoid loops */
                        printf ("=== irq %u\n", irq);
                        if (irq < PIC32_VECT_CN)
                                IECCLR(0) = mask_by_vector [irq];
                        else
                                IECCLR(1) = mask_by_vector [irq];
                        break;
                }
		if ((cause & USER) && runrun) {
		        /* Process switch: in user mode only. */
                        /* Switch to kernel mode, enable interrupts. */
                        mips_write_c0_register (C0_STATUS, status & ~(ST_UM | ST_EXL));
                        goto out;
                }
                return;

	/*
	 * System call.
	 */
	case CA_Sys + USER:		/* Syscall */
                /* Switch to kernel mode, enable interrupts. */
                mips_write_c0_register (C0_STATUS, status & ~(ST_UM | ST_EXL));
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
#ifdef TRACE_EXCEPTIONS
                printf ("--- syscall: %s (", syscallnames [code >= nsysent ? 0 : code]);
                if (callp->sy_narg > 0)
                        print_args (callp->sy_narg, u.u_arg[0], u.u_arg[1],
                        u.u_arg[2], u.u_arg[3], u.u_arg[4], u.u_arg[5]);
                printf (") at %08x\n", opc);
#endif
		u.u_rval = 0;
		if (setjmp (&u.u_qsave) == 0) {
			(*callp->sy_call) ();
		}
		frame [FRAME_R8] = u.u_error;		/* $t0 - errno */
		switch (u.u_error) {
		case 0:
#ifdef TRACE_EXCEPTIONS
                        printf ("    syscall returned %u\n", u.u_rval);
#endif
			frame [FRAME_R2] = u.u_rval;	/* $v0 */
			break;
		case ERESTART:
			frame [FRAME_PC] = opc;		/* return to syscall */
			break;
		default:
#ifdef TRACE_EXCEPTIONS
                        printf ("    syscall failed, errno %u\n", u.u_error);
#endif
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
