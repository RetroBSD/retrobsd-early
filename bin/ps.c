/*
 *	1999/8/11 - Remove reference to SDETACH.  It was removed from the kernel
 *		    (finally) because it was not needed.
 *
 *	1997/12/16 - Fix coredump when processing -U.
 *
 *	1996/11/16 - Move 'psdatabase' in /var/run.
 *
 *	12/20/94 - Missing casts caused errors in reporting on swapped
 *		   processes - sms
 *	1/7/93 - Heavily revised when the symbol table format changed - sms
 *
 *	ps - process status
 *	Usage:  ps [ acgklnrtuwxU# ] [ corefile [ swapfile [ system ] ] ]
 */
#define PIC32MX

#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <a.out.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <utmp.h>
#include <psout.h>

#define	within(x,y,z)	(((unsigned)(x) >= (y)) && ((unsigned)(x) < (z)))
#define	round(x,y) ((long) ((((long) (x) + (long) (y) - 1L) / (long) (y)) * (long) (y)))

struct	nlist nl[] = {
#define	X_PROC		0
	{ "_proc" },
#define X_NPROC		1
	{ "_nproc" },
#define	X_HZ		2
	{ "_hz" },
	{ "_ipc" },
	{ "_lbolt" },
	{ "_memlock" },
	{ "_runin" },
	{ "_runout" },
	{ "_selwait" },
	{ "_u" },
	{ "" },
};

/*
 * This is no longer the size of a symbol's name, symbols can be much
 * larger now.  This define says how many characters of a symbol's name
 * to save in the wait channel name.  A new define has been created to
 * limit the size of symbol string read from the string table.
*/
#define	NNAMESIZ	8
#define	MAXSYMLEN	32

struct	proc *mproc, proc[8];
struct	user	u;
int	hz;
int	chkpid	= 0;
char	aflg;	/* -a: all processes, not just mine */
char	cflg;	/* -c: not complete listing of args, just comm. */
char	gflg;	/* -g: complete listing including group headers, etc */
char	kflg;	/* -k: read from core file instead of real memory */
char	lflg;	/* -l: long listing form */
char	nflg;	/* -n: numeric wchans */
char	rflg;	/* -r: raw output in style <psout.h> */
char	uflg;	/* -u: user name */
char	wflg;	/* -w[w]: wide terminal */
char	xflg;	/* -x: ALL processes, even those without ttys */
char	*tptr, *mytty;
char	*uname;
int	file;
int	nproc;
int	nchans;
int	nttys;
int	npr;			/* number of processes found so far */
int	twidth;			/* terminal width */
int	cmdstart;		/* start position for command field */
char	*memf;			/* name of kernel memory file */
char	*kmemf = "/dev/kmem";	/* name of physical memory file */
char	*swapf;			/* name of swap file to use */
int	kmem, mem, swap;

/*
 *	Structure for the unix wchan table
 */
typedef struct wchan {
	char	cname[NNAMESIZ];
	unsigned	caddr;
} WCHAN;

WCHAN	*wchand;

char	*gettty(), *getptr(), *getchan();
int	pscomp(), wchancomp();

/*
 * 256 terminals was not only wasteful but unrealistic.  For one thing
 * 2.11BSD uses bit 7 of the minor device (for almost all terminal interfaces)
 * to indicate direct/modem status - this means that 128 terminals is
 * a better maximum number of terminals, for another thing the system can't
 * support 256 terminals - other resources (memory, files, processes) will
 * have been exhausted long ago.  If 'ps' complains about too many terminals
 * it is time to clean up /dev!
 */
#define	MAXTTYS		160	/* 128 plus a few extra */

struct	ttys {
	char	name[14];	/* MAXNAMLEN uses too much memory,  besides */
				/* device names tend to be very short */
	dev_t	ttyd;

} allttys[MAXTTYS];

struct	map {
	off_t	b1, e1; off_t	f1;
	off_t	b2, e2; off_t	f2;
};

struct	winsize ws;
struct	map datmap;
struct	psout *outargs;		/* info for first npr processes */

