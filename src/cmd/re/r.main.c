/*
 * Main program: enter/exit, open windows, parse options.
 *
 * Usage:
 *      re file [line-number]   - Open file
 * or
 *      re                      - Open last edited file
 * or
 *      re -                    - Replay last session after crash
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"
#include <signal.h>
#include <sys/stat.h>

int tabstops[NTABS] = {
    -1, 0,  8,  16, 24,  32,  40,  48,  56,  64,
    72, 80, 88, 96, 104, 112, 120, 128, 136, BIGTAB,
    0,  0,  0,  0,  0,   0,   0,   0,   0,   0,
};

int lread1 = -1;         /* -1 означает, что символ использован */

/* Параметры для движения по файлу */

int defplline = 10;                 /* +LINE       */
int defmiline = 10;                 /* -LINE       */
int defplpage =  1;                 /* +PAGE      */
int defmipage =  1;                 /* -PAGE      */
int deflport  = 30;                 /* LEFT PORT  */
int defrport  = 30;                 /* RIGHT PORT */
int definsert  = 1;                 /* OPEN       */
int defdelete  = 1;                 /* CLOSE      */
int defpick    = 1;                 /* PICK       */
char deffile[] = "/share/re.help";  /* Help file */

/* Инициализации  */
int cline_max  = 0;
int cline_incr = 20; /* Инкремент для расширения */
int clineno = -1;
char cline_modified = 0;

/* Файл / протокол */
int ttyfile  = -1;
int inputfile = 0;

int oldttmode;

char *ttynm, *ttytmp, *rfile;
clipboard_t pb, db;

/*
 * Фатальный сигнал
 */
void sig(n)
    int n;
{
    fatal("Fatal signal");
}

/*
 * Установить игнорирование
 */
void igsig(n)
    int n;
{
    void testsig(int);

    signal(3,igsig);
    signal(2,testsig);
}

/*
 * проверить, не было ли прерывания
 */
void testsig(n)
    int n;
{
    signal(2, igsig);
    if (intrflag)
        fatal("RE WAS INTERRUPTED\n");
    igsig();
    intrflag = 1;
}

/*
 * Инициализация режимов и файлов
 * 2 - повторить сеанс из ttyfile
 * 1 - начать заново
 */
static void startup(restart)
    int restart;
{
    register int i;
    register char *c, *name;

    if (nice(-10) < 0)
        /* ignore errors */;
    userid = getuid();
    groupid = getgid();
    setuid(userid);
    setgid(groupid);
    for (i=NCOLS; i; )
        blanks[--i] = ' ';
    name = getenv("USER");
    if (! name)
        name = getnm(userid);
    ttynm = ttyname(0);
    if (! ttynm)
        ttynm = "nottyno\0\0";
    c = ttynm + strlen (ttynm) - 2;
    if (*c == '/')
        *c = '-';
    tmpname = append(append("/tmp/retm", c), name);
    ttytmp  = append(append("/tmp/rett", c), name);
    rfile   = append(append("/tmp/resv", c), name);
    if (restart) {
        ttyfile = open(ttytmp, 2);
        if (ttyfile >= 0 && restart != 2)
            close(ttyfile);
    }
    if (restart != 2) {
        unlink(ttytmp);
        ttyfile = creat(ttytmp, FILEMODE);
    } else
        inputfile = ttyfile;
    if (ttyfile < 0) {
        puts1("can't open ttyfile.\n");
        exit(1);
    }
    unlink(tmpname);
    i = creat(tmpname, 0600);
    if (i < 0) {
        puts1("Can't open temporary file.\n");
        exit(1);
    }
    /*  Рабочий файл нужен на read/write - приходится переоткрыть */
    close(i);
    i = open(tmpname, 2);
    file[i].name = tmpname;
    file[i].writable = 1;
    tempfile = i;
    /* файл '#' -- запоминание убранного и отмеченного  */
    file[2].name = "#";
    file[2].nlines = 0;
    pickwksp = (workspace_t*) salloc(sizeof(workspace_t));
    pickwksp->cursegm = file[2].chain = (segment_t*) salloc(sizeof_segment_t);
    pickwksp->wfile = 2;
    pickbuf = &pb;
    deletebuf = &db;

    /* Устанавливаем режимы терминала */
    ttstartup();

    /* Устанавливаем описатель всего экрана */
    setupviewport(&wholescreen, 0, NCOLS-1, 0, NLINES-1, 0);

    /* Устанавливаем описатель окна параметров */
    setupviewport(&paramport, 0, NCOLS-1, NLINES-NPARAMLINES, NLINES-1, 0);
    paramport.rtext--;
    paramport.redit = PARAMREDIT;

    /* Закрываем терминал на прием сообщений от других */
    oldttmode = getpriv(0);
    chmod(ttynm,0600);

    /*
     * Переопределяем сигналы
     */
    for (i=SIGTERM; i; i--)
        signal(i,sig);
    signal(SIGINT,testsig);
    signal(SIGQUIT,igsig);
    curport = &wholescreen;
    putcha(COSTART);
    putcha(COHO);
}

