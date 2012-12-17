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
    char lmc, *cp, rmargflg;

    l0 = lo;
    lo += 2;
    if (lo > 0)
        lo = 0; /* Нач. колонка */
    l1 = lo;
    lmc = (curwin->lmarg == curwin->text_topcol ? 0 :
        curwksp->topcol == 0 ? LMCH : MLMCH);
    rmargflg = (curwin->text_topcol + curwin->text_width < curwin->rmarg);
    while (l0 <= lf) {
        lo = l1;
        if (l0 < 0) {
            l0 = lf;
            lf = -1;
            i = 0;
        } else {
            if (l0 != lf && interrupt())
                return;
            i = wksp_seek(curwksp, curwksp->toprow + l0);
            if (i && lmc != 0)
                lmc = ELMCH;
        }
        if (lmc == curwin->lmchars[l0] || lmc == 0 || lo < 0)
            poscursor(0, l0);
        else {
            poscursor(-1, l0);
            putch(lmc, 0);
        }
        curwin->lmchars[l0] = lmc;
        if (rmargflg != 0)
            rmargflg = RMCH;
        if (i != 0)
            i = 0;
        else {
            if (lf >= 0)
                chars(1);
            i = (cline_len - 1) - curwksp->topcol;
            if (i < 0)
                i = 0;
            else if (i > curwin->text_width) {
                if (i > 1 + curwin->text_width && rmargflg)
                    rmargflg = MRMCH;
                i = 1 + curwin->text_width;
            }
        }
        /*
         * Вывод символов.
         * Пытаемся пропустить начальные пробелы
         */
        if (lo == 0) {
            for (fc=0; cline != 0 && cline[curwksp->topcol + fc]==' '; fc++);
            j = curwin->text_width+1;
            if (fc > j)
                fc = j;
            if (fc > 127)
                fc = 127;
            lo = (curwin->firstcol)[l0] > fc ?
                - fc : - (curwin->firstcol)[l0];
            if (i + lo <= 0)
                lo = 0;
            else
                curwin->firstcol[l0] = fc;
        }
        if (lo)
            poscursor(-lo, l0);
        j = i + lo;
        cp = cline + curwksp->topcol - lo;
        while(j--)
            putcha(*cp++);
        cursorcol += (i + lo);
        if (curwin->lastcol[l0] < cursorcol)
            curwin->lastcol[l0] = cursorcol;
        /* Хвост строки заполняем пробелами */
        k = (j = curwin->lastcol[l0]) - i;
        if (k > 0) {
            putblanks(k);
        }
        if (curwin->text_topcol + cursorcol >= NCOLS)
            cursorcol = - curwin->text_topcol;
        if (rmargflg && rmargflg != curwin->rmchars[l0]) {
            poscursor(curwin->rmarg - curwin->text_topcol, l0);
            putch(rmargflg,0);
        } else
            movecursor(0);
        curwin->rmchars[l0] = rmargflg;
        curwin->lastcol[l0] = (k > 0 ? i : j);
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
    register int scol;
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
    scol = col + curwin->text_topcol;
    slin = lin + curwin->text_toprow;    /* screen col, lin */
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
        col = 0;
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
        col = col + curwksp->topcol;
        while (col >= tabstops[i])
            i++;
        col = tabstops[i] - curwksp->topcol;
        break;
    case CCBACKTAB:             /* tab left */
        i = 0;
        col = col + curwksp->topcol;
        while (col >  tabstops[i])
            i++;
        col = (i ? tabstops[i-1] - curwksp->topcol : -1);
        break;
    }
    if (col > curwin->text_width)
        col = 0;
    else if (col < 0)
        col = curwin->text_width;

    if (lin < 0)
        lin = curwin->text_height;
    else if (lin > curwin->text_height)
        lin = 0;

    poscursor(col, lin);
}

/*
 * Put a symbol to current position.
 * When flag=1, count it to line size.
 */
