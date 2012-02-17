#include "defs.h"
#include <sys/file.h>

char    *lp;
BKPTR   bkpthead;
char    lastc;
u_int   *uar0;
int     fcor;
int     fsym;
const char *errflg;
int     signo;
long    dot;
char    *symfil;
int     wtflag;
int     pid;
long    expv;
int     adrflg;
long    loopcnt;
int     userpc=1;

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
    int rc;
    register BKPTR bkpt;

    if (adrflg)
        userpc = shorten(dot);
    print("%s: running\n", symfil);

    while (loopcnt-- > 0) {
#ifdef DEBUG
        print("\ncontinue %d %d\n", userpc, execsig);
#endif
        stty(0, &usrtty);
        ptrace (runmode, pid, (void*) userpc, execsig);
        bpwait();
        chkerr();
        readregs();

        /* look for bkpt */
        if (signo==0 && (bkpt=scanbkpt(uar0[FRAME_PC]-2))) {
            /* stopped at bkpt */
            userpc = uar0[FRAME_PC] = bkpt->loc;
            if (bkpt->flag == BKPTEXEC ||
                ((bkpt->flag = BKPTEXEC, command(bkpt->comm, ':')) &&
                --bkpt->count))
            {
                execbkpt(bkpt);
                execsig = 0;
                loopcnt++;
                userpc = 1;
            } else {
                bkpt->count = bkpt->initcnt;
                rc = 1;
             }
        } else {
            rc = 0;
            execsig = signo;
            userpc = 1;
        }
    }
    return(rc);
}

endpcs()
{
    register BKPTR  bkptr;

    if (pid) {
        ptrace(PT_KILL, pid, 0, 0);
        pid = 0;
        userpc = 1;
        for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt) {
            if (bkptr->flag) {
                bkptr->flag = BKPTSET;
            }
        }
    }
}

setup()
{
    close(fsym);
    fsym = -1;
    pid = fork();
    if (pid == 0) {
        ptrace(PT_TRACE_ME, 0, 0, 0);
        signal(SIGINT, sigint);
        signal(SIGQUIT, sigqit);
        doexec();
        exit(0);
    } else if (pid == -1) {
        error(NOFORK);
    } else {
        bpwait();
        readregs();
        lp[0] = EOR;
        lp[1] = 0;
        fsym = open(symfil, wtflag);
        if (errflg) {
            print("%s: cannot execute\n", symfil);
            endpcs();
            error((char *)0);
        }
    }
}

execbkpt(bkptr)
    BKPTR bkptr;
{
    int bkptloc;
#ifdef DEBUG
    print("exbkpt: %d\n", bkptr->count);
#endif
    bkptloc = bkptr->loc;
    ptrace(PT_WRITE_I, pid, (void*) bkptloc, bkptr->ins);
    stty(0, &usrtty);
    ptrace(PT_STEP, pid, (void*) bkptloc, 0);
    bpwait();
    chkerr();
    ptrace(PT_WRITE_I, pid, (void*) bkptloc, BPT);
    bkptr->flag = BKPTSET;
}

doexec()
{
    char *argl[MAXARG];
    char args[LINSIZ];
    char *p, **ap, *filnam;

    ap = argl;
    p = args;
    *ap++ = symfil;
    do {
        if (rdc() == EOR)
            break;
        /*
         * If we find an argument beginning with a `<' or a `>', open
         * the following file name for input and output, respectively
         * and back the argument collocation pointer, p, back up.
         */
        *ap = p;
        while (lastc != EOR && lastc != SP && lastc != TB) {
            *p++ = lastc;
            readchar();
        }
        *p++ = 0;
        filnam = *ap + 1;
        if (**ap == '<') {
            close(0);
            if (open(filnam, 0) < 0) {
                print("%s: cannot open\n", filnam);
                exit(0);
            }
            p = *ap;
        } else if (**ap == '>') {
            close(1);
            if (open(filnam, O_CREAT | O_WRONLY, 0666) < 0) {
                print("%s: cannot create\n", filnam);
                exit(0);
            }
            p = *ap;
        } else {
            ap++;
        }
    } while (lastc != EOR);
    *ap++ = 0;
    execv(symfil, argl);
}

BKPTR
scanbkpt(adr)
{
    register BKPTR bkptr;

    for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt) {
        if (bkptr->flag && bkptr->loc == adr)
            break;
    }
    return bkptr;
}

delbp()
{
    register BKPTR bkptr;

    for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt) {
        if (bkptr->flag)
            del1bp(bkptr);
    }
}

del1bp(bkptr)
    BKPTR bkptr;
{
    ptrace(PT_WRITE_I, pid, (void*) bkptr->loc, bkptr->ins);
}

setbp()
{
    register BKPTR bkptr;

    for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt) {
        if (bkptr->flag)
            set1bp(bkptr);
    }
}

set1bp(bkptr)
    BKPTR bkptr;
{
    register int a;

    a = bkptr->loc;
    bkptr->ins = ptrace(PT_READ_I, pid, (void*) a, 0);
    ptrace(PT_WRITE_I, pid, (void*) a, BPT);
    if (errno) {
        print("cannot set breakpoint: ");
        psymoff(leng(bkptr->loc), ISYM, "\n");
    }
}

bpwait()
{
    register int w;
    int stat;

    signal(SIGINT, SIG_IGN);
    while ((w = wait(&stat)) != pid && w != -1)
        ;
    signal(SIGINT, sigint);
    gtty(0, &usrtty);
    stty(0, &adbtty);
    if (w == -1) {
        pid = 0;
        errflg = BADWAIT;

    } else if ((stat & 0177) != 0177) {
        signo = stat & 0177;
        if (signo) {
            sigprint();
        }
        if (stat & 0200) {
            print(" - core dumped");
            close(fcor);
            setcor();
        }
        pid = 0;
        errflg = ENDPCS;

    } else {
        signo = stat>>8;
        if (signo != SIGTRAP) {
            sigprint();
        } else {
            signo = 0;
        }
        flushbuf();
    }
}

/*
 * get REG values from pcs
 */
readregs()
{
    register u_int i, *afp;

    for (i=0; i<NREG; i++) {
        uar0[reglist[i].roffs] = ptrace(PT_READ_U, pid,
            (int *)((int)&uar0[reglist[i].roffs] - (int)&corhdr),
            0);
    }
}
