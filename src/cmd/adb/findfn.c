#include "defs.h"

int	callpc;
char	localok;
long	var[36];
const char *errflg;

extern struct SYMbol *symbol;

findroutine(cframe)
	long	cframe;
{
	register int	narg, inst;
	int	lastpc, back2;
	char	v;

	v=FALSE; localok=FALSE; lastpc=callpc;
	callpc=get(cframe+2, DSP); back2=get(leng(callpc-2), ISP);
	IF (inst=get(leng(callpc-4), ISP)) == 04737	/* jsr pc,*$... */
	THEN	narg = 1;
	ELIF (inst&~077)==04700			/* jsr pc,... */
	THEN	narg=0; v=(inst!=04767);
	ELIF (back2&~077)==04700
	THEN	narg=0; v=TRUE;
	ELSE	errflg=NOCFN;
		return(0);
	FI
	if (findsym((v ? lastpc : ((inst==04767?callpc:0) + back2)),ISYM) == -1
	    && !v)
		symbol = NULL;
	else
		localok=TRUE;
	inst = get(leng(callpc), ISP);
	IF inst == 062706		/* add $n,sp */
	THEN
		narg += get(leng(callpc+2), ISP)/2;
		return(narg);
	FI
	if (inst == 05726 || inst == 010026)	/* tst (sp)+ or mov r0,(sp)+ */
		return(narg+1);
	IF inst == 022626		/* cmp (sp)+,(sp)+ */
	THEN
		return(narg+2);
	FI
	return(narg);
}
