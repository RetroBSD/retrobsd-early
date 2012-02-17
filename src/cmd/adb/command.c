#include "defs.h"

MAP     txtmap;
MAP     datmap;

int     executing;
char    *lp;
int     fcor;
int     fsym;
int     mkfault;
const char *errflg;
char    lastc;
char    eqformat[512] = "o";
char    stformat[512] = "o\"= \"^i";
u_int   *uar0;
long    dot;
long    ditto;
int     dotinc;
int     lastcom = '=';
long    locval;
long    locmsk;
int     pid;
long    expv;
long    adrval;
int     adrflg;
long    cntval;
int     cntflg;

extern u_int corhdr[];
extern long var[];
extern char *myname;

/*
 * command decoding
 */
int
command(buf, defcom)
    char    *buf;
    char    defcom;
{
    int     itype, ptype, modifier, regptr;
    char    longpr, eqcom;
    char    wformat[1];
    char    savc;
    long    w, savdot;
    char    *savlp=lp;

    if (buf) {
        if (*buf == EOR)
            return(FALSE);
        lp = buf;
    }

    do {
        adrflg = expr(0);
        if (adrflg) {
            dot = expv;
            ditto = dot;
        }
        adrval = dot;
        if (rdc()==',' && expr(0)) {
            cntflg = TRUE;
            cntval = expv;
        } else {
            cntflg = FALSE;
            cntval = 1;
            lp--;
        }

        if (! eol(rdc())) {
            lastcom = lastc;
        } else {
            if (adrflg == 0) {
                dot = inkdot(dotinc);
            }
            lp--;
            lastcom = defcom;
        }

        switch (lastcom & STRIP) {
        case '/':
            itype = DSP;
            ptype = DSYM;
            goto trystar;

        case '=':
            itype = NSP;
            ptype = ASYM;
            goto trypr;

        case '?':
            itype = ISP;
            ptype = ISYM;
            goto trystar;

trystar:
            if (rdc() == '*') {
                lastcom |= QUOTE;
            } else {
                lp--;
            }
            if (lastcom & QUOTE) {
                itype |= STAR;
                ptype = (DSYM + ISYM) - ptype;
            }
trypr:
            longpr = FALSE;
            eqcom = (lastcom == '=');

            switch (rdc()) {
            case 'm': {                 /* reset map data */
                int         fcount;
                MAPPTR      smap;
                long        *mp;

                if (eqcom) {
                    error(BADEQ);
                }
                smap = (itype & DSP) ? &datmap : &txtmap;
                fcount = 3;
                if (itype & STAR) {
                    mp = &(smap->b2);
                } else {
                    mp = &(smap->b1);
                }
                while (fcount-- && expr(0)) {
                    *(mp)++ = expv;
                }
                if (rdc() == '?') {
                    smap->ufd = fsym;
                } else if (lastc == '/') {
                    smap->ufd = fcor;
                } else {
                    lp--;
                }
                }
                break;

            case 'L':
                longpr = TRUE;
            case 'l':                   /* search for exp */
                if (eqcom) {
                    error(BADEQ);
                }
                dotinc = 2;
                savdot = dot;
                expr(1);
                locval = expv;
                if (expr(0)) {
                    locmsk = expv;
                } else {
                    locmsk = -1L;
                }
                for (;;) {
                    w = leng(get(dot, itype));
                    if (longpr) {
                        w = itol(w, get(inkdot(2), itype));
                    }
                    if (errflg || mkfault || (w & locmsk) == locval)
                        break;
                    dot = inkdot(dotinc);
                }
                if (errflg) {
                    dot = savdot;
                    errflg = NOMATCH;
                }
                psymoff(dot, ptype, "");
                break;

            case 'W':
                longpr = TRUE;
            case 'w':
                if (eqcom) {
                    error(BADEQ);
                }
                wformat[0] = lastc;
                expr(1);
                do {
                    savdot = dot;
                    psymoff(dot, ptype, ":%16t");
                    exform(1, wformat, itype, ptype);
                    errflg = 0;
                    dot = savdot;
                    if (longpr) {
                        put(dot, itype, expv);
                    }
                    put((longpr ? inkdot(2) : dot), itype, shorten(expv));
                    savdot = dot;
                    print("=%8t");
                    exform(1, wformat, itype, ptype);
                    printc(EOR);
                } while (expr(0) && errflg == 0);
                dot = savdot;
                chkerr();
                break;

            default:
                lp--;
                getformat(eqcom ? eqformat : stformat);
                if (! eqcom) {
                    if (*stformat != 'a') {
                        psymoff(dot, ptype, ":%16t");
                    }
                }
                scanform(cntval, (eqcom ? eqformat : stformat), itype, ptype);
            }
            break;

        case '>':
            lastcom = 0;
            savc = rdc();
            regptr = getreg(savc);
            if (regptr != NOREG) {
                uar0[regptr] = shorten(dot);
                ptrace(PT_WRITE_U, pid, &uar0[regptr] - (int)&corhdr,
                    uar0[regptr]);
            } else if ((modifier = varchk(savc)) != -1) {
                var[modifier] = dot;
            } else {
                error(BADVAR);
            }
            break;

        case '!':
            lastcom = 0;
            unox();
            break;

        case '$':
            lastcom = 0;
            printtrace(nextchar());
            break;

        case ':':
            if (! executing) {
                executing = TRUE;
                subpcs(nextchar());
                executing = FALSE;
                lastcom = 0;
            }
            break;

        case 0:
            print("%s\n", myname);
            break;

        default:
            error(BADCOM);
        }
        flushbuf();

    } while (rdc() == ';');

    if (buf) {
        lp = savlp;
    } else {
        lp--;
    }
    return (adrflg && dot != 0);
}
