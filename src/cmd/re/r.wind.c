/*
 * Implementing windows.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"

/*
 * Move a current window by nl rows down.
 */
void movew(nl)
    int nl;
{
    register int cc, cl;

    curwksp->toprow += nl;
    if (curwksp->toprow < 0)
        curwksp->toprow = 0;
    cc = cursorcol;
    cl = cursorline - nl;
    drawlines(0, curwin->text_height);
    if (cl < 0 || cl > curwin->text_height) {
        cl = defplline;
        cc = 0;
    }
    poscursor(cc, cl);
}

/*
 * Move a window by nc columns to right.
 */
void movep(nc)
    int nc;
{
    register int cl, cc;

    cl = cursorline;
    cc = cursorcol;
    if ((curwksp->topcol + nc) < 0)
        nc = - curwksp->topcol;
    curwksp->topcol += nc;
    drawlines(0, curwin->text_height);
    cc -= nc;
    if (cc < 0)
        cc = 0;
    else if (cc > curwin->text_width)
        cc = curwin->text_width;
    poscursor(cc, cl);
}

/*
 * Go to line by number.
 */
void gtfcn(number)
    int number;
{
    register int i;

    movew(number - curwksp->toprow - defplline);
    i = number - curwksp->toprow;
    if (i >= 0) {
        if (i > curwin->text_height)
            i = curwin->text_height;
        poscursor(cursorcol, i);
    }
}

/*
 * Scroll window to show a given area.
 * slin - номер строки, на которой был курсор раньше
 * (для стирания отметки курсора)
 * lkey = 0 - не двигать окно, если можно.
 */
void cgoto(ln, col, slin, lkey)
    int ln, col, slin, lkey;
{
    register int lin;

    lin = ln - curwksp->toprow;
    if (lkey || lin < 0 || lin  > curwin->text_height) {
        lkey = -1;
        lin = defplline;
        curwksp->toprow = ln - defplline;
        if (curwksp->toprow < 0) {
            lin += curwksp->toprow;
            curwksp->toprow = 0;
        }
    }
    col -= curwksp->topcol;
    if (col < 0 || col > curwin->text_width) {
        curwksp->topcol += col;
        col = 0;
        lkey = -1;
    }
    if (lkey)
        drawlines(0, curwin->text_height);
    else if (slin >=0)
        drawlines(slin, slin);

    poscursor(col, lin);
}


/*
 * Switch to given window.
 * Compute cursorcol, cursorline for new window.
 */
void win_switch(ww)
    window_t *ww;
{
    register window_t *w = ww;

    cursorcol  -= (w->text_col - curwin->text_col);
    cursorline -= (w->text_row - curwin->text_row);
    curwin = w;
    curwksp = curwin->wksp;
    if (curwksp)
        curfile = curwksp->wfile;
}

/*
 * Create new window.
 * Flag c = 1 when borders enable.
 */
void win_create(ww, cl, cr, lt, lb, c)
    window_t *ww;
    int cl, cr, lt, lb, c;
{
    register int i, size;
    register window_t *w;

    w = ww;
    w->base_col = cl;
    w->max_col = cr;
    w->base_row = lt;
    w->max_row = lb;
    if (c) {
        w->text_col = cl + 1;
        w->text_width = cr - cl - 2;
        w->text_row = lt + 1;
        w->text_height = lb - lt - 2;
    } else {
        w->text_col = cl;
        w->text_width = cr - cl;
        w->text_row = lt;
        w->text_height = lb - lt;
    }

    /* eventually this extra space may not be needed */
    w->wksp = (workspace_t*) salloc(sizeof(workspace_t));
    w->altwksp = (workspace_t*) salloc(sizeof(workspace_t));
    size = NLINES - lt + 1;
    w->firstcol = (unsigned char*) salloc(size);
    for (i=0; i<size; i++)
        w->firstcol[i] = w->text_width + 1;
    w->lastcol = (unsigned char*) salloc(size);
    w->leftbar = salloc(size);
    w->rightbar = salloc(size);
    w->wksp->cursegm = file[2].chain; /* "#" - так как он всегда есть */
}


/*
 * Make new window.
 */
void win_open(file)
    char *file;
{
    register window_t *oldwin, *win;
    char horiz;             /* 1 - если горизонтально */
    register int i;
    int winnum;

    /* Есть ли место */
    if (nwinlist >= MAXWINLIST) {
        error("Can't make any more windows.");
        return;
    }
    if (cursorcol == 0 && cursorline > 0
        && cursorline < curwin->text_height) horiz = 1;
    else if (cursorline == 0 && cursorcol > 0
        && cursorcol < curwin->text_width-1) horiz = 0;
    else {
        error("Can't put a window there.");
        return;
    }
    oldwin = curwin;
    win = winlist[nwinlist++] = (window_t*) salloc(sizeof(window_t));

    /* Find a win number */
    for (winnum=0; winlist[winnum] != curwin; winnum++);
    win->prevwinnum = winnum;
    oldwin->wksp->cursorcol = oldwin->altwksp->cursorcol = 0;
    oldwin->wksp->cursorrow = oldwin->altwksp->cursorrow = 0;
    if (horiz) {
        win_create(win, oldwin->base_col, oldwin->max_col,
            oldwin->base_row + cursorline + 1, oldwin->max_row, 1);
        oldwin->max_row = oldwin->base_row + cursorline + 1;
        oldwin->text_height = cursorline - 1;
        for (i=0; i <= win->text_height; i++) {
            win->firstcol[i] = oldwin->firstcol[i + cursorline + 1];
            win->lastcol[i] = oldwin->lastcol[i + cursorline + 1];
        }
    } else {
        win_create(win, oldwin->base_col + cursorcol + 2, oldwin->max_col,
            oldwin->base_row, oldwin->max_row, 1);
        oldwin->max_col = oldwin->base_col + cursorcol + 1;
        oldwin->text_width = cursorcol - 1;
        for (i=0; i <= win->text_height; i++) {
            if (oldwin->lastcol[i] > oldwin->text_width + 1) {
                win->firstcol[i] = 0;
                win->lastcol[i] = oldwin->lastcol[i] - cursorcol - 2;
                oldwin->lastcol[i] = oldwin->text_width + 1;
                oldwin->rightbar[i] = MRMCH;
            }
        }
    }
    win_switch(win);
    defplline = defmiline = (win->max_row - win->base_row)/ 4 + 1;
    if (editfile (file, 0, 0, 1, 1) <= 0 &&
        editfile (deffile, 0, 0, 0, 1) <= 0)
        error("Default file gone: notify sys admin.");
    win_draw(oldwin, 1);
    win_draw(win, 1);
    poscursor(0, 0);
}

