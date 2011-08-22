/*
 * Interface to display - physical level.
 * Parsing escape sequences.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"
#include <sgtty.h>

#ifdef TERMIOS
#include <termios.h>

#ifndef NCC
#   define NCC NCCS
#endif
static struct termios tioparam;

#else /* TERMIOS */

static struct sgttyb templ;
static struct tchars tchars0;

#endif /* TERMIOS */

/*
 * Работа с буфером символов
 */
#define NPUTCBUF 256   /* Размер буфера вывода */

static char putcbuf[NPUTCBUF];
static int iputcbuf = 0;

static char *sy0;
static int isy0f = -1;
static int knockdown = 0;

/*
 * Setup terminal modes.
 */
void ttstartup()
{
#ifdef TERMIOS
    {
    struct termios param;
    register int i;

    tcgetattr(0, &tioparam);
    param = tioparam;

    for (i=0; i<NCC; i++)
        param.c_cc[i] = 0377;

    if (param.c_cc[VINTR] == 0177)
        param.c_cc[VINTR] = 3;
    param.c_cc[VMIN ] = 1;
    param.c_cc[VTIME] = 2;

    /* input modes */
    param.c_iflag &= ~ICRNL;

    /* output modes */
    param.c_oflag &= ~OPOST;

    /* control modes */
    param.c_cflag |= CREAD;

    /* local modes */
    param.c_lflag = ECHOK | ISIG;

    tcsetattr(0, TCSADRAIN, &param);
    }
#else /* TERMIOS */
#ifdef TIOCGETC
    struct tchars tcharsw;

    ioctl(0, TIOCGETC, &tchars0);
    tcharsw = tchars0;
    tcharsw.t_eofc = -1;        /* end-of-file */
    tcharsw.t_quitc = -1;       /* quit */
    tcharsw.t_intrc = -1;	/* interrupt */
    ioctl(0, TIOCSETC, &tcharsw);
#endif
#ifdef TIOCGETP
    {
    struct sgttyb templw;

    ioctl(0, TIOCGETP, &templ);
    templw = templ;

    templw.sg_flags &= ~(ECHO | CRMOD | XTABS | RAW);
    templw.sg_flags |= CBREAK;

    ioctl(0, TIOCSETP, &templw);
    }
#endif
#endif /* TERMIOS */
}

/*
 * Restore terminal modes.
 */
void ttcleanup()
{
#ifdef TERMIOS
    tcsetattr(0, TCSADRAIN, &tioparam);
#else /* TERMIOS */
    ioctl(0, TIOCSETP, &templ);
    ioctl(0, TIOCSETC, &tchars0);
#endif /* TERMIOS */
}

/*
 * Вывод символа в безусловном режиме
 */
static void putchb(c)
    int c;
{
    putcbuf[iputcbuf++] = c & 0177;
    if (iputcbuf == NPUTCBUF)
        dumpcbuf();
}

/*
 * Move screen cursor to given coordinates.
 */
void pcursor(col, lin)
    int col, lin;
{
    register char *c, sy;

    c = tgoto (curspos, col, lin);
    while ((sy = *c++)) {
        putchb(sy);
    }
}

/*
 * putcha(c) - выдать символ "c".
 * "c" может быть кодом управления.
 * Возвращается 0, если запрошенная операция невозможна
 */
int putcha(c)
    int c;
{
    register int cr;
    register char *s;

    cr = c & 0177;
    if (cr >= 0 && cr <= COMCOD) {
        s = cvtout[cr];
        if (! s)
            return(0);
        while ((cr = *s++) != 0)
            putchb(cr);
    } else {
        putcbuf[iputcbuf++] = c;
        if (iputcbuf == NPUTCBUF)
            dumpcbuf();
    }
    return(1);
}

/*
 * Output a line of spaces.
 */
void putblanks(k)
    register int k;
{
    cursorcol += k;
    while (k--) {
        putcbuf[iputcbuf++] = ' ';
        if (iputcbuf == NPUTCBUF)
            dumpcbuf();
    }
    dumpcbuf();
}

/*
 * Flush output buffer.
 */
void dumpcbuf()
{
    if (iputcbuf == 0)
        return;
    if (write(2, putcbuf, iputcbuf) != iputcbuf)
        /* ignore errors */;
    iputcbuf = 0;
}