main (argc, argv)
char	**argv;
{
	int	uid, euid, puid, nread;
	register int i, j;
	char	*ap;
	register struct	proc	*procp;

	if ((ioctl(fileno(stdout), TIOCGWINSZ, &ws) != -1 &&
	     ioctl(fileno(stderr), TIOCGWINSZ, &ws) != -1 &&
	     ioctl(fileno(stdin), TIOCGWINSZ, &ws) != -1) ||
	     ws.ws_col == 0)
	 	twidth = 80;
	else
		twidth = ws.ws_col;

	argc--, argv++;
	if (argc > 0) {
		ap = argv [0];
		while (*ap) switch (*ap++) {
		case '-':
			break;

		case 'a':
			aflg++;
			break;

		case 'c':
			cflg++;
			break;

		case 'g':
			gflg++;
			break;

		case 'k':
			kflg++;
			break;

		case 'l':
			lflg	= 1;
			break;

		case 'n':
			nflg++;
			lflg	= 1;
			break;

		case 'r':
			rflg++;
			break;

		case 't':
			if (*ap)
				tptr = ap;
			else if ((tptr = ttyname(0)) != 0) {
				tptr = strcpy(mytty, tptr);
				if (strncmp(tptr, "/dev/", 5) == 0)
					tptr += 5;
			}
			if (strncmp(tptr, "tty", 3) == 0)
				tptr += 3;
			aflg++;
			gflg++;
			if (tptr && *tptr == '?')
				xflg++;
			while (*ap)
				ap++;
			break;

		case 'u':
			uflg	= 1;
			break;

		case 'w':
			if (wflg)
				twidth	= BUFSIZ;
			else if (twidth < 132)
				twidth	= 132;
			wflg++;
			break;

		case 'x':
			xflg++;
			break;

		default:
			if (!isdigit(ap[-1]))
				break;
			chkpid	= atoi(--ap);
			*ap = '\0';
			aflg++;
			xflg++;
			break;
		}
	}

	openfiles(argc, argv);
	getkvars(argc, argv);
	lseek(kmem, (off_t)nl[X_PROC].n_value, 0);
	uid = getuid();
	euid = geteuid();
	mytty = ttyname(0);
	if (!strncmp(mytty,"/dev/",5)) mytty += 5;
	if (!strncmp(mytty,"tty",3)) mytty += 3;
	printhdr();
	for (i = 0; i < nproc; i += 8) {
		j = nproc - i;
		if (j > 8)
			j = 8;
		j *= sizeof (struct proc);
		if ((nread = read(kmem, (char *) proc, j)) != j) {
                        fprintf(stderr, "ps: error reading proc table from %s\n", kmemf);
			if (nread == -1)
				break;
		}
		for (j = nread / sizeof (struct proc) - 1; j >= 0; j--) {
			mproc	= &proc[j];
			procp	= mproc;
			/* skip processes that don't exist */
			if (procp->p_stat == 0)
				continue;
			/* skip those without a tty unless -x */
			if (procp->p_pgrp == 0 && xflg == 0)
				continue;
			/* skip group leaders on a tty unless -g, -x, or -t.. */
			if (!tptr && !gflg && !xflg && procp->p_ppid == 1)
				continue;
			/* -g also skips those where **argv is "-" - see savcom */
			puid = procp->p_uid;
			/* skip other peoples processes unless -a or a specific pid */
			if ((uid != puid && euid != puid && aflg == 0) ||
			    (chkpid != 0 && chkpid != procp->p_pid))
				continue;
			if (savcom(puid))
				npr++;
		}
	}
	fixup(npr);
	for (i = 0; i < npr; i++) {
		register int	cmdwidth = twidth - cmdstart - 2;
		register struct	psout *a = &outargs[i];

		if (rflg) {
			if (write(1, (char *) a, sizeof (*a)) != sizeof (*a))
				perror("write");
			continue;
		} else if (lflg)
			lpr(a);
		else if (uflg)
			upr(a);
		else
			spr(a);

		if (cmdwidth < 0)
			cmdwidth = 80 - cmdstart - 2;
		if (a->o_stat == SZOMB)
			printf("%.*s", cmdwidth, " <defunct>");
		else if (a->o_pid == 0)
			printf("%.*s", cmdwidth, " swapper");
		else
			printf(" %.*s", twidth - cmdstart - 2, cflg ?  a->o_comm : a->o_args);
		putchar('\n');
	}
	exit(!npr);
}

