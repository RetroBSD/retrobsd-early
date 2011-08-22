/*
 * Interface to display - logical level.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"

/*
 * putup(l0,lf) - выдать строки с l0 до lf
 * Особый случай - если l0 отрицательно, то выдать только строку lf.
 * При этом строка берется из cline, и выдавать только с колонки -l0.
 */
void putup(lo, lf)
    int lo, lf;
{
    register int i, l0, fc;
    int j, k, l1;
    char lmc, *cp, c, rmargflg;

    l0 = lo;
    lo += 2;
    if (lo > 0)
        lo = 0; /* Нач. колонка */
    l1 = lo;
    lmc = (curport->lmarg == curport->ltext ? 0 :
        curwksp->ulhccno == 0 ? LMCH : MLMCH);
    rmargflg = (curport->ltext + curport->rtext < curport->rmarg);
    while (l0 <= lf) {
        lo = l1;
        if (l0 < 0) {
            l0 = lf;
            lf = -1;
            i = 0;
        } else {
            if (l0 != lf && interrupt())
                return;
            i = wseek(curwksp, curwksp->ulhclno + l0);
            if (i && lmc != 0)
                lmc = ELMCH;
        }
        if (lmc == curport->lmchars[l0] || lmc == 0 || lo < 0)
            poscursor(0, l0);
        else {
            poscursor(-1, l0);
            putch(lmc, 0);
        }
        curport->lmchars[l0] = lmc;
        if (rmargflg != 0)
            rmargflg = RMCH;
        if (i != 0)
            i = 0;
        else {
            if (lf >= 0)
                c = chars(1);
            i = (ncline - 1) - curwksp->ulhccno;
            if (i < 0)
                i = 0;
            else if (i > curport->rtext) {
                if (i > 1 + curport->rtext && rmargflg)
                    rmargflg = MRMCH;
                i = 1 + curport->rtext;
            }
        }
        /*
         * Вывод символов.
         * Пытаемся пропустить начальные пробелы
         */
        if (lo == 0) {
            for (fc=0; cline != 0 && cline[curwksp->ulhccno + fc]==' '; fc++);
            j = curport->rtext+1;
            if (fc > j)
                fc = j;
            if (fc > 127)
                fc = 127;
            lo = (curport->firstcol)[l0] > fc ?
                - fc : - (curport->firstcol)[l0];
            if (i + lo <= 0)
                lo = 0;
            else
                curport->firstcol[l0] = fc;
        }
        if (lo)
            poscursor(-lo, l0);
        j = i + lo;
        cp = cline + curwksp->ulhccno - lo;
        while(j--)
            putcha(*cp++);
        cursorcol += (i + lo);
        if (curport->lastcol[l0] < cursorcol)
            curport->lastcol[l0] = cursorcol;
        /* Хвост строки заполняем пробелами */
        k = (j = curport->lastcol[l0]) - i;
        if (k > 0) {
            putblanks(k);
        }
        if (curport->ltext + cursorcol >= LINEL)
            cursorcol = - curport->ltext;
        if (rmargflg && rmargflg != curport->rmchars[l0]) {
            poscursor(curport->rmarg - curport->ltext, l0);
            putch(rmargflg,0);
        } else
            movecursor(0);
        curport->rmchars[l0] = rmargflg;
        curport->lastcol[l0] = (k > 0 ? i : j);
        ++l0;
    }
}

/*
 * poscursor(col,lin) -
 * Позиционирование курсора в текущем окне
 */
void poscursor(col, lin)
    int col, lin;
{
    register int scol,dcol,dlin;
    int slin;

    if (cursorline == lin) {
        if (cursorcol == col)
            return;
        if ((cursorcol == col-1) && putcha(CORT)) {
            ++cursorcol;
            return;
        }
        if ((cursorcol == col+1) && putcha(COLT)) {
            --cursorcol;
            return;
        }
    }
    if (cursorcol == col) {
        if ((cursorline == lin-1) && putcha(CODN)) {
            ++cursorline;
            return;
        }
        if ((cursorline == lin+1) && putcha(COUP)) {
            --cursorline;
            return;
        }
    }
    scol = col + curport->ltext;
    slin = lin + curport->ttext;    /* screen col, lin */
    dcol = col - cursorcol;
    dlin = lin - cursorline;        /* delta col, lin */
    cursorcol = col;
    cursorline = lin;
    pcursor(scol, slin);            /* direct positioning */
}

/*
 * Движение курсора в границах окна "curscreen"
 * Значение аргумента:
 * UP - Вверх
 * CR - переход на начало строки
 * DN - вниз на строку
 * RT - вправо на колонку
 * LT - влево на колонку
 * TB - прямая табуляция
 * BT - табуляция назад
 *  0 - не операции (только проверить)
 */
