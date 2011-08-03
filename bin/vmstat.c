/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <nlist.h>

#include <sys/param.h>
#include <sys/file.h>
#include <sys/vm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/dir.h>
#include <sys/inode.h>
#include <sys/namei.h>

#include <machine/machparam.h>

struct nlist nl[] = {
#define	X_CPTIME	0
	{ "_cp_time" },
#define	X_RATE		1
	{ "_rate" },
#define X_TOTAL		2
	{ "_total" },
#define	X_FORKSTAT	4
	{ "_forkstat" },
#define X_SUM		5
	{ "_sum" },
#define	X_BOOTTIME	8
	{ "_boottime" },
#define	X_DKXFER	9
	{ "_dk_xfer" },
#define X_HZ		12
	{ "_hz" },
#define X_NCHSTATS	14
	{ "_nchstats" },
#define	X_DK_NDRIVE	19
	{ "_dk_ndrive" },
#define	X_DK_NAME	21
	{ "_dk_name" },
#define	X_DK_UNIT	22
	{ "_dk_unit" },
#define X_FREEMEM	23
	{ "_freemem" },
	{ "" },
};

char	**dk_name;
int	*dk_unit;
size_t	pfree;
int	flag29;
char	**dr_name;
int	*dr_select;
int	dk_ndrive;
int	ndrives = 0;
char	*defdrives[] = { "sd0", 0 };
double	stat1();
int	hz;

#define	INTS(x)	((x) - (hz) < 0 ? 0 : (x) - (hz))

struct {
	int	busy;
	long	time[CPUSTATES];
	long	*xfer;

	struct	vmrate Rate;
	struct	vmtotal	Total;
	struct	vmsum Sum;

	struct	forkstat Forkstat;
	unsigned rectime;
	unsigned pgintime;
} s, s1, z;
#define	rate		s.Rate
#define	total		s.Total
#define	sum		s.Sum
#define	forkstat	s.Forkstat

struct	vmsum osum;
double	etime;
int 	mf;
time_t	now, boottime;
int	lines = 1;

void printhdr(sig)
        int sig;
{
	register int i, j;

	if (flag29)
	    printf(" procs       memory      swap      ");
	else
	    printf(" procs     memory              page           ");

	i = (ndrives * 3 - 6) / 2;
	if (i < 0)
		i = 0;
	for (j = 0; j < i; j++)
		putchar(' ');
	printf("faults");
	i = ndrives * 3 - 6 - i;
	for (j = 0; j < i; j++)
		putchar(' ');

	if (flag29) {
		printf("              cpu\n");
		printf(" r b w   avm  tx   fre   i  o   ");
	}
	else {
		printf("               cpu\n");
		printf(" r b w   avm   fre  ti tc  pi  po  fr  fc  ov ");
	}

	for (i = 0; i < dk_ndrive; i++)
		if (dr_select[i])
			printf("%c%c ", dr_name[i][0], dr_name[i][2]);
	if (flag29)
	    printf(" pd  in  sy  tr  ov  cs us ni sy id\n");
	else
            printf(" in  sy  cs us sy id\n");
	lines = 19;
}

