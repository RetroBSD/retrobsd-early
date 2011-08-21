/*
 *      Редактор RED. ИАЭ им. И.В. Курчатова, ОС ДЕМОС
 *      Мини / вариант termcap.
 *      $Header: /home/sergev/Project/vak-opensource/trunk/relcom/nred/RCS/r.termc.c, 3.1 1986/04/20 23:42:58 alex Exp $
 *      $Log: r.termc.c, $
 *      Revision 3.1  1986/04/20 23:42:58  alex
 *      *** empty log message ***
 *
 */
#define MAXHOP  32  /* max number of tc= indirections */
#define BUFSIZ  1024

#include <stdlib.h>
#include <string.h>
#include <sgtty.h>
#include <ctype.h>
#define E_TERMCAP "/etc/termcap"

/*
 * Программы для взаимодействия с описанием  терминала "/etc/termcap"
 */
static char *tbuf;
static int hopcount;   /* detect infinite loops in termcap, init 0 */
static char    *tskip();
static char    *tdecode();
char    *tgetstr();

/*
 * Terminal description is placed in TERMCAP variable.
 * ko= 1 - нормально, 0  - TERMCAP не найден
 */
tgettc()
{
    tbuf = getenv("TERMCAP");
    if (! tbuf) {
        printf1 ("re: TERMCAP environment variable required\n");
        exit(1);
    }
}

/*
 * Skip to the next field.  Notice that this is very dumb, not
 * knowing about \: escapes or any such.  If necessary, :'s can be put
 * into the termcap file in octal.
 */
static char *
tskip(bp)
register char *bp;
{
    while (*bp && *bp != ':')
        bp++;
    if (*bp == ':')
        bp++;
    return (bp);
}

/*
 * Return the (numeric) option id.
 * Numeric options look like
 *  li#80
 * i.e. the option string is separated from the numeric value by
 * a # character.  If the option is not found we return -1.
 * Note that we handle octal numbers beginning with 0.
 */
tgetnum(id)
char *id;
{
    register int i, base;
    register char *bp = tbuf;
    for (;;) {
        bp = tskip(bp);
        if (*bp == 0)
            return (-1);
        if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
            continue;
        if (*bp == '@')
            return(-1);
        if (*bp != '#')
            continue;
        bp++;
        base = 10;
        if (*bp == '0')
            base = 8;
        i = 0;
        while (isdigit(*bp))
            i *= base, i += *bp++ - '0';
        return (i);
    }
}

/*
 * Handle a flag option.
 * Flag options are given "naked", i.e. followed by a : or the end
 * of the buffer.  Return 1 if we find the option, or 0 if it is
 * not given.
 */
tgetflag(id)
char *id;
{
    register char *bp = tbuf;
    for (;;) {
        bp = tskip(bp);
        if (!*bp)
            return (0);
        if (*bp++ == id[0] && *bp != 0 && *bp++ == id[1]) {
            if (!*bp || *bp == ':')
                return (1);
            else if (*bp == '@')
                return(0);
        }
    }
}

/*
 * Get a string valued option.
 * These are given as
 *  cl=^Z
 * Much decoding is done on the strings, and the strings are
 * placed in area, which is a ref parameter which is updated.
 * No checking on area overflow.
 */
char *
tgetstr(id, area)
char *id, **area;
{
    register char *bp = tbuf;
    for (;;) {
        bp = tskip(bp);
        if (!*bp)
            return (0);
        if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
            continue;
        if (*bp == '@')
            return(0);
        if (*bp != '=')
            continue;
        bp++;
        return (tdecode(bp, area));
    }
}

/*
 * Tdecode does the grung work to decode the
 * string capability escapes.
 */
static char *
tdecode(str, area)
register char *str;
char **area;
{
    register char *cp;
    register int c;
    register char *dp;
    int i,jdelay=0;
    while(*str>='0' && *str<='9') {
        jdelay=jdelay*10+(*str++ - '0');
    }
    cp = *area;
    while ((c = *str++) && c != ':') {
        switch (c) {
        case '^':
            c = *str++ & 037;
            break;
        case '\\':
            dp = "E\033^^\\\\::n\nr\rt\tb\bf\f";
            c = *str++;
nextc:
            if (*dp++ == c) {
                c = *dp++;
                break;
            }
            dp++;
            if (*dp)
                goto nextc;
            if (isdigit(c)) {
                c -= '0', i = 2;
                do
                    c <<= 3, c |= *str++ - '0';
                while (--i && isdigit(*str));
            }
            break;
        }
        *cp++ = c;
    }
    if(jdelay>100) jdelay=100;
    while(jdelay--) *cp++='\200';
    *cp++ = 0;
    str = *area;
    *area = cp;
    return (str);
}
#define CTL(c) ('c' & 037)

