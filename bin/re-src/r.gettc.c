/*
 * Редактор RED. ИАЭ им. И.В. Курчатова, ОС ДЕМОС
 * Вытаскивание описания терминала из termcap
 * и работа с таблицей команд.
 */
extern char *UP, *BC;
char **toup = &UP;        /* Так сложно из-за совпадения имен */

#include "r.defs.h"

/*
 * Выходные коды
 */
char *cvtout[] = {
    /* COSTART */ "cl?is?ti?ho",    /* COUP   */ "up",
    /* CODN    */ "do",             /* CORN   */ "\015",
    /* COHO    */ "ho",             /* CORT   */ "nd",
    /* COLT    */ "le",             /* COCURS */ "cu",
    /* COBELL  */ "\007",           /* COFIN  */ "cl?fs?te",
    /* COERASE */ "cl",
};

char *curspos;

/*   Входные коды
 *
 *   Таблица in0tab является временным явлением для
 *   замены кодов 01 - 010.
 */

char in0tab[BT] = { /* meanings of the <ctrl>A - <ctrl>-H */
    CCLPORT, BT, CCCHPORT, CCSETFILE, CCMISRCH, CCPICK, CCPUT, LT,
};

/*
 * Таблица кодов клавиш терминала и их комбинаций
 * Сюда же записываются коды при переопределении
 */
struct ex_int inctab[] = {
    BT,         "\002",     /* ^B */
    BT,         "k.",
    LT,         "kl",
    TB,         "\011",     /* ^I */
    HO,         "kh",
    RN,         "\015",     /* ^M */
    UP,         "ku",
    DN,         "kd",
    RT,         "kr",
    LT,         "kl",
    CCDELCH,    "kD",
    CCBACKSPACE,"\10",      /* ^H */
    CCCHPORT,   "k2k0",
    CCCLOSE,    "k2k8",
    CCCTRLQUOTE,"\20",      /* ^P */
    CCDELCH,    "k6",
    CCDOCMD,    "k2k.",
    CCENTER,    "k1",
    CCGOTO,     "k4",
    CCINSMODE,  "kI",       CCINSMODE,  "k5",
    CCLPORT,    "k2kl",
    CCMAKEPORT, "k2k4",
    CCMILINE,   "k2ku",
    CCMIPAGE,   "\33k7",
    CCMISRCH,   "k2k3",
    CCOPEN,     "k8",
    CCPICK,     "k9",
    CCPLLINE,   "k2kd",
    CCPLPAGE,   "k7",
    CCPLSRCH,   "k3",
    CCPUT,      "k2k9",
    CCRPORT,    "k2kr",
    CCSAVEFILE, "k2k-",
    CCSETFILE,  "k-",
    CCTABS,     "k2k5",
    CCPLPAGE,   "kN",
    CCMIPAGE,   "kP",
    CCQUIT,     "k0",
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0,
};

/* Декодирование termcap */
int nfinc = 8;

/*
 * char *gettcs(tc) -
 * дай описание возможности "tc"
 * tc="XXYY..ZZ", знак вопроса перед кодом
 * возможности означает - не обязательна
 */
char *gettcs(tc)
char *tc;
{
    char name[3], buftc0[128], *buftc = buftc0;
    register char c1, *c;
    int i, optional;

    if (*tc < ' ' && *tc >= 0)
        return(tc);
    c = tc;
    while (*c != 0) {
        if (*c == '?') {
            optional = 1;
            c++;
        } else if (*c < ' ') {
            *buftc++ = *c++;
            continue;
        } else
            optional = 0;
        name[0] = *c++;
        name[1] = *c++;
        name[2] = 0;
        if (tgetstr(name, &buftc) == 0) {
            /* not found */
            if (! optional)
                return(0);
            *buftc++ = 0;
        }
        buftc--;
    }
    *buftc++ = 0;
    buftc = strdup (buftc0);
#ifdef TEST
    printf("%s=", tc);
    ptss(buftc);
#endif
    return(buftc);
}

/*
 * tcread()
 * Чтение описаний. Код ответа:
 * 0 - term cap reading O'KEY
 * 1 - no termcap
 * 2 - not enoughf termcap
 */
tcread()
{
    register int i;
    register struct ex_int *ir, *iw;

    tgettc();
    LINEL = tgetnum("co");
    NLINES = tgetnum("li");
    for (i=0; i<COMCOD; i++) {
        cvtout[i] = gettcs(cvtout[i]);
    }
    if (tgetflag("nb"))
        cvtout[COBELL] = "\0";
    if (tgetflag("bs"))
        cvtout[COLT] = "\010";
    if (curspos = gettcs("cm"))
        agoto = tgoto;
    if (! cvtout[COCURS])
        cvtout[COCURS] = "@";
    if (LINEL <= 60 || NLINES < 8)
        return(2);
    if (LINEL > LINELM)
        LINEL = LINELM;
    if (NLINES > NLINESM)
        NLINES = NLINESM;
    if (! curspos) {
        for (i=0; i<COMCOD; i++) {
            if (! cvtout[i]) {
                printf1 ("re can not work with this terminal\n");
                exit(1);
            }
        }
    }
    /*
     input codes definition
     */
    ir = iw = inctab;
    while (ir->excc) {
        if ((iw->excc = gettcs(ir->excc))) {
            iw->incc = ir->incc;
            iw++;
        } else
            nfinc++;
        ir++;
    }
    iw->excc = NULL;
    iw->incc = 0;
    itsort(inctab,iw-1,0);
    *toup = cvtout[UP];
    BC = cvtout[LT];
}

/*
 * itsort(fb,fe,ns) -
 * сортировка inctab  для работы
 * функции findt
 */
itsort(fb,fe,ns)
struct ex_int *fb,*fe;
int ns;
{
    register struct ex_int *fr, *fw;
    char c;
    struct ex_int temp;

    fw = fb - 1;
    while (fw != fe) {
        fr = fb = ++fw;
        c = fw->excc[ns];
        while (fr++ != fe) {
            if (fr->excc[ns] == c) {
                if (fr != ++fw) {
                    temp = *fr;
                    *fr = *fw;
                    *fw = temp;
                }
            }
        }
        if (c != 0 && (fw - fb) > 1)
            itsort(fb, fw, ns+1);
    }
}
