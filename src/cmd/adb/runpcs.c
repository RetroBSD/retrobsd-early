#include "defs.h"
#include <sys/file.h>

char    *lp;
BKPTR   bkpthead;
char	lastc;
u_int	*uar0;
int	fcor;
int	fsym;
const char *errflg;
int	signo;
long	dot;
char	*symfil;
int	wtflag;
int	pid;
long	expv;
int	adrflg;
long	loopcnt;
int	userpc=1;

extern REGLIST reglist[];
extern u_int corhdr[];
extern long var[];

/* service routines for sub process control */

getsig(sig)
{
	return(expr(0) ? shorten(expv) : sig);
}

runpcs(runmode, execsig)
{
	int	rc;
	register BKPTR       bkpt;

	IF adrflg
	THEN userpc=shorten(dot);
	FI
	print("%s: running\n", symfil);

	WHILE (loopcnt--)>0
	DO
#ifdef DEBUG
		print("\ncontinue %d %d\n",userpc,execsig);
#endif
		stty(0,&usrtty);
		ptrace(runmode,pid,userpc,execsig);
		bpwait(); chkerr(); readregs();

		/*look for bkpt*/
		IF signo==0 ANDF (bkpt=scanbkpt(uar0[FRAME_PC]-2))
		THEN /*stopped at bkpt*/
		     userpc = uar0[FRAME_PC] = bkpt->loc;
		     IF bkpt->flag==BKPTEXEC
			ORF ((bkpt->flag=BKPTEXEC, command(bkpt->comm,':')) ANDF --bkpt->count)
		     THEN execbkpt(bkpt); execsig=0; loopcnt++;
			  userpc=1;
		     ELSE bkpt->count=bkpt->initcnt;
			  rc=1;
		     FI
		ELSE rc=0; execsig=signo; userpc=1;
		FI
	OD
	return(rc);
}

endpcs()
{
	register BKPTR       bkptr;

	IF pid
	THEN ptrace(PT_KILL,pid,0,0); pid=0; userpc=1;
	     FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	     DO IF bkptr->flag
		THEN bkptr->flag=BKPTSET;
		FI
	     OD
	FI
}

setup()
{

	close(fsym); fsym = -1;
	IF (pid = fork()) == 0
	THEN ptrace(PT_TRACE_ME,0,0,0);
	     signal(SIGINT, sigint);
             signal(SIGQUIT, sigqit);
	     doexec(); exit(0);
	ELIF pid == -1
	THEN error(NOFORK);
	ELSE bpwait(); readregs(); lp[0]=EOR; lp[1]=0;
	     fsym=open(symfil,wtflag);
	     IF errflg
	     THEN print("%s: cannot execute\n",symfil);
		  endpcs(); error((char *)0);
	     FI
	FI
}

execbkpt(bkptr)
BKPTR   bkptr;
{
	int	bkptloc;
#ifdef DEBUG
	print("exbkpt: %d\n",bkptr->count);
#endif
	bkptloc = bkptr->loc;
	ptrace(PT_WRITE_I,pid,bkptloc,bkptr->ins);
	stty(0,&usrtty);
	ptrace(PT_STEP,pid,bkptloc,0);
	bpwait(); chkerr();
	ptrace(PT_WRITE_I,pid,bkptloc,BPT);
	bkptr->flag=BKPTSET;
}

doexec()
{
	char	*argl[MAXARG];
	char	args[LINSIZ];
	char	*p, **ap, *filnam;

	ap=argl; p=args;
	*ap++=symfil;
	REP     IF rdc()==EOR THEN break; FI
		/*
		 * If we find an argument beginning with a `<' or a `>', open
		 * the following file name for input and output, respectively
		 * and back the argument collocation pointer, p, back up.
		 */
		*ap = p;
		WHILE lastc!=EOR ANDF lastc!=SP ANDF lastc!=TB DO *p++=lastc; readchar(); OD
		*p++=0; filnam = *ap+1;
		IF **ap=='<'
		THEN    close(0);
			IF open(filnam,0)<0
			THEN    print("%s: cannot open\n",filnam); exit(0);
			FI
			p = *ap;
		ELIF **ap=='>'
		THEN    close(1);
			IF open(filnam, O_CREAT|O_WRONLY, 0666)<0
			THEN    print("%s: cannot create\n",filnam); exit(0);
			FI
			p = *ap;
		ELSE    ap++;
		FI
	PER lastc!=EOR DONE
	*ap++=0;
	execv(symfil, argl);
}

BKPTR   scanbkpt(adr)
{
	register BKPTR       bkptr;

	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag ANDF bkptr->loc==adr
	   THEN break;
	   FI
	OD
	return(bkptr);
}

delbp()
{
	register BKPTR       bkptr;

	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag
	   THEN del1bp(bkptr);
	   FI
	OD
}

del1bp(bkptr)
BKPTR bkptr;
{
	ptrace(PT_WRITE_I,pid,bkptr->loc,bkptr->ins);
}

setbp()
{
	register BKPTR       bkptr;

	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag
	   THEN set1bp(bkptr);
	   FI
	OD
}
set1bp(bkptr)
BKPTR bkptr;
{
	register int         a;

	a = bkptr->loc;
	bkptr->ins = ptrace(PT_READ_I, pid, a, 0);
	ptrace(PT_WRITE_I, pid, a, BPT);
	IF errno
	THEN print("cannot set breakpoint: ");
	     psymoff(leng(bkptr->loc),ISYM,"\n");
	FI
}

bpwait()
{
	register int w;
	int stat;

	signal(SIGINT, SIG_IGN);
	WHILE (w = wait(&stat))!=pid ANDF w != -1 DONE
	signal(SIGINT, sigint);
	gtty(0,&usrtty);
	stty(0,&adbtty);
	IF w == -1
	THEN pid=0;
	     errflg = BADWAIT;
	ELIF (stat & 0177) != 0177
	THEN IF signo = stat&0177
	     THEN sigprint();
	     FI
	     IF stat&0200
	     THEN print(" - core dumped");
		  close(fcor);
		  setcor();
	     FI
	     pid = 0;
	     errflg = ENDPCS;
	ELSE signo = stat>>8;
	     IF signo!=SIGTRAP
	     THEN sigprint();
	     ELSE signo=0;
	     FI
	     flushbuf();
	FI
}

readregs()
{
	/*get REG values from pcs*/
	register u_int i, *afp;

	FOR i=0; i<NREG; i++
	DO uar0[reglist[i].roffs] =
		    ptrace(PT_READ_U, pid,
 		        (int *)((int)&uar0[reglist[i].roffs] - (int)&corhdr),
 			0);
	OD
}