/*
 * Поиск кода клавиши.
 * struct ex_int * (fb , fe) = NULL при подаче
 * первого символа с данной клавиши.
 * Дальше они используются при поиске кода.
 * Коды ответа:
 * CONTF - дай следующий символ,
 * BADF  - такой последовательности нет в таблице,
 * >=0   - код команды.
 */
static int findt (fb, fe, sy, ns)
    struct ex_int **fb, **fe;
    char sy;
    int ns;
{
    char sy1;
    register struct ex_int *fi;

    fi = *fb ? *fb : inctab;
    *fb = 0;
    if (sy == 0)
        return BADF;
    for (; fi != *fe; fi++) {
        if (! *fe && ! fi->excc)
            goto exit;
        sy1 = fi->excc[ns];
        if (*fb) {
            if (sy != sy1)
                goto exit;
        } else {
            if (sy == sy1) {
                *fb = fi;
            }
        }
    }
exit:
    *fe = fi; /* for "addkey" */
    if (! *fb)
        return BADF;
    fi = *fb;
    if (fi->excc[ns + 1])
        return CONTF;
    return fi->incc;
}

/*
 * Получение символа из файла протокола.
 * возвращается 0, если файл кончился
 */
static int readfc()
{
    char sy1 = CCQUIT;
    do {
        lread1 = isy0f;
        if (intrflag || read(inputfile, &sy1, 1) !=1) {
            if (inputfile != ttyfile)
                close(inputfile);
            else
                lseek(ttyfile, (long) -1, 1);
            inputfile = 0;
            intrflag = 0;
            putcha(COBELL);
            dumpcbuf();
            return 0;
        }
        isy0f = (unsigned char) sy1;
    } while (lread1 < 0);
    return 1;
}

/*
 * read1()
 * Чтание очередного символа с терминала.
 * Кроме того, здесь же разворачиваются макро и
 * происходит чтение из файла протокола.   Последние две
 * функции будут убраны повыше.
 * Ответ приходит в lread1.
 */
