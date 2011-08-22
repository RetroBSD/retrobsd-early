/*
 * Редактор RED. ИАЭ им. И.В. Курчатова, ОС ДЕМОС
 * Вытаскивание описания терминала из termcap
 * и работа с таблицей команд.
 */
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

/*
 * Meanings of the input control codes: ^A - ^H
 */
const char in0tab[32] = {
    -1,             /* ^@ */
    CCENTER,        /* ^A */
    CCMISRCH,       /* ^B */
    -1,             /* ^C */
    CCDELCH,        /* ^D */
    -1,             /* ^E */
    CCPLSRCH,       /* ^F */
    -1,             /* ^G */
    CCBACKSPACE,    /* ^H */
    CCTAB,          /* ^I */
    CCRETURN,       /* ^J */
    -1,             /* ^K */
    CCREDRAW,       /* ^L */
    CCRETURN,       /* ^M */
    CCSETFILE,      /* ^N */
    -1,             /* ^O */
    CCCTRLQUOTE,    /* ^P */
    -1 /*special*/, /* ^Q */
    -1,             /* ^R */
    -1 /*special*/, /* ^S */
    -1,             /* ^T */
    -1,             /* ^U */
    -1,             /* ^V */
    -1,             /* ^W */
    -1 /*special*/, /* ^X */
    CCCLOSE,        /* ^Y */
    -1,             /* ^Z */
    -1 /*special*/, /* ^[ */
    -1,             /* ^\ */
    -1,             /* ^] */
    -1,             /* ^^ */
    -1,             /* ^_ */
};

/*
 * Таблица кодов клавиш терминала и их комбинаций
 * Сюда же записываются коды при переопределении
 */
struct ex_int inctab[] = {
    { CCMOVELEFT,   "kl",   },
    { CCMOVERIGHT,  "kr",   },
    { CCMOVEUP,     "ku",   },
    { CCMOVEDOWN,   "kd",   },
    { CCHOME,       "kh",   },
    { CCEND,        "kH",   },
    { CCPLPAGE,     "kN",   },
    { CCMIPAGE,     "kP",   },
    { CCINSMODE,    "kI",   },
    { CCDELCH,      "kD",   },
    { CCENTER,      "k1",   },
    { CCSAVEFILE,   "k2",   },
    { CCCHPORT,     "k3",   },
    { CCMAKEPORT,   "k4",   },
    { CCSETFILE,    "k5",   },
    { CCPICK,       "k6",   },
    { CCOPEN,       "k7",   },
    { CCCLOSE,      "k8",   },
    { CCREDRAW,     "k9",   }, // free
    { CCGOTO,       "k0",   },
    { CCPUT,        "F1",   },
    { CCREDRAW,     "F2",   }, // free
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
};

/* Декодирование termcap */
int nfinc = 8;

/*
 * дай описание возможности "tc"
 * tc="XXYY..ZZ", знак вопроса перед кодом
 * возможности означает - не обязательна
 */
static char *gettcs(termcap, tc)
    char *termcap, *tc;
{
    char name[3], buftc0[128], *buftc = buftc0;
    register char *c;
    int optional;

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
        if (tgetstr(termcap, name, &buftc) == 0) {
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
 * сортировка inctab  для работы
 * функции findt
 */
static void itsort(fb, fe, ns)
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

/*
 * Load termcap descriptions.
 */
void tcread()
{
    register int i;
    register struct ex_int *ir, *iw;
    char *termcap;

    /* Terminal description is placed in TERMCAP variable. */
    termcap = getenv("TERMCAP");
    if (! termcap) {
#if 1
        termcap = "linux:co#80:li#25:bs:cm=\33[%i%d;%dH:cl=\33[H\33[2J:"
                  "ho=\33[H:up=\33[A:do=\33[B:nd=\33[C:le=\10:cu=\33[7m \33[m:"
                  "kh=\33[1~:ku=\33[A:kd=\33[B:kr=\33[C:kl=\33[D:kP=\33[5~:"
                  "kN=\33[6~:kI=\33[2~:kD=\33[3~:kh=\33[1~:kH=\33[4~:k.=\33[Z:"
                  "k1=\33[[A:k2=\33[[B:k3=\33[[C:k4=\33[[D:k5=\33[[E:"
                  "k6=\33[17~:k7=\33[18~:k8=\33[19~:k9=\33[20~:k0=\33[21~:"
                  "F1=\33[23~:F2=\33[24~:";
#else
        puts1 ("re: TERMCAP environment variable required\n");
        exit(1);
#endif
    }
    curspos = gettcs(termcap, "cm");
    if (! curspos) {
        puts1 ("re: terminal does not support direct positioning\n");
        exit(1);
    }
    LINEL = tgetnum(termcap, "co");
    NLINES = tgetnum(termcap, "li");
    if (LINEL <= 60 || NLINES < 8) {
        puts1 ("re: too small screen size\n");
        exit(1);
    }
    if (LINEL > LINELM)
        LINEL = LINELM;
    if (NLINES > NLINESM)
        NLINES = NLINESM;
    for (i=0; i<COMCOD; i++) {
        cvtout[i] = gettcs(termcap, cvtout[i]);
    }
    if (tgetflag(termcap, "nb"))
        cvtout[COBELL] = "\0";
    if (tgetflag(termcap, "bs"))
        cvtout[COLT] = "\010";
    if (! cvtout[COCURS])
        cvtout[COCURS] = "@";

    /* Input codes. */
    ir = iw = inctab;
    while (ir->excc) {
        if ((iw->excc = gettcs(termcap, ir->excc))) {
            iw->incc = ir->incc;
            iw++;
        } else
            nfinc++;
        ir++;
    }
    iw->excc = NULL;
    iw->incc = 0;
    itsort(inctab, iw-1, 0);
}
