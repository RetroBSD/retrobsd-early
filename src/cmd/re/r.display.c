/*
 * Interface to display - logical level.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"

/*
 * drawlines(l0,lf) - выдать строки с l0 до lf
 * Особый случай - если l0 отрицательно, то выдать только строку lf.
 * При этом строка берется из cline, и выдавать только с колонки -l0.
 */
void drawlines(lo, lf)
    int lo, lf;
{
    register int i, l0;
    int j, k, l1;
    char lmc, *cp, max_colflg;

    l0 = lo;
    lo += 2;
    if (lo > 0)
        lo = 0; /* Нач. колонка */
    l1 = lo;
    lmc = (curwin->base_col == curwin->text_col ? 0 :
        curwksp->coloffset == 0 ? LMCH : MLMCH);
    max_colflg = (curwin->text_col + curwin->text_maxcol < curwin->max_col);
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
        if (lmc == curwin->leftbar[l0] || lmc == 0 || lo < 0)
            poscursor(0, l0);
        else {
            poscursor(-1, l0);
            putch(lmc, 0);
        }
        curwin->leftbar[l0] = lmc;
        if (max_colflg != 0)
            max_colflg = RMCH;
        if (i != 0)
            i = 0;
        else {
            if (lf >= 0)
                chars(1);
            i = (cline_len - 1) - curwksp->coloffset;
            if (i < 0)
                i = 0;
            else if (i > curwin->text_maxcol) {
                if (i > 1 + curwin->text_maxcol && max_colflg)
                    max_colflg = MRMCH;
                i = 1 + curwin->text_maxcol;
            }
        }
        /*
         * Вывод символов.
         * Пытаемся пропустить начальные пробелы
         */
        if (lo == 0) {
            int fc;
            for (fc=0; cline != 0 && cline[curwksp->coloffset + fc]==' '; fc++);
            j = curwin->text_maxcol + 1;
            if (fc > j)
                fc = j;
            if (fc > 255)
                fc = 255;
            lo = (curwin->firstcol[l0] > fc) ? - fc : - curwin->firstcol[l0];
            if (i + lo <= 0)
                lo = 0;
            else
                curwin->firstcol[l0] = fc;
        }
        if (lo)
            poscursor(-lo, l0);
        j = i + lo;
        cp = cline + curwksp->coloffset - lo;
        while(j--)
            putcha(*cp++);
        cursorcol += (i + lo);
        if (curwin->lastcol[l0] < cursorcol)
            curwin->lastcol[l0] = cursorcol;

        /* Хвост строки заполняем пробелами */
        j = curwin->lastcol[l0];
        k = j - i;
        if (k > 0) {
            putblanks(k);
        }
        if (curwin->text_col + cursorcol >= NCOLS)
            cursorcol = - curwin->text_col;
        if (max_colflg && max_colflg != curwin->rightbar[l0]) {
            poscursor(curwin->max_col - curwin->text_col, l0);
            putch(max_colflg, 0);
        } else
            movecursor(0);
        curwin->rightbar[l0] = max_colflg;
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
    scol = col + curwin->text_col;
    slin = lin + curwin->text_row;    /* screen col, lin */
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
    register int lin, col;

    lin = cursorline;
    col = cursorcol;
    switch (arg) {
    case 0:
        break;
    case CCHOME:                /* home: cursor to line start */
        if (curwksp->coloffset != 0)
            wksp_offset(-curwksp->coloffset);
        col = 0;
        break;
    case CCEND:                 /* home: cursor to line end */
        col = curwin->lastcol[cursorline];
        // TODO: increase offset
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
    case CCMOVERIGHT:           /* move forward */
        ++col;
        break;
    case CCMOVELEFT:            /* backspace */
        --col;
        break;
    case CCTAB:                 /* tab */
        col += curwksp->coloffset;
        col = (col + 4) & ~3;
        col -= curwksp->coloffset;
        break;
    }
    if (col > curwin->text_maxcol)
        col = 0;
    else if (col < 0)
        col = curwin->text_maxcol;

    if (lin < 0)
        lin = curwin->text_maxrow;
    else if (lin > curwin->text_maxrow)
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
    if (curwin->text_col + cursorcol >= NCOLS)
        cursorcol = - curwin->text_col;
    putcha(j);
    if (cursorcol <= 0)
        poscursor(0,
            cursorline < 0 ? 0 :
            cursorline > curwin->text_maxrow ? 0 :
            cursorline);
    movecursor(0);
}