void movecursor(arg)
    int arg;
{
    register int lin, col, i;

    lin = cursorline;
    col = cursorcol;
    switch (arg) {
    case 0:
        break;
    case CCHOME:                /* home cursor */
        col = lin = 0;
        break;
    case CCMOVEUP:              /* move up 1 line */
        --lin;
        break;
    case CCRETURN:              /* return */
        col = curport->ledit;
        /* fall through... */
    case CCMOVEDOWN:            /* move down 1 line */
        ++lin;
        break;
    case CCMOVERIGHT:           /* forward move */
        ++col;
        break;
    case CCMOVELEFT:            /* backspace */
        --col;
        break;
    case CCTAB:                 /* tab */
        i = 0;
        col = col + curwksp->ulhccno;
        while (col >= tabstops[i])
            i++;
        col = tabstops[i] - curwksp->ulhccno;
        break;
    case CCBACKTAB:             /* tab left */
        i = 0;
        col = col + curwksp->ulhccno;
        while (col >  tabstops[i])
            i++;
        col = (i ? tabstops[i-1] - curwksp->ulhccno : -1);
        break;
    }
    if (col > curport->redit)
        col = curport->ledit;
    else if (col < curport->ledit)
        col = curport->redit;

    if (lin < curport->tedit)
        lin = curport->bedit;
    else if (lin > curport->bedit)
        lin = curport->tedit;

    poscursor(col, lin);
}

/*
 * Put a symbol to current position.
 * When flag=1, count it to line size.
 */
void putch(j, flg)
    int j, flg;
{
    if (flg && lread1 != ' ') {
        if (curport->firstcol[cursorline] > cursorcol)
            curport->firstcol[cursorline] = cursorcol;
        if (curport->lastcol[cursorline] <= cursorcol)
            curport->lastcol[cursorline] = cursorcol + 1;
    }
    ++cursorcol;
    if (curport->ltext + cursorcol >= LINEL)
        cursorcol = - curport->ltext;
    putcha(j);
    if (cursorcol <= 0)
        poscursor(curport->ledit,
            cursorline < curport->tedit ? curport->tedit :
            cursorline > curport->bedit ? curport->tedit :
            cursorline);
    movecursor(0);
}

/*
 * Get a parameter.
 * Три типа параметров.
 *         paramtype = -1 -- определение области
 *         paramtype = 0  -- пустой аргумент
 *         paramtype = 1  -- строка.
 *         при использовании макро бывает paramtype = -2 - tag defined
 * Возвращается указатель на введенную строку (paramv).
 * Длина возвращается в переменной paraml.
 * Если при очередном вызове paraml не 0, старый paramv
 * освобождается, так что если старый параметр нужен,
 * нужно обнулить paraml.
 */
char *param(macro)
    int macro;
{
    register char *c1;
    char *cp,*c2;
    int c;
    register int i,pn;
    struct viewport *w;
#define LPARAM 20       /* length increment */

    if (paraml != 0 && paramv != 0)
        free(paramv);
    paramc1 = paramc0 = cursorcol;
    paramr1 = paramr0 = cursorline;
    putch(COCURS,1);
    poscursor(cursorcol,cursorline);
    w = curport;
back:
    telluser(macro ? "mac: " : "arg: ", 0);
    switchport(&paramport);
    poscursor(5,0);
    do {
        lread1 = -1;
        read1();
    } while (lread1 == CCBACKSPACE);

    if (macro)
        goto rmac;
    if (MOVECMD(lread1)) {
        telluser("arg: *** cursor defined ***", 0);
        switchport(w);
        poscursor(paramc0, paramr0);
t0:
        while (MOVECMD(lread1)) {
            movecursor(lread1);
            if (cursorline == paramr0 && cursorcol == paramc0)
                goto back;
            lread1 = -1;
            read1();
        }
        if (CTRLCHAR(lread1) && lread1 != CCBACKSPACE) {
            if (cursorcol > paramc0)
                paramc1 = cursorcol;
            else
                paramc0 = cursorcol;
            if (cursorline > paramr0)
                paramr1 = cursorline;
            else
                paramr0 = cursorline;
            paraml = 0;
            paramv = NULL;
            paramtype = -1;
        } else {
            error("Printing character illegal here");
            lread1 = -1;
            read1();
            goto t0;
        }
    } else if (CTRLCHAR(lread1)) {
        paraml = 0;
        paramv = NULL;
        paramtype = 0;
    } else {
rmac:
        paraml = pn = 0;
loop:
        c = read1();
        if (pn >= paraml) {
            cp = paramv;
            paramv = salloc(paraml + LPARAM + 1); /* 1 for dechars */
            c1 = paramv;
            c2 = cp;
            for (i=0; i<paraml; ++i)
                *c1++ = *c2++;
            if (paraml)
                free(cp);
            paraml += LPARAM;
        }
        /* Конец ввода параметра */
        if ((! macro && lread1 < ' ') || c==CCBACKSPACE || c==CCQUIT) {
            if (c == CCBACKSPACE && cursorcol != curport->ledit) {
                /* backspace */
                if (pn == 0) {
                    lread1 = -1;
                    goto loop;
                }
                movecursor(CCMOVELEFT);
                --pn;
                if ((paramv[pn] & 0340) == 0) {
                    putch(' ', 0);
                    movecursor(CCMOVELEFT);
                    movecursor(CCMOVELEFT);
                }
                paramv[pn] = 0;
                putch(' ', 0);
                movecursor(CCMOVELEFT);
                lread1 = -1;
                if (pn == 0)
                    goto back;
                goto loop;
            } else
                c = 0;
        }
        if (c == 0177)          /* del is a control code */
            c = 0;
        paramv[pn++] = c;
        if (c != 0) {
            if ((c & 0140) == 0){
                putch('^', 0);
                c = c | 0100;
            }
            putch(c, 0);
            lread1 = -1;
            goto loop;
        }
        paramtype = 1;
    }
    switchport(w);
    putup(paramr0, paramr0);
    poscursor(paramc0, paramr0);
    return (paramv);
}