char    *UP;
char    *BC;
#define UPTC UP
#define BCTC BC

/*
 * Routine to perform cursor addressing.
 * CM is a string containing printf type escapes to allow
 * cursor addressing.  We start out ready to print the destination
 * line, and switch each time we print row or column.
 * The following escapes are defined for substituting row/column:
 *
 *  %d  as in printf
 *  %2  like %2d
 *  %3  like %3d
 *  %.  gives %c hacking special case characters
 *  %+x like %c but adding x first
 *
 *  The codes below affect the state but don't use up a value.
 *
 *  %>xy    if value > x add y
 *  %r  reverses row/column
 *  %i  increments row/column (for one origin indexing)
 *  %%  gives %
 *  %B  BCTCD (2 decimal digits encoded in one byte)
 *  %D  Delta Data (backwards bcd)
 *
 * all other characters are ``self-inserting''.
 */
char *tgoto(CM, destcol, destline)
    char *CM;
    int destcol, destline;
{
    static char result[16];
    static char added[10];
    char *cp = CM;
    register char *dp = result;
    register int c;
    int oncol = 0;
    register int which = destline;
    if (cp == 0) {
toohard:
        /*
                 * ``We don't do that under BOZO's big top''
                 */
        return ("OOPS");
    }
    added[0] = 0;
    while (c = *cp++) {
        if (c != '%') {
            *dp++ = c;
            continue;
        }
        switch (c = *cp++) {
#ifdef CM_N
        case 'n':
            destcol ^= 0140;
            destline ^= 0140;
            goto setwhich;
#endif
        case 'd':
            if (which < 10)
                goto one;
            if (which < 100)
                goto two;
            /* fall into... */
        case '3':
            *dp++ = (which / 100) | '0';
            which %= 100;
            /* fall into... */
        case '2':
two:
            *dp++ = which / 10 | '0';
one:
            *dp++ = which % 10 | '0';
swap:
            oncol = 1 - oncol;
setwhich:
            which = oncol ? destcol : destline;
            continue;
#ifdef CM_GT
        case '>':
            if (which > *cp++)
                which += *cp++;
            else
                cp++;
            continue;
#endif
        case '+':
            which += *cp++;
            /* fall into... */

        case '.':
#ifndef lint
casedot:
#endif
            /*
             * This code is worth scratching your head at for a
             * while.  The idea is that various weird things can
             * happen to nulls, EOT's, tabs, and newlines by the
             * tty driver, arpanet, and so on, so we don't send
             * them if we can help it.
             *
 * Tab is taken out to get Ann Arbors to work, otherwise
             * when they go to column 9 we increment which is wrong
             * because bcd isn't continuous.  We should take out
             * the rest too, or run the thing through more than
             * once until it doesn't make any of these, but that
             * would make termlib (and hence pdp-11 ex) bigger,
             * and also somewhat slower.  This requires all
             * programs which use termlib to stty tabs so they
             * don't get expanded.  They should do this anyway
             * because some terminals use ^I for other things,
             * like nondestructive space.
             */
            if (UPTC && (which == 0 || which == CTL(d) || which==CTL(n) || which == '\t' || which == '\n')) {
                if (oncol || UPTC) /* Assumption: backspace works */
                    /*
                                         * Loop needed because newline happens
                                         * to be the successor of tab.
                                         */
                do {
                    strcat(added, oncol ? (BCTC ? BCTC : "\b") :
                    UPTC);
                    which++;
                }
                while (which == '\n');
            }
            *dp++ = which;
            goto swap;
        case 'r':
            oncol = 1;
            goto setwhich;
        case 'i':
            destcol++;
            destline++;
            which++;
            continue;
        case '%':
            *dp++ = c;
            continue;
#ifdef CM_B
        case 'B':
            which = (which/10 << 4) + which%10;
            continue;
#endif
#ifdef CM_D
        case 'D':
            which = which - 2 * (which%16);
            continue;
#endif

        default:
            goto toohard;
        }
    }
    strcpy(dp, added);
    return (result);
}