getdev()
{
	register DIR *df;
	register struct direct *dbuf;

	if (chdir("/dev") < 0) {
		perror("/dev");
		exit(1);
	}
	if ((df = opendir(".")) == NULL) {
		fprintf(stderr, "Can't open . in /dev\n");
		exit(1);
	}
	while (dbuf = readdir(df))
		maybetty(dbuf->d_name);
	closedir(df);
}

/*
 * Attempt to avoid stats by guessing minor device
 * numbers from tty names.  Console is known,
 * know that r(hp|up|mt) are unlikely as are different mem's,
 * floppy, null, tty, etc.
 */
maybetty(cp)
	register char *cp;
{
	register struct ttys *dp;
	struct stat stb;

	switch (cp[0]) {

	case 'c':
		if (!strcmp(cp, "console"))
			break;
		/* cu[la]? are possible!?! don't rule them out */
		break;

	case 'd':
		if (!strcmp(cp, "drum"))
			return;
		break;

	case 'f':
		if (!strcmp(cp, "floppy"))
			return;
		break;

	case 'k':
		if (!strcmp(cp, "kUmem") || !strcmp(cp, "kmem"))
			return;
		if (!strcmp(cp, "klog"))
			return;
		break;

	case 'r':
#define is(a,b) cp[1] == 'a' && cp[2] == 'b'
		if (is(h,p) || is(r,a) || is(u,p) || is(h,k) || is(x,p)
		    || is(r,b) || is(r,l) || is(m,t)) {
			if (isdigit(cp[3]))
				return;
		}
		break;

	case 'm':
		if (!strcmp("mem", cp))
			return;
		if (cp[1] == 't')
			return;
		break;

	case 'n':
		if (!strcmp(cp, "null"))
			return;
		if (!strncmp(cp, "nrmt", 4))
			return;
		break;

	case 'p':
		if (cp[1] == 't' && cp[2] == 'y')
			return;
		break;
	}
	if (nttys >= MAXTTYS) {
		fprintf(stderr, "ps: tty table overflow\n");
		exit(1);
	}
	dp = &allttys[nttys++];
	(void)strcpy(dp->name, cp);
	dp->ttyd = -1;
}

/*
 * Save command data to outargs[].
 * Return 1 on success.
 */
