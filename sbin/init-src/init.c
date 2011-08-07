/*
 * Copyright (c) 1980,1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <sys/sysctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <utmp.h>
#include <setjmp.h>
#include <sys/reboot.h>
#include <errno.h>
#include <sys/file.h>
#include <ttyent.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <paths.h>

//#undef KERN_SECURELVL

#define	LINSIZ	sizeof(wtmp.ut_line)
#define	CMDSIZ	200	/* max string length for getty or window command*/
#define SCPYN(a, b)	strncpy(a, b, sizeof(a))
#define SCMPN(a, b)	strncmp(a, b, sizeof(a))

char	shell[]	= _PATH_BSHELL;
char	minus[]	= "-";
char	runc[]	= "/etc/rc";
char	utmpf[]	= _PATH_UTMP;
char	wtmpf[]	= _PATH_WTMP;
char	ctty[]	= _PATH_CONSOLE;

extern int execl (const char *path, const char *arg0, ...);
void merge (int signum);

struct utmp wtmp;
struct	tab
{
	char	line[LINSIZ];
	char	comn[CMDSIZ];
	char	xflag;
	int	pid;
	int	wpid;		/* window system pid for SIGHUP	*/
	char	wcmd[CMDSIZ];	/* command to start window system process */
	time_t	gettytime;
	int	gettycnt;
	time_t	windtime;
	int	windcnt;
	struct	tab *next;
} *itab;

int	fi;
int	mergflag;
char	tty[20];
jmp_buf	sjbuf, shutpass;
time_t	time0;

void	reset();
void	setsecuritylevel();
int	getsecuritylevel();
extern	int errno;

struct	sigvec rvec = { reset, sigmask (SIGHUP), 0 };

jmp_buf	idlebuf;

#if 1
#include <stdarg.h>

void mylog (const char *fmt, ...)
{
	va_list ap;
	char buf [200];
	int out;

	va_start (ap, fmt);
	vsprintf (buf, fmt, ap);
	va_end (ap);
        out = open (ctty, O_RDWR, 0);
        if (out >= 0) {
                write (out, buf, strlen(buf));
                write (out, "\n", 1);
                close (out);
        }
}
#endif

/*
 * Catch a SIGSYS signal.
 *
 * These may arise if a system does not support sysctl.
 * We tolerate up to 25 of these, then throw in the towel.
 */
void
badsys (sig)
	int sig;
{
	static int badcount = 0;

	if (badcount++ < 25)
		return;
	syslog(LOG_EMERG, "fatal signal: %d", sig);
	sleep(30);
	_exit(sig);
}

void
idlehup (sig)
	int sig;
{

	longjmp (idlebuf, 1);
}

void
idle (sig)
	int sig;
{
	register struct tab *p;
	register pid;

	signal (SIGHUP, idlehup);
	for (;;) {
		if (setjmp(idlebuf))
			return;
		pid = wait((int *) 0);
		if (pid == -1) {
			sigpause(0L);
			continue;
		}
		for (p=itab; p; p=p->next) {
			/* if window system dies, mark it for restart */
			if (p->wpid == pid)
				p->wpid = -1;
			if (p->pid == pid) {
				rmut(p);
				p->pid = -1;
			}
		}
	}
}