void putch(j, flg)
    int j, flg;
{
    if (flg && keysym != ' ') {
        if (curwin->firstcol[cursorline] > cursorcol)
            curwin->firstcol[cursorline] = cursorcol;
        if (curwin->lastcol[cursorline] <= cursorcol)
            curwin->lastcol[cursorline] = cursorcol + 1;
    }
    ++cursorcol;
    if (curwin->text_topcol + cursorcol >= NCOLS)
        cursorcol = - curwin->text_topcol;
    putcha(j);
    if (cursorcol <= 0)
        poscursor(0,
            cursorline < 0 ? 0 :
            cursorline > curwin->text_height ? 0 :
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
    window_t *w;
#define LPARAM 20       /* length increment */

    if (paraml != 0 && paramv != 0)
        free(paramv);
    paramc1 = paramc0 = cursorcol;
    paramr1 = paramr0 = cursorline;
    putch(COCURS,1);
    poscursor(cursorcol,cursorline);
    w = curwin;
back:
    telluser(macro ? "mac: " : "arg: ", 0);
    win_switch(&paramwin);
    poscursor(5,0);
    do {
        keysym = -1;
        getkeysym();
    } while (keysym == CCBACKSPACE);

    if (macro)
        goto rmac;
    if (MOVECMD(keysym)) {
        telluser("arg: *** cursor defined ***", 0);
        win_switch(w);
        poscursor(paramc0, paramr0);
t0:
        while (MOVECMD(keysym)) {
            movecursor(keysym);
            if (cursorline == paramr0 && cursorcol == paramc0)
                goto back;
            keysym = -1;
            getkeysym();
        }
        if (CTRLCHAR(keysym) && keysym != CCBACKSPACE) {
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
            keysym = -1;
            getkeysym();
            goto t0;
        }
    } else if (CTRLCHAR(keysym)) {
        paraml = 0;
        paramv = NULL;
        paramtype = 0;
    } else {
rmac:
        paraml = pn = 0;
loop:
        c = getkeysym();
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
        if ((! macro && keysym < ' ') || c==CCBACKSPACE || c==CCQUIT) {
            if (c == CCBACKSPACE && cursorcol != 0) {
                /* backspace */
                if (pn == 0) {
                    keysym = -1;
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
                keysym = -1;
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
            keysym = -1;
            goto loop;
        }
        paramtype = 1;
    }
    win_switch(w);
    putup(paramr0, paramr0);
    poscursor(paramc0, paramr0);
    return (paramv);
}

/*
 * Draw borders for a window.
 * When vertf, draw a vertical borders.
 */
void win_draw(win, vertf)
    register window_t *win;
    int vertf;
{
    register int i;

    win_switch(&wholescreen);
    if (win->tmarg != win->text_toprow) {
        poscursor(win->lmarg, win->tmarg);
        for (i = win->lmarg; i <= win->rmarg; i++)
            putch(TMCH, 0);
    }
    if (vertf) {
        int j;
        for (j = win->tmarg + 1; j <= win->bmarg - 1; j++) {
            int c = win->lmchars[j - win->tmarg - 1];
            if (c != 0) {
                poscursor(win->lmarg, j);
                putch(c, 0);
                poscursor(win->rmarg, j);
                putch(win->rmchars[j - win->tmarg - 1], 0);
            }
        }
    }
    if (win->tmarg != win->text_toprow) {
        poscursor(win->lmarg, win->bmarg);
        for (i = win->lmarg; i <= win->rmarg; i++)
            putch(BMCH, 0);
    }
    /* poscursor(win->lmarg + 1, win->tmarg + 1); */
    win_switch(win);
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
    message_displayed = 1;
}

/*
 * Display a message from column col.
 * When col=0 - clear the arg area.
 */
void telluser(msg, col)
    char *msg;
    int col;
{
    window_t *oldwin;
    register int c,l;
    oldwin = curwin;
    c = cursorcol;
    l = cursorline;
    win_switch(&paramwin);
    if (col == 0)
    {
        poscursor(0,0);
        putblanks(paramwin.text_width);
    }
    poscursor(col,0);
    /* while (*msg) putch(*msg++, 0); */
    putstr(msg, PARAMREDIT);
    win_switch(oldwin);
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
    register window_t *curp, *curp0 = curwin;
    int col = cursorcol, lin = cursorline;

    win_switch(&wholescreen);
    cursorcol = cursorline = 0;
    putcha(COFIN);
    putcha(COSTART);
    putcha(COHO);
    for (j=0; j<nwinlist; j++) {
        win_switch(winlist[j]);
        curp = curwin;
        for (i=0; i<curwin->text_height+1; i++) {
            curp->firstcol[i] = 0;
            curp->lastcol[i] = 0; /* curwin->text_width;*/
            curp->lmchars[i] = curp->rmchars[i] =' ';
        }
        win_draw(curp, 0);
        putup(0, curp->text_height);
    }
    win_switch(curp0);
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
