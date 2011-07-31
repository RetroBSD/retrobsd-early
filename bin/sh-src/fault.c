/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 */
#include	"defs.h"


STRING		trapcom[MAXTRAP];
BOOL		trapflg[MAXTRAP];
BOOL		trapjmp[MAXTRAP];

/* ========	fault handling routines	   ======== */


void	fault(sig)
	REG INT		sig;
{
	REG INT		flag;
        sigset_t        set;

	IF sig==MEMF
	THEN	IF setbrk(brkincr) == -1
		THEN	error(nospace);
		FI
	ELIF sig==ALARM
	THEN	IF flags&waiting
		THEN	done();
		FI
	ELSE	flag = (trapcom[sig] ? TRAPSET : SIGSET);
		trapnote |= flag;
		trapflg[sig] |= flag;
	FI
        sigemptyset(&set);
        sigaddset(&set, sig);
        sigprocmask(SIG_UNBLOCK, &set, NULL);
	IF trapjmp[sig] ANDF sig==INTR
	THEN
		trapjmp[sig] = 0;
		longjmp(INTbuf, 1);
	FI
}

stdsigs()
{
	ignsig(QUIT);
	getsig(INTR);
	getsig(MEMF);
	getsig(ALARM);
}

ignsig(n)
{
	REG INT		s, i;

	i = n;
	s = (int) signal(i, SIG_IGN);
	IF (s & 1)==0
	THEN	trapflg[i] |= SIGMOD;
	FI
	return s;
}

getsig(n)
{
	REG INT		i;

	IF trapflg[i=n]&SIGMOD ORF ignsig(i)==0
	THEN	signal(i, fault);
	FI
}

oldsigs()
{
	REG INT		i;
	REG STRING	t;

	i=MAXTRAP;
	WHILE i--
	DO  t=trapcom[i];
	    IF t==0 ORF *t
	    THEN clrsig(i);
	    FI
	    trapflg[i]=0;
	OD
	trapnote=0;
}

clrsig(i)
	INT		i;
{
	free(trapcom[i]); trapcom[i]=0;
	IF trapflg[i]&SIGMOD
	THEN	signal(i,fault);
		trapflg[i] &= ~SIGMOD;
	FI
}

chktrap()
{
	/* check for traps */
	REG INT		i=MAXTRAP;
	REG STRING	t;

	trapnote &= ~TRAPSET;
	WHILE --i
	DO IF trapflg[i]&TRAPSET
	   THEN trapflg[i] &= ~TRAPSET;
		IF t=trapcom[i]
		THEN	INT	savxit=exitval;
			execexp(t,0);
			exitval=savxit; exitset();
		FI
	   FI
	OD
}