main(argc, argv)
	int argc;
	char **argv;
{
	extern char *ctime();
	register i;
	int iter, iflag = 0;
	long nintv, t;
	char *arg, **cp, buf[BUFSIZ];

	knlist(nl);
	if(nl[0].n_type == 0) {
		fprintf(stderr, "no /vmunix namelist\n");
		exit(1);
	}
	mf = open("/dev/kmem", 0);
	if(mf < 0) {
		fprintf(stderr, "cannot open /dev/kmem\n");
		exit(1);
	}
	iter = 0;
	argc--, argv++;
	while (argc>0 && argv[0][0]=='-') {
		char *cp = *argv++;
		argc--;
		while (*++cp) switch (*cp) {

		case 't':
			dotimes();
			exit(0);

		case 'z':
			close(mf);
			mf = open("/dev/kmem", 2);
			lseek(mf, (long)nl[X_SUM].n_value, L_SET);
			write(mf, &z.Sum, sizeof z.Sum);
			exit(0);

		case 'f':
			doforkst();
			exit(0);

		case 's':
			dosum();
			exit(0);

		case 'i':
			iflag++;
			break;

		case 'p':
			flag29++;
			break;

		default:
			fprintf(stderr,
			    "usage: vmstat [ -fsi ] [ interval ] [ count]\n");
			exit(1);
		}
	}
	lseek(mf, (long)nl[X_BOOTTIME].n_value, L_SET);
	read(mf, &boottime, sizeof boottime);
	lseek(mf, (long)nl[X_HZ].n_value, L_SET);
	read(mf, &hz, sizeof hz);
	if (nl[X_DK_NDRIVE].n_value == 0) {
		fprintf(stderr, "dk_ndrive undefined in system\n");
		exit(1);
	}
	lseek(mf, (long)nl[X_DK_NDRIVE].n_value, L_SET);
	read(mf, &dk_ndrive, sizeof (dk_ndrive));
	if (dk_ndrive <= 0) {
		fprintf(stderr, "dk_ndrive %d\n", dk_ndrive);
		exit(1);
	}
	dr_select = (int *)calloc(dk_ndrive, sizeof (int));
	dr_name = (char **)calloc(dk_ndrive, sizeof (char *));
	dk_name = (char **)calloc(dk_ndrive, sizeof (char *));
	dk_unit = (int *)calloc(dk_ndrive, sizeof (int));
#define	allocate(e, t) \
    s./**/e = (t *)calloc(dk_ndrive, sizeof (t)); \
    s1./**/e = (t *)calloc(dk_ndrive, sizeof (t));
	allocate(xfer, long);
	for (arg = buf, i = 0; i < dk_ndrive; i++) {
		dr_name[i] = arg;
		sprintf(dr_name[i], "dk%d", i);
		arg += strlen(dr_name[i]) + 1;
	}
	read_names();
	time(&now);
	nintv = now - boottime;
	if (nintv <= 0 || nintv > 60L*60L*24L*365L*10L) {
		fprintf(stderr,
		    "Time makes no sense... namelist must be wrong.\n");
		exit(1);
	}
	if (iflag) {
		dointr(nintv);
		exit(0);
	}
	/*
	 * Choose drives to be displayed.  Priority
	 * goes to (in order) drives supplied as arguments,
	 * default drives.  If everything isn't filled
	 * in and there are drives not taken care of,
	 * display the first few that fit.
	 */
	ndrives = 0;
	while (argc > 0 && !isdigit(argv[0][0])) {
		for (i = 0; i < dk_ndrive; i++) {
			if (strcmp(dr_name[i], argv[0]))
				continue;
			dr_select[i] = 1;
			ndrives++;
		}
		argc--, argv++;
	}
	for (i = 0; i < dk_ndrive && ndrives < 4; i++) {
		if (dr_select[i])
			continue;
		for (cp = defdrives; *cp; cp++)
			if (strcmp(dr_name[i], *cp) == 0) {
				dr_select[i] = 1;
				ndrives++;
				break;
			}
	}
	for (i = 0; i < dk_ndrive && ndrives < 4; i++) {
		if (dr_select[i])
			continue;
		dr_select[i] = 1;
		ndrives++;
	}
	if (argc > 1)
		iter = atoi(argv[1]);
	signal(SIGCONT, printhdr);
loop:
	if (--lines == 0)
		printhdr();
	lseek(mf, (long)nl[X_CPTIME].n_value, L_SET);
 	read(mf, s.time, sizeof s.time);
	lseek(mf, (long)nl[X_DKXFER].n_value, L_SET);
	read(mf, s.xfer, dk_ndrive * sizeof (long));

	if (nintv != 1) {
		lseek(mf, (long)nl[X_SUM].n_value, L_SET);
		read(mf, &sum, sizeof(sum));
		rate.v_swtch = sum.v_swtch;
		rate.v_trap = sum.v_trap;
		rate.v_syscall = sum.v_syscall;
		rate.v_intr = sum.v_intr;
		rate.v_pdma = sum.v_pdma;
		rate.v_ovly = sum.v_ovly;
		rate.v_pgin = sum.v_pgin;
		rate.v_pgout = sum.v_pgout;
		rate.v_swpin = sum.v_swpin;
		rate.v_swpout = sum.v_swpout;
	}
	else {
		lseek(mf, (long)nl[X_RATE].n_value, L_SET);
		read(mf, &rate, sizeof rate);
		lseek(mf, (long)nl[X_SUM].n_value, L_SET);
		read(mf, &sum, sizeof sum);
	}
	lseek(mf, (long)nl[X_FREEMEM].n_value, L_SET);
	read(mf, &pfree, sizeof(pfree));

	lseek(mf, (long)nl[X_TOTAL].n_value, L_SET);
	read(mf, &total, sizeof total);
	osum = sum;
	lseek(mf, (long)nl[X_SUM].n_value, L_SET);
	read(mf, &sum, sizeof sum);
	etime = 0;
	for (i=0; i < dk_ndrive; i++) {
		t = s.xfer[i];
		s.xfer[i] -= s1.xfer[i];
		s1.xfer[i] = t;
	}
	for (i=0; i < CPUSTATES; i++) {
		t = s.time[i];
		s.time[i] -= s1.time[i];
		s1.time[i] = t;
		etime += s.time[i];
	}
	if(etime == 0.)
		etime = 1.;

	printf("%2d%2d%2d", total.t_rq, total.t_dw, total.t_sw);
	/*
	 * We don't use total.t_free because it slops around too much
	 * within this kernel
	 */
	printf("%6D", total.t_avm);
	if (flag29)
		printf("%4D",
		    total.t_avm ? (total.t_avmtxt * 100) / total.t_avm : 0);
	printf("%6d", pfree);

	if (flag29)
		printf("%4D%3D  ", rate.v_swpin / nintv, rate.v_swpout / nintv);
	else {
                // TODO
		printf("%4D", rate.v_ovly / nintv);
	}

	etime /= (float)hz;
	for (i = 0; i < dk_ndrive; i++)
		if (dr_select[i])
			stats(i);
	if (flag29)
		printf("%4D%4D%4D%4D%4D%4D",
		    rate.v_pdma / nintv, INTS(rate.v_intr / nintv),
		    rate.v_syscall / nintv, rate.v_trap / nintv,
		    rate.v_ovly / nintv, rate.v_swtch / nintv);
	else
                printf("%4D%4D%4D", INTS(rate.v_intr/nintv), rate.v_syscall/nintv,
                    rate.v_swtch/nintv);
	for(i=0; i<CPUSTATES; i++) {
		float f = stat1(i);
		if (!flag29 && i == 0) {   	/* US+NI */
			i++;
			f += stat1(i);
		}
		printf("%3.0f", f);
	}
	printf("\n");
	fflush(stdout);
	nintv = 1;
	if (--iter &&argc > 0) {
		sleep(atoi(argv[0]));
		goto loop;
	}
}