savcom(puid)
{
	char	*tp;
	off_t	addr;
	off_t	daddr, saddr;
	register struct	psout	*a;
	register struct	proc	*procp	= mproc;
	register struct	user	*up	= &u;
	long	txtsiz, datsiz, stksiz;

	if (procp->p_flag & SLOAD) {
		addr = procp->p_addr;
		daddr = procp->p_daddr;
		saddr = procp->p_saddr;
		file = mem;
	} else {
		addr = (off_t)procp->p_addr * DEV_BSIZE;
		daddr = (off_t)procp->p_daddr * DEV_BSIZE;
		saddr = (off_t)procp->p_saddr * DEV_BSIZE;
		file = swap;
	}
	lseek(file, addr, 0);
	if (read(file, (char *) up, sizeof (u)) != sizeof (u))
		return(0);

	txtsiz = up->u_tsize;		/* set up address maps for user pcs */
	datsiz = up->u_dsize;
	stksiz = up->u_ssize;
	datmap.b1 = txtsiz;
	datmap.e1 = datmap.b1 + datsiz;
	datmap.f1 = daddr;
	datmap.f2 = saddr;
	datmap.b2 = stackbas(stksiz);
	datmap.e2 = stacktop(stksiz);
	tp = gettty();
//	if ((tptr && strncmp(tptr, tp, 2) != 0) ||
//          (! aflg && strncmp(mytty, tp, 2) != 0))
//		return(0);
	a = &outargs[npr];		/* saving com starts here */
	a->o_uid = puid;
	a->o_pid = procp->p_pid;
	a->o_flag = procp->p_flag;
	a->o_ppid = procp->p_ppid;
	a->o_cpu  = procp->p_cpu;
	a->o_pri = procp->p_pri;
	a->o_nice = procp->p_nice;
	a->o_addr0 = procp->p_addr;
	a->o_size = (procp->p_dsize + procp->p_ssize + USIZE) / DEV_BSIZE;
	a->o_wchan = procp->p_wchan;
	a->o_pgrp = procp->p_pgrp;
	strncpy(a->o_tty, tp, sizeof (a->o_tty));
	a->o_ttyd = tp[0] == '?' ?  -1 : up->u_ttyd;
	a->o_stat = procp->p_stat;
	a->o_flag = procp->p_flag;

	if (a->o_stat == SZOMB)
		return(1);
	a->o_utime = up->u_ru.ru_utime;
	a->o_stime = up->u_ru.ru_stime;
	a->o_cutime = up->u_cru.ru_utime;
	a->o_cstime = up->u_cru.ru_stime;
	a->o_sigs = (int)up->u_signal[SIGINT] + (int)up->u_signal[SIGQUIT];
	a->o_uname[0] = 0;
	strncpy(a->o_comm, up->u_comm, MAXCOMLEN);
	if (cflg)
		return (1);
	else
		return getcmd(a, saddr);
}

char *
gettty()
{
	register int tty_step;
	register char *p;

	if (u.u_ttyp)
		for (tty_step = 0;tty_step < nttys;++tty_step)
			if (allttys[tty_step].ttyd == u.u_ttyd) {
				p = allttys[tty_step].name;
				if (!strncmp(p,"tty",3))
					p += 3;
				return(p);
			}
	return("?");
}

/*
 * fixup figures out everybodys name and sorts into a nice order.
 */
fixup(np)
register int np;
{
	register int i;
	register struct	passwd	*pw;

	if (uflg) {
		/*
		 * If we want names, traverse the password file. For each
		 * passwd entry, look for it in the processes.
		 * In case of multiple entries in the password file we believe
		 * the first one (same thing ls does).
		 */
		while ((pw = getpwent()) != (struct passwd *) NULL) {
			for (i = 0; i < np; i++)
				if (outargs[i].o_uid == pw->pw_uid) {
					if (outargs[i].o_uname[0] == 0)
						strcpy(outargs[i].o_uname, pw->pw_name);
					}
			}
		}
	qsort(outargs, np, sizeof (outargs[0]), pscomp);
}

pscomp(x1, x2)
register struct	psout	*x1, *x2;
{
	register int c;

	c = (x1)->o_ttyd - (x2)->o_ttyd;
	if (c == 0)
		c = (x1)->o_pid - (x2)->o_pid;
	return(c);
}

wchancomp(x1, x2)
register WCHAN	*x1, *x2;
{
	if (x1->caddr > x2->caddr)
		return(1);
	else if (x1->caddr == x2->caddr)
		return(0);
	else
		return(-1);
}

char	*
getptr(adr)
char	**adr;
{
	char	*ptr = 0;
	register char	*p, *pa;
	register int	i;

	pa = (char *)adr;
	p  = (char *)&ptr;
	for (i = 0; i < sizeof (ptr); i++)
		*p++ = getbyte(pa++);
	return(ptr);
}

getbyte(adr)
register char	*adr;
{
	register struct	map *amap = &datmap;
	char	b;
	off_t	saddr;

	if (!within(adr, amap->b1, amap->e1))
		if (within(adr, amap->b2, amap->e2))
			saddr = (unsigned) adr + amap->f2 - amap->b2;
		else	return(0);
	else	saddr	= (unsigned) adr + amap->f1 - amap->b1;
	if (lseek(file, saddr, 0) == (off_t) -1 || read (file, &b, 1) < 1)
		return(0);
	return((unsigned) b);
}

