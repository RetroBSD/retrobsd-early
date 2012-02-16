#include "defs.h"
#include <sys/file.h>

MAP     txtmap;
MAP     datmap;
int	lastframe;
int     kernel;
int	callpc;
int	infile;
int	outfile;
char	*lp;
int	maxoff;
int	maxpos;
int	octal;
long	localval;
BKPTR   bkpthead;
char    lastc;
u_int	*uar0;
int	fcor;
char	*errflg;
int	signo;
long	dot;
char	*symfil;
char	*corfil;
int	pid;
long	adrval;
int	adrflg;
long	cntval;
int	cntflg;

extern struct SYMbol	*symbol;
extern u_int corhdr[];
extern long var[];

REGLIST reglist [FRAME_WORDS] = {
        "r1",       FRAME_R1,           /* 0  */
        "r2",       FRAME_R2,		/* 1  */
        "r3",       FRAME_R3,		/* 2  */
        "r4",       FRAME_R4,		/* 3  */
        "r5",       FRAME_R5,		/* 4  */
        "r6",       FRAME_R6,		/* 5  */
        "r7",       FRAME_R7,		/* 6  */
        "r8",       FRAME_R8,		/* 7  */
        "r9",       FRAME_R9,		/* 8  */
        "r10",      FRAME_R10,		/* 9  */
        "r11",      FRAME_R11,		/* 10 */
        "r12",      FRAME_R12,		/* 11 */
        "r13",      FRAME_R13,		/* 12 */
        "r14",      FRAME_R14,		/* 13 */
        "r15",      FRAME_R15,		/* 14 */
        "r16",      FRAME_R16,		/* 15 */
        "r17",      FRAME_R17,		/* 16 */
        "r18",      FRAME_R18,		/* 17 */
        "r19",      FRAME_R19,		/* 18 */
        "r20",      FRAME_R20,		/* 19 */
        "r21",      FRAME_R21,		/* 20 */
        "r22",      FRAME_R22,		/* 21 */
        "r23",      FRAME_R23,		/* 22 */
        "r24",      FRAME_R24,		/* 23 */
        "r25",      FRAME_R25,		/* 24 */
        "gp",       FRAME_GP,		/* 25 */
        "sp",       FRAME_SP,		/* 26 */
        "fp",       FRAME_FP,		/* 27 */
        "ra",       FRAME_RA,		/* 28 */
        "lo",       FRAME_LO,		/* 29 */
        "hi",       FRAME_HI,           /* 30 */
        "status",   FRAME_STATUS,	/* 31 */
        "pc",       FRAME_PC,    	/* 32 */
};

REGLIST kregs[] = {
        // TODO
        "r1",       FRAME_R1,           /* 0  */
        "r2",       FRAME_R2,		/* 1  */
        "r3",       FRAME_R3,		/* 2  */
        "r4",       FRAME_R4,		/* 3  */
        "r5",       FRAME_R5,		/* 4  */
        "r6",       FRAME_R6,		/* 5  */
        "sp",       FRAME_SP,		/* 26 */
};

/* general printing routines ($) */

