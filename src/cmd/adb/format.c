#include "defs.h"
#include <ctype.h>

int	mkfault;
char	*lp;
char	*printptr;
int	maxoff;
char	*errflg;
char	lastc;
long	dot;
int	dotinc;

extern struct SYMbol *symbol;
extern char printbuf[];
extern long var[];

scanform(icount,ifp,itype,ptype)
	long	icount;
	char	*ifp;
{
	char	*fp;
	char	modifier;
	int	fcount, init=1;
	long	savdot;

	WHILE icount
	DO  fp=ifp;
	    IF init==0 ANDF findsym(shorten(dot),ptype)==0 ANDF maxoff
	    THEN print("\n%s:%16t", cache_sym(symbol));
	    FI
	    savdot=dot; init=0;

	    /*now loop over format*/
	    WHILE *fp ANDF errflg==0
	    DO  IF isdigit(modifier = *fp)
		THEN fcount=0;
		     WHILE isdigit(modifier = *fp++)
		     DO fcount *= 10;
			fcount += modifier-'0';
		     OD
		     fp--;
		ELSE fcount=1;
		FI

		IF *fp==0 THEN break; FI
		fp=exform(fcount,fp,itype,ptype);
	    OD
	    dotinc=dot-savdot;
	    dot=savdot;

	    IF errflg
	    THEN IF icount<0
		 THEN errflg=0; break;
		 ELSE error(errflg);
		 FI
	    FI
	    IF --icount
	    THEN dot=inkdot(dotinc);
	    FI
	    IF mkfault THEN error((char *)0); FI
	OD
}

char *
exform(fcount,ifp,itype,ptype)
	int	fcount;
	char	*ifp;
	int	itype, ptype;
{
	/* execute single format item `fcount' times
	 * sets `dotinc' and moves `dot'
	 * returns address of next format item
	 */
	u_int	w;
	long	savdot, wx;
	char	*fp;
	char	c, modifier, longpr;
	struct	{
		long	sa;
		int	sb,sc;
		} fw;

	WHILE fcount>0
	DO	fp = ifp; c = *fp;
		longpr=(c>='A')&(c<='Z')|(c=='f');
		IF itype==NSP ORF *fp=='a'
		THEN wx=dot; w=dot;
		ELSE w=get(dot,itype);
		     IF longpr
		     THEN wx=itol(w,get(inkdot(2),itype));
		     ELSE wx=w;
		     FI
		FI
		IF c=='F'
		THEN fw.sb=get(inkdot(4),itype);
		     fw.sc=get(inkdot(6),itype);
		FI
		IF errflg THEN return(fp); FI
		IF mkfault THEN error((char *)0); FI
		var[0]=wx;
		modifier = *fp++;
		dotinc=(longpr?4:2);;

		if (!(printptr - printbuf) && modifier != 'a')
			print("%16m");

		switch(modifier) {

		    case SP: case TB:
			break;

		    case 't': case 'T':
			print("%T",fcount); return(fp);

		    case 'r': case 'R':
			print("%M",fcount); return(fp);

		    case 'a':
			psymoff(dot,ptype,":%16t"); dotinc=0; break;

		    case 'p':
			psymoff(var[0],ptype,"%16t"); break;

		    case 'u':
			print("%-8u",w); break;

		    case 'U':
			print("%-16U",wx); break;

		    case 'c': case 'C':
			IF modifier=='C'
			THEN printesc(w&LOBYTE);
			ELSE printc(w&LOBYTE);
			FI
			dotinc=1; break;

		    case 'b': case 'B':
			print("%-8o", w&LOBYTE); dotinc=1; break;

		    case 's': case 'S':
			savdot=dot; dotinc=1;
			WHILE (c=get(dot,itype)&LOBYTE) ANDF errflg==0
			DO dot=inkdot(1);
			   IF modifier == 'S'
			   THEN printesc(c);
			   ELSE printc(c);
			   FI
			   endline();
			OD
			dotinc=dot-savdot+1; dot=savdot; break;

		    case 'x':
			print("%-8x",w); break;

		    case 'X':
			print("%-16X", wx); break;

		    case 'Y':
			print("%-24Y", wx); break;

		    case 'q':
			print("%-8q", w); break;

		    case 'Q':
			print("%-16Q", wx); break;

		    case 'o':
		    case 'w':
			print("%-8o", w); break;

		    case 'O':
		    case 'W':
			print("%-16O", wx); break;

		    case 'i':
			printins(itype,w); printc(EOR); break;

		    case 'd':
			print("%-8d", w); break;

		    case 'D':
			print("%-16D", wx); break;

		    case 'f':
			*(double *)&fw = 0.0;
			fw.sa = wx;
			print("%-16.9f", *(double *)&fw);
			dotinc=4; break;

		    case 'F':
			fw.sa = wx;
			print("%-32.18F", *(double *)&fw);
			dotinc=8; break;

		    case 'n': case 'N':
			printc('\n'); dotinc=0; break;

		    case '"':
			dotinc=0;
			WHILE *fp != '"' ANDF *fp
			DO printc(*fp++); OD
			IF *fp THEN fp++; FI
			break;

		    case '^':
			dot=inkdot(-dotinc*fcount); return(fp);

		    case '+':
			dot=inkdot(fcount); return(fp);

		    case '-':
			dot=inkdot(-fcount); return(fp);

		    default: error(BADMOD);
		}
		IF itype!=NSP
		THEN	dot=inkdot(dotinc);
		FI
		fcount--; endline();
	OD

	return(fp);
}

unox()
{
	int	rc, status, unixpid;
	char	*argp = lp;

	WHILE lastc!=EOR DO rdc(); OD
	IF (unixpid=fork())==0
	THEN	signal(SIGINT, sigint);
                signal(SIGQUIT, sigqit);
		*lp=0; execl("/bin/sh", "sh", "-c", argp, 0);
		exit(16);
	ELIF unixpid == -1
	THEN	error(NOFORK);
	ELSE	signal(SIGINT,SIG_IGN);
		WHILE (rc = wait(&status)) != unixpid ANDF rc != -1 DONE
		signal(SIGINT, sigint);
		printc('!'); lp--;
	FI
}


printesc(c)
{
	c &= STRIP;
	IF c<SP ORF c>'~' ORF c=='@'
	THEN print("@%c",(c=='@' ? '@' : c^0140));
	ELSE printc(c);
	FI
}

long
inkdot(incr)
{
	long	newdot;

	newdot=dot+incr;
	IF (dot ^ newdot) >> 24 THEN error(ADWRAP); FI
	return(newdot);
}