addchan(name, caddr)
	char	*name;
	unsigned caddr;
{
	static	int	left = 0;
	register WCHAN	*wp;

	if (left == 0) {
		if (wchand) {
			left = 50;
			wchand = (WCHAN *)realloc(wchand, (nchans + left) *
						sizeof (struct wchan));
		} else {
			left = 300;
			wchand = (WCHAN *)malloc(left * sizeof (struct wchan));
		}
		if (! wchand) {
			fprintf(stderr, "ps: out of wait channel memory\n");
			nflg++;
			return;
		}
	}
	wp = &wchand[nchans++];
	left--;
	strncpy(wp->cname, name, NNAMESIZ - 1);
	wp->cname[NNAMESIZ-1] = '\0';
	wp->caddr = caddr;
}

char	*
getchan(chan)
register unsigned int chan;
{
	register int	i;
	register char	*prevsym;

	prevsym	= "";
	if (chan) {
		for (i = 0; i < nchans; i++) {
			if (wchand[i].caddr > chan)
				return (prevsym);
			prevsym = wchand[i].cname;
		}
        }
	return(prevsym);
}

perrexit(msg)
char *msg;
{
	perror(msg);
	exit(1);
}

openfiles(argc, argv)
char	**argv;
{
	if (kflg)
		kmemf = argc > 1 ?  argv[1] : "/usr/sys/core";
	kmem = open(kmemf, 0);
	if (kmem < 0)
		perrexit(kmemf);
	if (!kflg)
		memf = "/dev/mem";
	else
		memf = kmemf;
	mem = open(memf, 0);
	if (mem < 0)
		perrexit(memf);
	swapf = argc > 2 ?  argv[2] : "/dev/swap";
	swap = open(swapf, 0);
	if (swap < 0)
		perrexit(swapf);
}

getkvars(argc,argv)
int	argc;
char	**argv;
{
	struct	stat	st;

	knlist(nl);
	if (! nflg) {
                int i;
                for (i=0; i<sizeof(nl)/sizeof(*nl); i++) {
                        if (nl[i].n_value != 0)
                                addchan(nl[i].n_name + 1, nl[i].n_value);
                }
		qsort(wchand, nchans, sizeof(WCHAN), wchancomp);
        }
	getdev();

	/* find number of procs */
	if (nl[X_NPROC].n_value) {
		int ret = lseek(kmem, (off_t)nl[X_NPROC].n_value, 0);

		if (read(kmem,(char *)&nproc,sizeof(nproc)) != sizeof(nproc)) {
			perror(kmemf);
			exit(1);
		}
	} else {
		fputs("nproc not in namelist\n",stderr);
		exit(1);
	}
	outargs = (struct psout *)calloc(nproc, sizeof(struct psout));
	if (!outargs) {
		fputs("ps: not enough memory for saving info\n",stderr);
		exit(1);
	}

	/* find value of hz */
	lseek(kmem, (off_t)nl[X_HZ].n_value, 0);
	read(kmem, (char *)&hz, sizeof(hz));
}


char *uhdr = "USER       PID NICE SZ TTY TIME";
upr(a)
register struct psout	*a;
{
	printf("%-8.8s%6u%4d%4d %-2.2s",a->o_uname,a->o_pid,a->o_nice,a->o_size,a->o_tty);
	ptime(a);
}

char *shdr = "   PID TTY TIME";
spr (a)
register struct psout	*a;
{
	printf("%6u %-2.2s",a->o_pid,a->o_tty);
	ptime(a);
}

char *lhdr = "  F S   UID   PID  PPID CPU PRI NICE  ADDR  SZ WCHAN    TTY TIME";
lpr(a)
register struct psout *a;
{
	static char	clist[] = "0SWRIZT";

	printf("%3o %c%6u%6u%6u%4d%4d%4d%#7x%4d",
                0377 & a->o_flag, clist[a->o_stat], a->o_uid, a->o_pid,
                a->o_ppid, a->o_cpu & 0377, a->o_pri, a->o_nice,
                a->o_addr0, a->o_size);
	if (nflg)
		if (a->o_wchan)
			printf("%*.*o",NNAMESIZ, NNAMESIZ, a->o_wchan);
		else
			fputs("       ",stdout);
	else
		printf(" %-*.*s",NNAMESIZ, NNAMESIZ, getchan(a->o_wchan));
	printf(" %-2.2s",a->o_tty);
	ptime(a);
}