/*
 * Get a parameter.
 * Три типа параметров.
 *         param_type = -1 -- определение области
 *         param_type = 0  -- пустой аргумент
 *         param_type = 1  -- строка.
 *         при использовании макро бывает param_type = -2 - tag defined
 * Возвращается указатель на введенную строку (param_str).
 * Длина возвращается в переменной param_len.
 * Если при очередном вызове param_len не 0, старый param_str
 * освобождается, так что если старый параметр нужен,
 * нужно обнулить param_len.
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

    if (param_len != 0 && param_str != 0)
        free(param_str);
    param_c1 = param_c0 = cursorcol;
    param_r1 = param_r0 = cursorline;
    putch(COCURS,1);
    poscursor(cursorcol,cursorline);
    w = curwin;
back:
    telluser(macro ? "mac: " : "arg: ", 0);
    win_switch(&paramwin);
    poscursor(5, 0);
    do {
        keysym = -1;
        getkeysym();
    } while (keysym == CCBACKSPACE);

    if (macro)
        goto rmac;
    if (MOVECMD(keysym)) {
        telluser("arg: *** cursor defined ***", 0);
        win_switch(w);
        poscursor(param_c0, param_r0);
t0:
        while (MOVECMD(keysym)) {
            movecursor(keysym);
            if (cursorline == param_r0 && cursorcol == param_c0)
                goto back;
            keysym = -1;
            getkeysym();
        }
        if (CTRLCHAR(keysym) && keysym != CCBACKSPACE) {
            if (cursorcol > param_c0)
                param_c1 = cursorcol;
            else
                param_c0 = cursorcol;
            if (cursorline > param_r0)
                param_r1 = cursorline;
            else
                param_r0 = cursorline;
            param_len = 0;
            param_str = NULL;
            param_type = -1;
        } else {
            error("Printing character illegal here");
            keysym = -1;
            getkeysym();
            goto t0;
        }
    } else if (CTRLCHAR(keysym)) {
        param_len = 0;
        param_str = NULL;
        param_type = 0;
    } else {
rmac:
        param_len = pn = 0;
loop:
        c = getkeysym();
        if (pn >= param_len) {
            cp = param_str;
            param_str = salloc(param_len + LPARAM + 1); /* 1 for dechars */
            c1 = param_str;
            c2 = cp;
            for (i=0; i<param_len; ++i)
                *c1++ = *c2++;
            if (param_len)
                free(cp);
            param_len += LPARAM;
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
                if ((param_str[pn] & 0340) == 0) {
                    putch(' ', 0);
                    movecursor(CCMOVELEFT);
                    movecursor(CCMOVELEFT);
                }
                param_str[pn] = 0;
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
        param_str[pn++] = c;
        if (c != 0) {
            if ((c & 0140) == 0){
                putch('^', 0);
                c = c | 0100;
            }
            putch(c, 0);
            keysym = -1;
            goto loop;
        }
        param_type = 1;
    }
    win_switch(w);
    drawlines(param_r0, param_r0);
    poscursor(param_c0, param_r0);
    return (param_str);
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
    if (win->base_row != win->text_row) {
        poscursor(win->base_col, win->base_row);
        for (i = win->base_col; i <= win->max_col; i++)
            putch(TMCH, 0);
    }
    if (vertf) {
        int j;
        for (j = win->base_row + 1; j <= win->max_row - 1; j++) {
            int c = win->leftbar[j - win->base_row - 1];
            if (c != 0) {
                poscursor(win->base_col, j);
                putch(c, 0);
                poscursor(win->max_col, j);
                putch(win->rightbar[j - win->base_row - 1], 0);
            }
        }
    }
    if (win->base_row != win->text_row) {
        poscursor(win->base_col, win->max_row);
        for (i = win->base_col; i <= win->max_col; i++)
            putch(BMCH, 0);
    }
    /* poscursor(win->base_col + 1, win->base_row + 1); */
    win_switch(win);
}

/*
 * Display error message.
 */
void error(msg)
    char *msg;
{
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
        putblanks(paramwin.text_maxcol);
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
void redisplay()
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
        for (i=0; i<curp->text_maxrow+1; i++) {
            curp->firstcol[i] = 0;
            curp->lastcol[i] = 0; /* curwin->text_maxcol;*/
            curp->leftbar[i] = ' ';
            curp->rightbar[i] = ' ';
        }
        win_draw(curp, 0);
        drawlines(0, curp->text_maxrow);
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