/*
 * Create initial window state.
 */
static void makestate()
{
    register viewport_t *port;

    nportlist = 1;
    port = portlist[0] = (viewport_t*) salloc(sizeof(viewport_t));
    setupviewport(portlist[0], 0, NCOLS-1, 0, NLINES-NPARAMLINES-1, 1);
    drawport(port, 0);
    poscursor(0, 0);
}

/*
 * Make or restore window state.
 * '!' - restore, ' ' - make new.
 */
static void getstate(ichar)
    char ichar;
{
    int nletters, lmarg, rmarg, tmarg, bmarg, row, col, portnum, gf;
    register int i, n;
    register char *f1;
    char *fname;
    int gbuf;
    viewport_t *port;

    if (ichar != '!' || (gbuf = open(rfile, 0)) <= 0 ||
        (nportlist = get1w(gbuf)) == -1) {
make:   makestate();
        ichar = ' ';
        return;
    }
    portnum = get1w(gbuf);
    for (n=0; n<nportlist; n++) {
        port = portlist[n] = (viewport_t*) salloc(sizeof(viewport_t));
        port->prevport = get1w(gbuf);
        lmarg = get1w(gbuf);
        rmarg = get1w(gbuf);
        tmarg = get1w(gbuf);
        bmarg = get1w(gbuf);
        setupviewport(port, lmarg, rmarg, tmarg, bmarg, 1);
        drawport(port, 0);
        gf = 0;
        nletters = get1w(gbuf);
        if (nletters != 0) {
            f1 = fname = salloc(nletters);
            do {
                *f1 = get1c(gbuf);
            } while (*f1++);
            row = get1w(gbuf);
            col = get1w(gbuf);
            if (editfile(fname, row, col, 0, 0) == 1)
                gf = 1;
            curwksp->ccol = get1w(gbuf);
            curwksp->crow = get1w(gbuf);
            poscursor(curwksp->ccol, curwksp->crow);
        }
        nletters = get1w(gbuf);
        if (nletters < 0)
            goto make;
        f1 = fname = salloc(nletters);
        do {
            *f1 = get1c(gbuf);
        } while (*f1++);
        row = get1w(gbuf);
        col = get1w(gbuf);
        if (editfile(fname, row, col, 0, (n != portnum)) == 1)
            gf = 1;
        curwksp->ccol = get1w(gbuf);
        curwksp->crow = get1w(gbuf);
        if (gf == 0) {
            if (editfile(deffile, 0, 0, 0, (n != portnum)) <= 0)
                error("Default file gone: notify sys admin.");
            curwksp->ccol = curwksp->crow = 0;
        }
        poscursor(curwksp->ccol, curwksp->crow);
    }
    switchport(portlist[portnum]);
    poscursor(curwksp->ccol, curwksp->crow);
    if (nportlist > 1)
        for (i=0; i<nportlist; i++)
            chgport(-1);
    close(gbuf);
}

/*
 * Write a command with arguments to protocol file.
 */
static void writefile(code1, str, code2)
    int code1, code2;
    char *str;
{
    int len;

    if (write(ttyfile, &code1, 1) != 1)
        /* ignore errors */;
    len = strlen (str);
    if (len > 0 && write(ttyfile, str, len) != len)
        /* ignore errors */;
    if (write(ttyfile, &code2, 1) != 1)
        /* ignore errors */;
}

/*
 * Write state of windows, to restore session later.
 */
static void savestate()
{
    int i, nletters;
    register int portnum;
    register char *f1;
    char *fname;
    register viewport_t *port;
    int sbuf;

    curwksp->ccol = cursorcol;
    curwksp->crow = cursorline;
    unlink(rfile);
    sbuf = creat(rfile, FILEMODE);
    if (sbuf <= 0)
        return;
    put1w(nportlist, sbuf);
    for (portnum=0; portnum < nportlist; portnum++)
        if (portlist[portnum] == curport)
            break;
    put1w(portnum, sbuf);
    for (i=0; i<nportlist; i++) {
        port = portlist[i];
        put1w(port->prevport, sbuf);
        put1w(port->lmarg, sbuf);
        put1w(port->rmarg, sbuf);
        put1w(port->tmarg, sbuf);
        put1w(port->bmarg, sbuf);
        f1 = fname = file[port->altwksp->wfile].name;
        if (f1) {
            while (*f1++);
            nletters = f1 - fname;
            put1w(nletters, sbuf);
            f1 = fname;
            do {
                put1c(*f1, sbuf);
            } while (*f1++);
            put1w(port->altwksp->ulhclno, sbuf);
            put1w(port->altwksp->ulhccno, sbuf);
            put1w(port->altwksp->ccol, sbuf);
            put1w(port->altwksp->crow, sbuf);
        } else
            put1w(0, sbuf);

        f1 = fname = file[port->wksp->wfile].name;
        if (f1) {
            while (*f1++);
            nletters = f1 - fname;
            put1w(nletters, sbuf);
            f1 = fname;
            do {
                put1c(*f1, sbuf);
            } while (*f1++);
            put1w(port->wksp->ulhclno, sbuf);
            put1w(port->wksp->ulhccno, sbuf);
            put1w(port->wksp->ccol, sbuf);
            put1w(port->wksp->crow, sbuf);
        }
    }
    close(sbuf);
}