/*
 * Remove last window.
 */
void win_remove()
{
    int j, pwinnum;
    register int i;
    register window_t *win, *pwin;

    if (nwinlist == 1) {
        error ("Can't remove the last window.");
        return;
    }
    win = winlist[--nwinlist];
    pwinnum = win->prevwinnum;
    pwin = winlist[pwinnum];
    if (pwin->max_row != win->max_row) {
        /* Vertical */
        pwin->firstcol[j = pwin->text_height+1] = 0;
        pwin->lastcol[j++] = pwin->text_width+1;
        for (i=0; i<=win->text_height; i++) {
            pwin->firstcol[j+i] = win->firstcol[i];
            pwin->lastcol[j+i] = win->lastcol[i];
        }
        pwin->max_row = win->max_row;
        pwin->text_height = pwin->max_row - pwin->base_row - 2;
    } else {
        /* Горизонтальное */
        for (i=0; i<=pwin->text_height; i++) {
            pwin->lastcol[i] = win->lastcol[i] +
                win->base_col - pwin->base_col;
            if (pwin->firstcol[i] > pwin->text_width)
                pwin->firstcol[i] = pwin->text_width;
        }
        pwin->max_col = win->max_col;
        pwin->text_width = pwin->max_col - pwin->base_col - 2;
    }
    win_draw(pwin, 1);
    win_goto(pwinnum);
    drawlines(0, curwin->text_height);
    poscursor(0, 0);
    DEBUGCHECK;
    free(win->firstcol);
    free(win->lastcol);
    free(win->leftbar);
    free(win->rightbar);
    free((char *)win->wksp);
    free((char *)win->altwksp);
    free((char *)win);
    DEBUGCHECK;
}

/*
 * Switch to another window by number.
 * When number is -1, select a next window after the curwin.
 */
void win_goto(winnum)
    int winnum;
{
    register window_t *oldwin, *win;

    oldwin = curwin;
    oldwin->wksp->cursorcol = cursorcol;
    oldwin->wksp->cursorrow = cursorline;
    if (winnum < 0) {
        /* Find a window next to the current one. */
        for (winnum=0; winnum<nwinlist; winnum++) {
            if (oldwin == winlist[winnum]) {
                winnum++;
                break;
            }
        }
        if (winnum >= nwinlist)
            winnum = 0;
    }
    win = winlist[winnum];
    if (win == oldwin)
        return;

    /* win_draw(oldwin, win); */
    win_switch(win);
    defplline = defmiline = (win->max_row - win->base_row) / 4 + 1;
    poscursor(curwin->wksp->cursorcol, curwin->wksp->cursorrow);
}

/*
 * Redisplay changed windows after lines in file shifted.
 *
 * w - Рабочее пространство;
 * fn - индекс измененного файла;
 * from, to, delta - диапазон изменившихся строк и
 * общее изменение числа строк в нем
 *
 * Делается следующее:
 *  1. Перевыдаются все окна, в которые попал измененный участок;
 *  2. Получают новые ссылки на segm в тех рабочих пространствах,
 *     которые изменялись (из за breaksegm они могли измениться);
 *  3. Пересчитываются текущие номера строк в рабочих областях, если они
 *     отображают хвосты изменившихся файлов.
 */
void wksp_redraw(w, fn, from, to, delta)
    workspace_t *w;
    int from, to, delta, fn;
{
    register workspace_t *tw;
    register int i, j;
    int k, l, m;
    window_t *owin;

    for (i = 0; i < nwinlist; i++) {
        tw = winlist[i]->altwksp;
        if (tw->wfile == fn && tw != w) {
            /* Исправим указатель на segm. */
            tw->line = tw->segmline = 0;
            tw->cursegm = file[fn].chain;

            /* Исправить номер строки */
            j = delta >= 0 ? to : from;
            if (tw->toprow > j)
                tw->toprow += delta;
        }
        tw = winlist[i]->wksp;
        if (tw->wfile == fn && tw != w) {
            /* Исправляем указатель на segm. */
            tw->line = tw->segmline = 0;
            tw->cursegm = file[fn].chain;

            /* Исправляем номер строки и позиции на экране */
            j = delta >= 0 ? to : from;
            if (tw->toprow > j)
                tw->toprow += delta;

            /* Если изменилось, перевыдать окно */
            j = (from > tw->toprow ? from : tw->toprow);
            k = tw->toprow + winlist[i]->text_height;
            if (k > to)
                k = to;
            if (j <= k) {
                owin = curwin;
                l = cursorcol;
                m = cursorline;
                win_switch(winlist[i]);
                drawlines(j - tw->toprow,
                    delta == 0 ? k - tw->toprow : winlist[i]->text_height);
                win_switch(owin);
                poscursor(l, m);
            }
        }
    }
}