printtrace(modif)
{
	int	narg, i, stat, name, limit;
	u_int	dynam;
	register BKPTR       bkptr;
	char	hi, lo;
	int	word, stack;
	char	*comptr;
	long	argp, frame, link;
	register struct	SYMbol	*symp;

	IF cntflg==0 THEN cntval = -1; FI

	switch (modif) {

	    case '<':
			IF cntval == 0
			THEN	WHILE readchar() != EOR
				DO OD
				lp--;
				break;
			FI
			IF rdc() == '<'
			THEN	stack = 1;
			ELSE	stack = 0; lp--;
			FI
			/* fall thru ... */
	    case '>':
		{
			char		file[64];
			char		Ifile[128];
			extern char	*Ipath;
			int             index;

			index=0;
			IF rdc()!=EOR
			THEN    REP file[index++]=lastc;
				    IF index>=63 THEN error(LONGFIL); FI
				PER readchar()!=EOR DONE
				file[index]=0;
				IF modif=='<'
				THEN	IF Ipath THEN
						strcpy(Ifile, Ipath);
						strcat(Ifile, "/");
						strcat(Ifile, file);
					FI
					IF strcmp(file, "-") != 0
					THEN	iclose(stack, 0);
						infile=open(file,0);
						IF infile<0
						THEN	infile = open(Ifile, 0);
						FI
					ELSE	lseek(infile, 0L, 0);
					FI
					IF infile<0
					THEN    infile=0; error(NOTOPEN);
					ELSE	IF cntflg
						THEN var[9] = cntval;
						ELSE var[9] = 1;
						FI
					FI
				ELSE    oclose();
					outfile = open(file, O_CREAT|O_WRONLY, 0644);
					lseek(outfile,0L,2);
				FI

			ELSE	IF modif == '<'
				THEN	iclose(-1, 0);
				ELSE	oclose();
				FI
			FI
			lp--;
		}
		break;

	    case 'o':
		octal = TRUE; break;

	    case 'd':
		octal = FALSE; break;

	    case 'q': case 'Q': case '%':
		done();

	    case 'w': case 'W':
		maxpos=(adrflg?adrval:MAXPOS);
		break;

	    case 's': case 'S':
		maxoff=(adrflg?adrval:MAXOFF);
		break;

	    case 'v':
		print("variables\n");
		FOR i=0;i<=35;i++
		DO IF var[i]
		   THEN printc((i<=9 ? '0' : 'a'-10) + i);
			print(" = %Q\n",var[i]);
		   FI
		OD
		break;

	    case 'm': case 'M':
		printmap("? map",&txtmap);
		printmap("/ map",&datmap);
		break;

	    case 0: case '?':
		IF pid
		THEN print("pcs id = %d\n",pid);
		ELSE print("no process\n");
		FI
		sigprint(); flushbuf();

	    case 'r': case 'R':
		printregs();
		return;

	    case 'f': case 'F':
                // No FPU registers
		return;

	    case 'c': case 'C':
		frame = (adrflg ? adrval :
                        (kernel ? corhdr[FRAME_R5] : uar0[FRAME_R5])) & EVEN;
		lastframe = 0;
		callpc = (adrflg ? get(frame + 2, DSP) :
                         (kernel ? (-2) : uar0[FRAME_PC]));
		WHILE cntval--
		DO      chkerr();
 			print("%07O: ", frame); /* Add frame address info */
			narg = findroutine(frame);
			print("%s(", cache_sym(symbol));
			argp = frame+4;
			IF --narg >= 0
			THEN    print("%o", get(argp, DSP));
			FI
			WHILE --narg >= 0
			DO      argp += 2;
				print(",%o", get(argp, DSP));
			OD
 			/* Add return-PC info.  Force printout of
 			 * symbol+offset (never just a number! ) by using
 			 * max possible offset.  Overlay has already been set
 			 * properly by findfn.
 			 */
 			print(") from ");
 			{
				int	savmaxoff = maxoff;

 				maxoff = ((unsigned)-1)>>1;
 				psymoff((long)callpc,ISYM,"");
 				maxoff = savmaxoff;
 			}
 			printc('\n');

			IF modif=='C'
			THEN WHILE localsym(frame)
			     DO word=get(localval,DSP);
				print("%8t%s:%10t", cache_sym(symbol));
				IF errflg THEN print("?\n"); errflg=0; ELSE print("%o\n",word); FI
			     OD
			FI

			lastframe=frame;
			frame=get(frame, DSP)&EVEN;
			IF kernel ? ((u_int)frame > ((u_int)0140000 + USIZE)) :
			    (frame == 0)
				THEN break; FI
		OD
		break;

	    /*print externals*/
	    case 'e': case 'E':
		symset();
		WHILE (symp=symget())
		DO chkerr();
		   IF (symp->type == N_EXT|N_DATA) || (symp->type== N_EXT|N_BSS)
		   THEN print("%s:%12t%o\n", no_cache_sym(symp),
				get(leng(symp->value),DSP));
		   FI
		OD
		break;

	    case 'a': case 'A':
		frame = adrflg ? adrval : uar0[FRAME_R4];

		WHILE cntval--
		DO chkerr();
		   stat=get(frame,DSP); dynam=get(frame+2,DSP); link=get(frame+4,DSP);
		   IF modif=='A'
		   THEN print("%8O:%8t%-8o,%-8o,%-8o",frame,stat,dynam,link);
		   FI
		   IF stat==1 THEN break; FI
		   IF errflg THEN error(A68BAD); FI

		   IF get(link-4,ISP)!=04767
		   THEN IF get(link-2,ISP)!=04775
			THEN error(A68LNK);
			ELSE /*compute entry point of routine*/
			     print(" ? ");
			FI
		   ELSE print("%8t");
			valpr(name=shorten(link)+get(link-2,ISP),ISYM);
			name=get(leng(name-2),ISP);
			print("%8t\""); limit=8;
			REP word=get(leng(name),DSP); name += 2;
			    lo=word&LOBYTE; hi=(word>>8)&LOBYTE;
			    printc(lo); printc(hi);
			PER lo ANDF hi ANDF limit-- DONE
			printc('"');
		   FI
		   limit=4; i=6; print("%24targs:%8t");
		   WHILE limit--
		   DO print("%8t%o",get(frame+i,DSP)); i += 2; OD
		   printc(EOR);

		   frame=dynam;
		OD
		errflg=0;
		flushbuf();
		break;

	    /*set default c frame*/
	    /*print breakpoints*/
	    case 'b': case 'B':
		print("breakpoints\ncount%8tbkpt%24tcommand\n");
		FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
		DO IF bkptr->flag
		   THEN print("%-8.8d",bkptr->count);
			psymoff(leng(bkptr->loc),ISYM,"%24t");
			comptr=bkptr->comm;
			WHILE *comptr DO printc(*comptr++); OD
		   FI
		OD
		break;

	    default: error(BADMOD);
	}

}