/*
 * Draw borders for a window.
 * When vertf, draw a vertical borders.
 */
void drawport(newp, vertf)
    struct viewport *newp;
    int vertf;
{
    register struct viewport *newport;
    register int i, c;
    int j;

    newport = newp;
    switchport(&wholescreen);
    if(newport->tmarg != newport->ttext) {
        poscursor(newport->lmarg,newport->tmarg);
        for (i = newport->lmarg; i <= newport->rmarg; i++)
            putch(TMCH, 0);
    }
    if (vertf)
        for (j = newport->tmarg + 1; j <= newport->bmarg - 1; j++) {
            c = newport->lmchars[j - newport->tmarg - 1];
            if (c != 0) {
                poscursor(newport->lmarg, j);
                putch(c, 0);
                poscursor(newport->rmarg, j);
                putch(newport->rmchars[j - newport->tmarg - 1], 0);
            }
        }
    if (newport->tmarg != newport->ttext) {
        poscursor(newport->lmarg, newport->bmarg);
        for (i = newport->lmarg; i <= newport->rmarg; i++)
            putch(BMCH, 0);
    }
    /* poscursor(newport->lmarg + 1,newport->tmarg + 1); */
    switchport(newport);
}

/*
 * Display error message.
 */
void error(msg)
    char *msg;
{
    putcha(COBELL);
    putcha(COBELL);
    putcha(COBELL);
    telluser("**** ", 0);
    telluser(msg, 5);
    errsw = 1;
}

/*
 * Display a message from column col.
 * When col=0 - clear the arg area.
 */
void telluser(msg, col)
    char *msg;
    int col;
{
    struct viewport *oldport;
    register int c,l;
    oldport = curport;
    c = cursorcol;
    l = cursorline;
    switchport(&paramport);
    if (col == 0)
    {
        poscursor(0,0);
        putblanks(paramport.redit);
    }
    poscursor(col,0);
    /* while (*msg) putch(*msg++, 0); */
    putstr(msg, PARAMREDIT);
    switchport(oldport);
    poscursor(c,l);
    dumpcbuf();
}

/*
 * Redraw a screen.
 */
void rescreen()
{
    register int i;
    int j;
    register struct viewport *curp, *curp0 = curport;
    int col = cursorcol, lin = cursorline;

    switchport(&wholescreen);
    cursorcol = cursorline = 0;
    putcha(COFIN);
    putcha(COSTART);
    putcha(COHO);
    for (j=0; j<nportlist; j++) {
        switchport(portlist[j]);
        curp = curport;
        for (i=0; i<curport->btext+1; i++) {
            curp->firstcol[i] = 0;
            curp->lastcol[i] = 0; /* curport->rtext;*/
            curp->lmchars[i] = curp->rmchars[i] =' ';
        }
        drawport(curp, 0);
        putup(0, curp->btext);
    }
    switchport(curp0);
    poscursor(col, lin);
}

/*
 * Put a string, limited by column.
 */
void putstr(ss, ml)
    char *ss;
    int ml;
{
    register char *s = ss;

    while (*s && cursorcol < ml)
        putch(*s++, 0);
}