ptime(a)
struct psout	*a;
{
	time_t	tm;

	tm = (a->o_utime + a->o_stime + 30) / hz;
	printf("%3ld:",tm / 60);
	tm %= 60;
	printf(tm < 10 ? "0%ld" : "%ld",tm);
}

getcmd(a, addr)
off_t	addr;
register struct psout	*a;
{
	/* amount of top of stack to examine for args */
#define ARGLIST	(1024/sizeof(int))
	register int *ip;
	register char	*cp, *cp1;
	char	c, **ap;
	int	cc, nbad, abuf [ARGLIST];

	a->o_args[0] = 0;	/* in case of early return */
	addr += mproc->p_ssize - ARGLIST*sizeof(int);

	/* look for sh special */
	lseek(file, addr + ARGLIST*sizeof(int) - sizeof (char **), 0);
	if (read(file, (char *) &ap, sizeof (char *)) != sizeof (char *))
		return (1);
	if (ap) {
		char	b[82];
		char	*bp	= b;
		while ((cp = getptr(ap++)) && cp && (bp < b+sizeof (a->o_args)) ) {
			nbad	= 0;
			while ((c = getbyte(cp++)) && (bp < b+sizeof (a->o_args))) {
				if (c<' ' || c > '~') {
					if (nbad++ > 3)
						break;
                                        continue;
				}
				*bp++ = c;
			}
			*bp++ = ' ';
		}
		*bp++ = 0;
		(void)strcpy(a->o_args, b);
		return(1);
	}

	lseek(file, addr, 0);
	if (read(file, (char *) abuf, sizeof (abuf)) != sizeof (abuf))
		return (1);
	abuf[ARGLIST-1]	= 0;
	for (ip = &abuf[ARGLIST-2]; ip > abuf;) {
		if (*--ip == -1 || *ip == 0) {
			cp = (char *) (ip + 1);
			if (*cp == '\0')
				cp++;
			nbad = 0;
			for (cp1 = cp; cp1 < (char *) &abuf[ARGLIST]; cp1++) {
				cc = *cp1 & 0177;
				if (cc == 0)
					*cp1 = ' ';
				else if (cc < ' ' || cc > 0176) {
					if (++nbad >= 5) {
						*cp1++	= ' ';
						break;
					}
					*cp1 = '?';
				} else if (cc == '=') {
					*cp1 = '\0';
					while (cp1 > cp && *--cp1 != ' ')
						*cp1 = '\0';
					break;
				}
			}
			while (*--cp1 == ' ')
				*cp1 = 0;
			(void)strcpy(a->o_args, cp);
garbage:
			cp = a->o_args;
			if (cp[0] == '-' && cp[1] <= ' ' || cp[0] == '?' || cp[0] <= ' ') {
				strcat(cp, " (");
				strcat(cp, u.u_comm);
				strcat(cp, ")");
			}
			cp[63] = 0;	/* max room in psout is 64 chars */
			if (xflg || gflg || tptr || cp[0] != '-')
				return(1);
			return(0);
		}
	}
	goto garbage;
}

printhdr()
{
	register char	*hdr, *cmdstr	= " COMMAND";

	if (rflg)
		return;
	if (lflg && uflg) {
		fputs("ps: specify only one of l and u.\n",stderr);
		exit(1);
	}
	hdr = lflg ? lhdr : (uflg ? uhdr : shdr);
	fputs(hdr,stdout);
	cmdstart = strlen(hdr);
	if (cmdstart + strlen(cmdstr) >= twidth)
		cmdstr = " CMD";
	printf("%s\n", cmdstr);
	fflush(stdout);
}