printmap(s,amap)
	char	*s;
	MAP	*amap;
{
	int file;
	file=amap->ufd;
	print("%s%12t`%s'\n",s,(file<0 ? "-" : (file==fcor ? corfil : symfil)));
	print("b1 = %-16Q",amap->b1);
	print("e1 = %-16Q",amap->e1);
	print("f1 = %-16Q",amap->f1);
	IF amap->bo
	THEN    print("\n\t{ bo = %-16Q",amap->bo);
		print("eo = %-16Q",amap->eo);
		print("fo = %-16Q}",amap->fo);
	FI
	print("\nb2 = %-16Q",amap->b2);
	print("e2 = %-16Q",amap->e2);
	print("f2 = %-16Q",amap->f2);
	printc(EOR);
}

printregs()
{
	register REGPTR      p;
	int	v;

	IF kernel
	THEN    FOR p=kregs; p<&kregs[7]; p++
		DO      print("%s%8t%o%8t", p->rname, v=corhdr[p->roffs]);
			valpr(v,DSYM);
			printc(EOR);
		OD
	ELSE    FOR p=reglist; p < &reglist[NREG]; p++
		DO      print("%s%8t%o%8t", p->rname, v=uar0[p->roffs]);
			valpr(v, (p->roffs == FRAME_PC) ? ISYM : DSYM);
			printc(EOR);
		OD
		printpc();
	FI
}

getreg(regnam)
{
	register REGPTR      p;
	register char	*regptr;
	char	regnxt;

	IF kernel THEN return(NOREG); FI        /* not supported */
	regnxt=readchar();
	FOR p=reglist; p<&reglist[NREG]; p++
	DO      regptr=p->rname;
		IF (regnam == *regptr++) ANDF (regnxt == *regptr)
		THEN    return(p->roffs);
		FI
	OD
	lp--;
	return(NOREG);
}

printpc()
{
	dot = uar0[FRAME_PC];
	psymoff(dot, ISYM, ":%16t");
        printins(ISP, chkget(dot, ISP));
	printc(EOR);
}

sigprint()
{
	extern char	*sys_siglist[];		/* signal list */

	if (signo >= 0 && signo < NSIG)
		print("%s",sys_siglist[signo]);
	else
		print("unknown signal %d",signo);
}