dotimes()
{
	printf("page in/out/reclamation is not applicable to 2.11BSD\n");
}

dosum()
{
	struct nchstats nchstats;
	long nchtotal;

	lseek(mf, (long)nl[X_SUM].n_value, L_SET);
	read(mf, &sum, sizeof sum);
	printf("%9D swap ins\n", sum.v_swpin);
	printf("%9D swap outs\n", sum.v_swpout);
	printf("%9D pages swapped in\n", sum.v_pswpin);
	printf("%9D pages swapped out\n", sum.v_pswpout);
	printf("%9D page ins\n", sum.v_pgin);
	printf("%9D page outs\n", sum.v_pgout);
	printf("%9D cpu context switches\n", sum.v_swtch);
	printf("%9D device interrupts\n", sum.v_intr);
	printf("%9D software interrupts\n", sum.v_soft);
	printf("%9D pseudo-dma dz interrupts\n", sum.v_pdma);
	printf("%9D traps\n", sum.v_trap);
	printf("%9D overlay emts\n", sum.v_ovly);
	printf("%9D system calls\n", sum.v_syscall);
#define	nz(x)	((x) ? (x) : 1)
	lseek(mf, (long)nl[X_NCHSTATS].n_value, 0);
	read(mf, &nchstats, sizeof nchstats);
	nchtotal = nchstats.ncs_goodhits + nchstats.ncs_badhits +
	    nchstats.ncs_falsehits + nchstats.ncs_miss + nchstats.ncs_long;
	printf("%9D total name lookups", nchtotal);
	printf(" (cache hits %D%% system %D%% per-process)\n",
	    nchstats.ncs_goodhits * 100 / nz(nchtotal),
	    nchstats.ncs_pass2 * 100 / nz(nchtotal));
	printf("%9s badhits %D, falsehits %D, toolong %D\n", "",
	    nchstats.ncs_badhits, nchstats.ncs_falsehits, nchstats.ncs_long);
}

char Pages[] = "pages";

doforkst()
{
	lseek(mf, (long)nl[X_FORKSTAT].n_value, L_SET);
	read(mf, &forkstat, sizeof forkstat);
	printf("%D forks, %D %s, average=%.2f\n",
		forkstat.cntfork, forkstat.sizfork, Pages,
		(float) forkstat.sizfork / forkstat.cntfork);
	printf("%D vforks, %D %s, average=%.2f\n",
		forkstat.cntvfork, forkstat.sizvfork, Pages,
		(float)forkstat.sizvfork / forkstat.cntvfork);
}

stats(dn)
{
	if (dn >= dk_ndrive) {
		printf("  0");
		return;
	}
	printf("%3.0f", s.xfer[dn]/etime);
}

double
stat1(row)
{
	double t;
	register i;

	t = 0;
	for(i=0; i<CPUSTATES; i++)
		t += s.time[i];
	if(t == 0.)
		t = 1.;
	return(s.time[row]*100./t);
}

dointr(nintv)
	long nintv;
{
	printf("Device interrupt statistics are not applicable to 2.11BSD\n");
}

#define steal(where, var) \
	lseek(mf, where, L_SET); read(mf, &var, sizeof var);
/*
 * Read the drive names out of kmem.
 */
read_names()
{
	char two_char[2];
	register int i;

	lseek(mf, (long)nl[X_DK_NAME].n_value, L_SET);
	read(mf, dk_name, dk_ndrive * sizeof (char *));
	lseek(mf, (long)nl[X_DK_UNIT].n_value, L_SET);
	read(mf, dk_unit, dk_ndrive * sizeof (int));

	for(i = 0; dk_name[i]; i++) {
		lseek(mf, (long)dk_name[i], L_SET);
		read(mf, two_char, sizeof two_char);
		sprintf(dr_name[i], "%c%c%d", two_char[0], two_char[1],
		    dk_unit[i]);
	}
}