/*
 * main(nargs,args)
 * Головная программа
 */
int main(nargs, args)
    int nargs;
    char *args[];
{
    int restart, i;
    char *cp, ichar;

    tcread();

    /* режим работы 0 - норм, 1 - повторный, 2 - из файла /tmp/tt.. */
    restart = 0;
    if (nargs == 1) {
        restart = 1;
        ichar = '!';

    } else if (*(cp = args[1]) == '-') {
        if (*++cp) {
            /* Указан файл протокола */
            if ((inputfile = open( cp,0)) < 0) {
                puts1("Can't open command file.\n");
                return 1;
            }
        } else
            restart = 2;
        nargs = 1;
    } else
        ichar = ' ';

    startup(restart);
    if (inputfile) {
        if (read(inputfile, &ichar, 1) <= 0) {
            cleanup();
            puts1("Command file is empty.\n");
            return 1;
        }
    } else {
        /* For repeated session */
        if (write(ttyfile, &ichar, 1) != 1)
            /* ignore errors */;
    }
    getstate(ichar);
    if (nargs > 1 && *args[1] != '\0') {
        i = defplline + 1;
        if ((nargs > 2) && (s2i(args[2],&i) || i <= defplline+1))
            i = defplline+1;
        poscursor(curwksp->ccol, curwksp->crow);
        writefile(CCENTER, args[1], CCSETFILE);
        if (editfile(args[1], i - defplline - 1, 0, 1, 1) <= 0) {
            /* Failed to open file - use empty buffer. */
            putup(0, curport->btext);
            poscursor(curwksp->ccol, curwksp->crow);
        } else {
            /* File opened. */
            if (nargs > 2 && i > 1)
                writefile(CCENTER, args[2], CCGOTO);
        }
    } else {
        /* Saved session restored. */
        putup(0, curport->btext);
        poscursor(curwksp->ccol, curwksp->crow);
    }
    telluser("", 0);
    mainloop();
    putcha(COFIN);
    dumpcbuf();
    printf("\n");
    cleanup();
    savestate();    /* Записать выходное состояние */
    dumpcbuf();
    return 0;
}

/*
 * Cleanup before exit.
 */
void cleanup()
{
    /* restore tty mode and exit */
    ttcleanup();
    close(tempfile);
    unlink(tmpname);
    close(ttyfile);
    chmod(ttynm, 07777 & oldttmode);
}

/*
 * Print error message and exit.
 */
void fatal(s)
    char *s;
{
    putcha(COFIN);
    putcha(COBELL);
    putcha(COBELL);
    dumpcbuf();
    ttcleanup();
    puts1("\nFirst the bad news: editor just ");
    if (s) {
        puts1("died:\n");
        puts1(s);
    } else
        puts1("ran out of space.\n");
    puts1("\nNow the good news - your editing session can be reproduced\n");
    puts1("from file ");
    puts1(ttytmp);
    puts1("\nUse command 're -' to do it.\n");
#ifdef DEBUG
    if (inputfile || ! isatty(1)) {
        register int i;
        register workspace_t *w;

        if (s)
            printf("%s\n\n", s);
        for (i = 0; i < MAXFILES; i++)
            if (file[i].chain) {
                printf("\n*** File[%d] = %s\n", i, file[i].name);
                printsegm(file[i].chain);
            }
        for (i = 0; i < nportlist; i++) {
            w = portlist[i]->wksp;
            printf("\nViewport #%d: chain %d, current line %d at block %p,\n",
                i, w->wfile, w->curlno, w->cursegm);
            printf(" first line %d, ulhc (%d,%d)\n",
                w->curflno, w->ulhccno, w->ulhclno);
        }
        for (i=12; i; i--)
            signal(i, 0);
    }
#endif
    close(ttyfile);
    exit(1);
}
