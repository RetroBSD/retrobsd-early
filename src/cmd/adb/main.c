#include "defs.h"

char    *myname;        /* program name */
int     argcount;
int     mkfault;
int     executing;
int     infile;
char    *lp;
int     maxoff;
int     maxpos;
int     wtflag;
int     kernel;
long    maxfile;
long    maxstor;
long    txtsiz;
long    datsiz;
long    datbas;
long    stksiz;
const char *errflg;
int     exitflg;
int     magic;
long    entrypt;
char    lastc;
int     eof;
int     lastcom;
long    var[36];
char    *symfil;
char    *corfil;
char    *printptr;
char    *Ipath = "/share/adb";

long
roundn(a, b)
    long a, b;
{
    long w;

    w = ((a + b - 1) / b) * b;
    return w;
}

/*
 * error handling
 */
void
chkerr()
{
    if (errflg || mkfault)
        error(errflg);
}

void
error(msg)
    const char *msg;
{
    errflg = msg;
    iclose(0, 1);
    oclose();
    longjmp(erradb, 1);
}

void
fault(a)
{
    signal(a, fault);
    lseek(infile, 0L, 2);
    mkfault++;
}

/*
 * set up files and initial address mappings
 */
int
main(argc, argv)
    register char   **argv;
    int     argc;
{
    short   mynamelen;              /* length of program name */

    maxfile = 1L << 24;
    maxstor = 1L << 16;
    if (isatty(0))
        myname = *argv;
    else
        myname = "adb";
    mynamelen = strlen(myname);

    gtty(0, &adbtty);
    gtty(0, &usrtty);
    while (argc > 1) {
        if (! strcmp("-w", argv[1])) {
            wtflag = 2;
            argc--;
            argv++;
            continue;
        }
        if (! strcmp("-k", argv[1])) {
            kernel++;
            argc--;
            argv++;
            continue;
        }
        if (! strcmp("-I", argv[1])) {
            Ipath = argv[2];
            argc -= 2;
            argv += 2;
            continue;
        }
        break;
    }

    if (argc > 1) {
        symfil = argv[1];
    }
    if (argc > 2) {
        corfil = argv[2];
    }
    argcount = argc;
    setsym();
    setcor();

    /* set up variables for user */
    maxoff = MAXOFF;
    maxpos = MAXPOS;
    var[VARB] = datbas;
    var[VARD] = datsiz;
    var[VARE] = entrypt;
    var[VARM] = magic;
    var[VARS] = stksiz;
    var[VART] = txtsiz;

    sigint = signal(SIGINT, SIG_IGN);
    if (sigint != SIG_IGN) {
        sigint = fault;
        signal(SIGINT, fault);
    }
    sigqit = signal(SIGQUIT, SIG_IGN);
    siginterrupt(SIGINT, 1);
    siginterrupt(SIGQUIT, 1);
    setjmp(erradb);
    if (executing) {
        delbp();
    }
    executing = FALSE;

    for (;;) {
        flushbuf();
        if (errflg) {
            print("%s\n", errflg);
            exitflg = (int) errflg;
            errflg = 0;
        }
        if (mkfault) {
            mkfault = 0;
            printc(EOR);
            print("%s\n", myname);
        }
        if (myname && ! infile) {
            write(1, myname, mynamelen);
            write(1, "> ", 2);
        }
        lp = 0;
        rdc();
        lp--;
        if (eof) {
            if (infile) {
                iclose(-1, 0);
                eof = 0;
                longjmp(erradb, 1);
            }
            done();
        } else {
            exitflg = 0;
        }
        command(0, lastcom);
        if (lp && lastc != EOR) {
            error(NOEOR);
        }
    }
}

done()
{
    endpcs();
    exit(exitflg);
}