main(argc, argv)
	char **argv;
{
#if 0
        /* Trivial init: just start shell. */
        int fd = open(ctty, O_RDWR, 0);
        if (fd < 0)
                return 0;
	write(fd, "init: starting /bin/sh\n", 23);
        if (fd > 0)
                dup2(fd, 0);
        dup2(0, 1);
        dup2(0, 2);
        execl(shell, minus, (char *)0);
        perror(shell);
        return 0;
#else
	int howto, oldhowto;

	time0 = time(0);
	if (argc > 1 && argv[1][0] == '-') {
		char *cp;

		howto = 0;
		cp = &argv[1][1];
		while (*cp) switch (*cp++) {
		case 'a':
			howto |= RB_ASKNAME;
			break;
		case 's':
			howto |= RB_SINGLE;
			break;
		}
	} else {
		howto = RB_SINGLE;
	}
	if (getuid() != 0)
		exit(1);
	if (getpid() != 1)
		exit(1);

	openlog("init", LOG_CONS|LOG_ODELAY, LOG_AUTH);
//mylog("init: starting");

	signal(SIGSYS, badsys);
	sigvec(SIGTERM, &rvec, (struct sigvec *)0);
	signal(SIGTSTP, idle);
	signal(SIGSTOP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	(void) setjmp(sjbuf);
	for (;;) {
		oldhowto = howto;
		howto = RB_SINGLE;
		if (setjmp(shutpass) == 0)
			shutdown();
		if (oldhowto & RB_SINGLE)
			single();
		if (runcom(oldhowto) == 0)
			continue;
		merge(0);
		multiple();
	}
#endif
}

const char shutfailm[] = "WARNING: Something is hung (wont die); ps axl advised\n";

void
shutreset (sig)
	int sig;
{
	int status;

	if (fork() == 0) {
		int ct = open(ctty, 1);
		write(ct, shutfailm, sizeof (shutfailm));
		sleep(1);
		exit(1);
	}
	//sleep(1);
	shutend();
	longjmp(shutpass, 1);
}

shutdown()
{
	register i;
	register struct tab *p, *p1;

	close(creat(utmpf, 0644));
	signal(SIGHUP, SIG_IGN);
	for (p = itab; p ; ) {
		term(p);
		p1 = p->next;
		free(p);
		p = p1;
	}
	itab = (struct tab *)0;
	signal(SIGALRM, shutreset);
	(void) kill(-1, SIGTERM);	/* one chance to catch it */
	//sleep(1);
	alarm(30);
	for (i = 0; i < 5; i++)
		kill(-1, SIGKILL);
	while (wait((int *)0) != -1)
		;
	alarm(0);
	shutend();
}

shutend()
{
	register i, f;

	signal(SIGALRM, SIG_DFL);
	for (i = 0; i < 10; i++)
		close(i);
	f = open(wtmpf, O_WRONLY|O_APPEND);
	if (f >= 0) {
		SCPYN(wtmp.ut_line, "~");
		SCPYN(wtmp.ut_name, "shutdown");
		SCPYN(wtmp.ut_host, "");
		time(&wtmp.ut_time);
		write(f, (char *)&wtmp, sizeof(wtmp));
		close(f);
	}
	return (1);
}

/*
 * Get the security level of the kernel.
 */
int
getsecuritylevel()
{
#ifdef KERN_SECURELVL
	int name[2], curlevel;
	size_t len;

	name[0] = CTL_KERN;
	name[1] = KERN_SECURELVL;
	len = sizeof curlevel;
	if (sysctl(name, 2, &curlevel, &len, NULL, 0) == -1) {
		syslog(LOG_EMERG, "cannot get kernel security level: %s",
		    strerror(errno));
		return (-1);
	}
	return (curlevel);
#else
	return (-1);
#endif
}

/*
 * Set the security level of the kernel.
 */
void
setsecuritylevel(newlevel)
	int newlevel;
{
#ifdef KERN_SECURELVL
	int name[2], curlevel;

	curlevel = getsecuritylevel();
	if (newlevel == curlevel)
		return;
	name[0] = CTL_KERN;
	name[1] = KERN_SECURELVL;
	if (sysctl(name, 2, NULL, NULL, &newlevel, sizeof newlevel) == -1) {
		syslog(LOG_EMERG,
		    "cannot change kernel security level from %d to %d: %s",
		    curlevel, newlevel, strerror(errno));
		return;
	}
	syslog(LOG_ALERT, "kernel security level changed from %d to %d",
	    curlevel, newlevel);
#endif
}

single()
{
	register pid;
	register xpid;
	int	fd;

	/*
	 * If the kernel is in secure mode, downgrade it to insecure mode.
	 */
	if (getsecuritylevel() > 0)
		setsecuritylevel(0);

	do {
		pid = fork();
		if (pid == 0) {
			signal(SIGTERM, SIG_DFL);
			signal(SIGHUP, SIG_DFL);
			signal(SIGALRM, SIG_DFL);
			signal(SIGTSTP, SIG_IGN);
			fd = open(ctty, O_RDWR, 0);
			if (fd)
				dup2(fd, 0);
			dup2(0, 1);
			dup2(0, 2);
			execl(shell, minus, (char *)0);
			perror(shell);
			exit(0);
		}
		while ((xpid = wait((int *)0)) != pid)
			if (xpid == -1 && errno == ECHILD)
				break;
	} while (xpid == -1);
}

runcom(oldhowto)
	int oldhowto;
{
	register pid, f;
	int status;
	char *arg1, *arg2;

//mylog("start %s", runc);
	pid = fork();
	if (pid == 0) {
		f = open("/", O_RDONLY, 0);
		if (f > 0)
			dup2(f, 0);
		dup2(0, 1);
		dup2(0, 2);
		if (oldhowto & RB_SINGLE)
			execl(shell, shell, runc, (char *)0);
		else
			execl(shell, shell, runc, "autoboot", (char *)0);
		exit(1);
	}
	while (wait(&status) != pid)
		;
	if (status)
		return (0);
	f = open(wtmpf, O_WRONLY|O_APPEND);
	if (f >= 0) {
		SCPYN(wtmp.ut_line, "~");
		SCPYN(wtmp.ut_name, "reboot");
		SCPYN(wtmp.ut_host, "");
		if (time0) {
			wtmp.ut_time = time0;
			time0 = 0;
		} else
			time(&wtmp.ut_time);
		write(f, (char *)&wtmp, sizeof(wtmp));
		close(f);
	}
	return (1);
}

struct	sigvec	mvec = { merge, sigmask(SIGTERM), 0 };

/*
 * Multi-user.  Listen for users leaving, SIGHUP's
 * which indicate ttys has changed, and SIGTERM's which
 * are used to shutdown the system.
 */
multiple()
{
	register struct tab *p;
	register pid;
	long omask;

	/*
	 * If the administrator has not set the security level to -1
	 * to indicate that the kernel should not run multiuser in secure
	 * mode, and the run script has not set a higher level of security
	 * than level 1, then put the kernel into secure mode.
	 */
	if (getsecuritylevel() == 0)
		setsecuritylevel(1);

	sigvec(SIGHUP, &mvec, (struct sigvec *)0);
	for (;;) {
//mylog("waiting for child termination");
		pid = wait((int *)0);
		if (pid == -1)
			return;
//mylog("child pid %d terminated", pid);
		omask = sigblock(sigmask(SIGHUP));
		for (p=itab; p; p=p->next) {
			/* must restart window system BEFORE emulator */
			if (p->wpid == pid || p->wpid == -1)
				wstart(p);
			if (p->pid == pid || p->pid == -1) {
				/* disown the window system */
				if (p->wpid)
					kill(p->wpid, SIGHUP);
				rmut(p);
				dfork(p);
			}
		}
		sigsetmask(omask);
	}
}

/*
 * Merge current contents of ttys file
 * into in-core table of configured tty lines.
 * Entered as signal handler for SIGHUP.
 */
#define	FOUND	1
#define	CHANGE	2
#define WCHANGE 4

void merge(int signum)
{
	register struct tab *p;
	register struct ttyent *t;
	register struct tab *p1;

	for (p=itab; p; p=p->next)
		p->xflag = 0;
	setttyent();
	while (t = getttyent()) {
//mylog("ty_name %s, ty_getty = %s, ty_type = %s, ty_status = %#x, ty_comment = %s", t->ty_name, t->ty_getty, t->ty_type, t->ty_status, t->ty_comment);
		if ((t->ty_status & TTY_ON) == 0) {
//mylog("ty_status OFF");
			continue;
}
		for (p=itab; p; p=p->next) {
			if (SCMPN(p->line, t->ty_name))
				continue;
			p->xflag |= FOUND;
			if (SCMPN(p->comn, t->ty_getty)) {
				p->xflag |= CHANGE;
				SCPYN(p->comn, t->ty_getty);
			}
			if (SCMPN(p->wcmd, t->ty_window ? t->ty_window : "")) {
				p->xflag |= WCHANGE | CHANGE;
				SCPYN(p->wcmd, t->ty_window);
			}
			goto contin1;
		}

		/*
		 * Make space for a new one
		 */
		p1 = (struct tab *)calloc(1, sizeof(*p1));
		if (!p1) {
//mylog("no space for '%s' !?!", t->ty_name);
			syslog(LOG_ERR, "no space for '%s' !?!", t->ty_name);
			goto contin1;
		}
		/*
		 * Put new terminal at the end of the linked list.
		 */
		if (itab) {
			for (p = itab; p->next ; p = p->next)
				;
			p->next = p1;
		} else
			itab = p1;

		p = p1;
		SCPYN(p->line, t->ty_name);
		p->xflag |= FOUND|CHANGE;
		SCPYN(p->comn, t->ty_getty);
		if (t->ty_window && strcmp(t->ty_window, "") != 0) {
			p->xflag |= WCHANGE;
			SCPYN(p->wcmd, t->ty_window);
		}
//mylog("new itab entry");
	contin1:
		;
	}
	endttyent();
	p1 = (struct tab *)0;
//mylog("done ttyent");
	for (p=itab; p; p=p->next) {
//mylog("p->comm = %s, p->line = %s", p->comn, p->line);
		if ((p->xflag & FOUND) == 0) {
			term(p);
			wterm(p);
			if (p1)
				p1->next = p->next;
			else
				itab = p->next;
			free(p);
			p = p1 ? p1 : itab;
		} else {
			/* window system should be started first */
			if (p->xflag & WCHANGE) {
				wterm(p);
				wstart(p);
			}
			if (p->xflag & CHANGE) {
				term(p);
				dfork(p);
			}
		}
		p1 = p;
	}
}

term(p)
	register struct tab *p;
{

	if (p->pid != 0) {
		rmut(p);
		kill(p->pid, SIGKILL);
	}
	p->pid = 0;
	/* send SIGHUP to get rid of connections */
	if (p->wpid > 0)
		kill(p->wpid, SIGHUP);
}

dfork(p)
	struct tab *p;
{
	register pid;
	time_t t;
	int dowait = 0;

	time(&t);
	p->gettycnt++;
	if ((t - p->gettytime) >= 60) {
		p->gettytime = t;
		p->gettycnt = 1;
	} else if (p->gettycnt >= 5) {
		dowait = 1;
		p->gettytime = t;
		p->gettycnt = 1;
	}
	pid = fork();
	if (pid == 0) {
		signal(SIGTERM, SIG_DFL);
		signal(SIGHUP, SIG_IGN);
		sigsetmask(0L);	/* since can be called from masked code */
		if (dowait) {
			syslog(LOG_ERR, "'%s %s' failing, sleeping", p->comn, p->line);
			closelog();
			sleep(30);
		}
		execit(p->comn, p->line);
		exit(0);
	}
	p->pid = pid;
}

/*
 * Remove utmp entry.
 */
rmut(p)
	register struct tab *p;
{
	register f;
	int found = 0;
	static unsigned utmpsize;
	static struct utmp *utmp;
	register struct utmp *u;
	int nutmp;
	struct stat statbf;

	f = open(utmpf, O_RDWR);
	if (f >= 0) {
		fstat(f, &statbf);
		if (utmpsize < statbf.st_size) {
			utmpsize = statbf.st_size + 10 * sizeof(struct utmp);
			if (utmp)
				utmp = (struct utmp *)realloc(utmp, utmpsize);
			else
				utmp = (struct utmp *)malloc(utmpsize);
			if (!utmp)
				syslog(LOG_ERR, "utmp malloc failed");
		}
		if (statbf.st_size && utmp) {
			nutmp = read(f, utmp, (int)statbf.st_size);
			nutmp /= sizeof(struct utmp);
			for (u = utmp ; u < &utmp[nutmp] ; u++) {
				if (SCMPN(u->ut_line, p->line) ||
				    u->ut_name[0]==0)
					continue;
				lseek(f, ((long)u)-((long)utmp), L_SET);
				SCPYN(u->ut_name, "");
				SCPYN(u->ut_host, "");
				time(&u->ut_time);
				write(f, (char *)u, sizeof(*u));
				found++;
			}
		}
		close(f);
	}
	if (found) {
		f = open(wtmpf, O_WRONLY|O_APPEND);
		if (f >= 0) {
			SCPYN(wtmp.ut_line, p->line);
			SCPYN(wtmp.ut_name, "");
			SCPYN(wtmp.ut_host, "");
			time(&wtmp.ut_time);
			write(f, (char *)&wtmp, sizeof(wtmp));
			close(f);
		}
		/*
		 * After a proper login force reset
		 * of error detection code in dfork.
		 */
		p->gettytime = 0;
		p->windtime = 0;
	}
}

void
reset()
{
	longjmp(sjbuf, 1);
}

wterm(p)
	register struct tab *p;
{
	if (p->wpid != 0) {
		kill(p->wpid, SIGKILL);
                p->wpid = 0;
	}
}

wstart(p)
	register struct tab *p;
{
	register pid;
	time_t t;
	int dowait = 0;

	time(&t);
	p->windcnt++;
	if ((t - p->windtime) >= 60) {
		p->windtime = t;
		p->windcnt = 1;
	} else if (p->windcnt >= 5) {
		dowait = 1;
		p->windtime = t;
		p->windcnt = 1;
	}

	pid = fork();

	if (pid == 0) {
		signal(SIGTERM, SIG_DFL);
		signal(SIGHUP,  SIG_IGN);
		sigsetmask(0L);	/* since can be called from masked code */
		if (dowait) {
			syslog(LOG_ERR, "'%s %s' failing, sleeping", p->wcmd, p->line);
			closelog();
			sleep(30);
		}
		execit(p->wcmd, p->line);
		exit(0);
	}
	p->wpid = pid;
}

#define NARGS	20	/* must be at least 4 */
#define ARGLEN	512	/* total size for all the argument strings */

execit(s, arg)
	char *s;
	char *arg;	/* last argument on line */
{
	char *argv[NARGS], args[ARGLEN], *envp[1];
	register char *sp = s;
	register char *ap = args;
	register char c;
	register int i;

	/*
	 * First we have to set up the argument vector.
	 * "prog arg1 arg2" maps to exec("prog", "-", "arg1", "arg2").
	 */
	for (i = 1; i < NARGS - 2; i++) {
		argv[i] = ap;
		for (;;) {
			if ((c = *sp++) == '\0' || ap >= &args[ARGLEN-1]) {
				*ap = '\0';
				goto done;
			}
			if (c == ' ') {
				*ap++ = '\0';
				while (*sp == ' ')
					sp++;
				if (*sp == '\0')
					goto done;
				break;
			}
			*ap++ = c;
		}
	}
done:
	argv[0] = argv[1];
	argv[1] = "-";
	argv[i+1] = arg;
	argv[i+2] = 0;
	envp[0] = 0;
	execve(argv[0], &argv[1], envp);
	/* report failure of exec */
	syslog(LOG_ERR, "%s: %m", argv[0]);
	closelog();
	sleep(10);	/* prevent failures from eating machine */
}