int read1()
{
    char sy1;

    dumpcbuf();

    /* Если остался неиспользованным символ */
    if (lread1 != -1)
        goto retnl;

    /* Если еще не кончилось макро-расширение */
rmac:
    if (symac) {
        lread1 = (unsigned char) *symac++;
        if (*symac == 0)
            symac = 0;
        goto retnl;
    }
    /* Если идет чтение из файла */
    if (inputfile != 0 && readfc())
        goto retnm;
    /*
     * Ниже - то, что относится к терминалу
     * =====================================
     */
    /* Было преобразование и символы не кончились */
    if (sy0 != 0) {
        lread1 = (unsigned char) *sy0++;
        if (*sy0 == 0)
            sy0 = 0;
        goto retn;
    }
    /* Чтение с клавиатуры */
new:
    intrflag = 0;
    if (read(inputfile, &sy1, 1) != 1)
        goto readquit;

    /* Если символ не управляющий */
    lread1 = (unsigned char) sy1;
    if (lread1 == 0177) {
        lread1 = CCBACKSPACE;
        goto readychr;
    }
    if (lread1 > 037)
        goto readychr;
    if (knockdown) {
        lread1 += 0100;
        goto readychr;
    }
    /* Если символ - ^X? */
    if (lread1 == ('X' & 037)) {
        if (read(inputfile, &sy1, 1) != 1)
            goto readquit;
        switch (sy1) {
        case 'C' & 037:     /* ^X ^C */
            lread1 = CCQUIT;
            goto readychr;
        case 'n':           /* ^X n */
        case 'N':           /* ^X N */
            lread1 = CCPLLINE;
            goto readychr;
        case 'p':           /* ^X p */
        case 'P':           /* ^X P */
            lread1 = CCMILINE;
            goto readychr;
        case 'f':           /* ^X f */
        case 'F':           /* ^X F */
            lread1 = CCRPORT;
            goto readychr;
        case 'b':           /* ^X b */
        case 'B':           /* ^X B */
            lread1 = CCLPORT;
            goto readychr;
        case 'h':           /* ^X h */
        case 'H':           /* ^X H */
            lread1 = CCHOME;
            goto readychr;
        case 't':           /* ^X t */
        case 'T':           /* ^X T */
            lread1 = CCBACKTAB;
            goto readychr;
        case 'i':           /* ^X i */
        case 'I':           /* ^X I */
            lread1 = CCINSMODE;
            goto readychr;
        case 'x':           /* ^X x */
        case 'X':           /* ^X X */
            lread1 = CCDOCMD;
            goto readychr;
        case '$':           /* ^X $ */
rmacname:
            if (read(inputfile, &sy1, 1) != 1)
                goto readquit;
            if (sy1 >= 'a' && sy1 <='z') {
                lread1 = (unsigned char) sy1 - 'a' + CCMAC+1;
                goto readychr;
            }
            goto new;
        default:
            ;
        }
        lread1 = sy1 & 037;
        goto corrcntr;
    }
    /* Введен управляющий символ - ищем команду в таблице */
    {
        struct ex_int *i1, *i2;
        int ts, k;
        i1 = i2 = 0;
        ts = 0;
        sy1 = lread1;
        while ((k = findt(&i1, &i2, sy1, ts++)) == CONTF) {
            if (read(inputfile, &sy1, 1) != 1)
                goto readquit;
        }
        if (k == BADF) {
            if (ts == 1)
                goto corrcntr;
            goto new;
        }
        lread1 = k;
        if (lread1 == CCMAC)
            goto rmacname;
        goto readychr;
    }
    /* ========================================================= */
corrcntr:
    if (lread1 > 0 && lread1 <= 037)
        lread1 = in0tab[lread1];
readychr:
    if (lread1 == -1)
        goto new;
    knockdown = 0;
    if (lread1 == CCCTRLQUOTE) {
        knockdown = 1;
    }
retn:
    sy1 = lread1;
    if (write (ttyfile, &sy1, 1) != 1)
        /* ignore errors */;
    /*
     * Конец того, что относится к терминалу (до quit).
     * ================================================
     */
retnm:
    lread1 = (unsigned char) lread1;
    if (lread1 > CCMAC && lread1 <= CCMAC+1+'z'-'a' && (symac = rmacl(lread1)))
        goto rmac;
retnl:
    return lread1;
readquit:
    if (intrflag) {
        lread1 = CCENTER;
        intrflag = 0;
        goto readychr;
    }
    lread1 = CCQUIT;
    goto readychr;
}
#undef lread1

/*
 * We were interrupted?
 */
int interrupt()
{
    char sy1;

    if (inputfile) {
        if (isy0f == CCINTRUP) {
            isy0f = -1;
            return 1;
        }
        return 0;
    }
    if (intrflag) {
        intrflag = 0;
        sy1 = CCINTRUP;
        if (write(ttyfile, &sy1, 1) != 1)
            /* ignore errors */;
        return 1;
    }
    return 0;
}

#define CCDEL 0177

/*
 * Read raw input characters.
 */
int read2()
{
    char c;

    if (inputfile && readfc())
        return lread1;
    if (read(0, &c, 1) != 1) {
        c = CCDEL;
        intrflag = 0;
    }
    c &= 0177;
    if (write(ttyfile, &c, 1) != 1)
        /* ignore errors */;
    return (unsigned char) c;
}

/*
 * Add new command key to the code table.
 */
extern int nfinc; /* число свободных мест в таблице */

int addkey(cmd, key)
    int cmd;
    char *key;
{
    struct ex_int *fb, *fe;
    register struct ex_int *fw;
    register int ns, i;

    ns=0;
    fb = fe = 0;
    while ((i = findt(&fb, &fe, key[ns], ns)) == CONTF && key[ns++]);
    if (i != BADF) {
        telluser("key redefined",0);
        fw = fb;
        goto retn;
    }
    /* Код новый = нужно расширить таблицу */
    if (!nfinc) {
        error("too many key's");
        return(0);
    }
    fw = fe;
    nfinc--;
    while ((fw++)->excc);
    do {
        *fw = *(fw-1);
    } while (--fw != fe);
retn:
#ifdef TEST
    test("addkey out");
#endif
    fw ->excc = key;
    fw->incc = cmd;
    return(1);
}

#ifdef TEST
test(s)
char *s;
{
    printf("test: %s\n",s);
    return(0);
}
#endif
