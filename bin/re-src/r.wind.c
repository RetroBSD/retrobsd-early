/*
 * Редактор RED. ИАЭ им. И.В. Курчатова, ОС ДЕМОС
 * r.wind.c - работа с окнами
 * $Header: /home/sergev/Project/vak-opensource/trunk/relcom/nred/RCS/r.wind.c,v 3.1 1986/04/20 23:43:41 alex Exp $
 * $Log: r.wind.c,v $
 * Revision 3.1  1986/04/20 23:43:41  alex
 */
#include "r.defs.h"

/*
 * Move a current window by nl rows down.
 */
void movew(nl)
    int nl;
{
    register int cc, cl;

    curwksp->ulhclno += nl;
    if (curwksp->ulhclno < 0)
        curwksp->ulhclno = 0;
    cc = cursorcol;
    cl = cursorline - nl;
    putup(0, curport->btext);
    if (cl < 0 || cl > curport->btext) {
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
    if ((curwksp->ulhccno + nc) < 0)
        nc = - curwksp->ulhccno;
    curwksp->ulhccno += nc;
    putup(0, curport->btext);
    cc -= nc;
    if (cc < curport->ledit)
        cc = curport->ledit;
    else if (cc > curport->redit)
        cc = curport->redit;
    poscursor(cc, cl);
}

/*
 * Go to line by number.
 */
void gtfcn(number)
    int number;
{
    register int i;

    movew(number - curwksp->ulhclno - defplline);
    i = number - curwksp->ulhclno;
    if (i >= 0) {
        if (i > curport->btext)
            i = curport->btext;
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

    lin = ln - curwksp->ulhclno;
    if (lkey || lin < 0 || lin  > curport->btext) {
        lkey = -1;
        lin = defplline;
        curwksp->ulhclno = ln - defplline;
        if (curwksp->ulhclno < 0) {
            lin += curwksp->ulhclno;
            curwksp->ulhclno = 0;
        }
    }
    col -= curwksp->ulhccno;
    if (col < 0 || col > curport->rtext) {
        curwksp->ulhccno += col;
        col = 0;
        lkey = -1;
    }
    if (lkey)
        putup(0, curport->btext);
    else if (slin >=0)
        putup(slin, slin);

    poscursor(col, lin);
}


/*
 * Switch to given window.
 * Compute cursorcol, cursorline for new window.
 */
void switchport(ww)
    struct viewport *ww;
{
    register struct viewport *w = ww;

    cursorcol  -= (w->ltext - curport->ltext);
    cursorline -= (w->ttext - curport->ttext);
    curport = w;
    if (curwksp = curport->wksp)
        curfile = curwksp->wfile;
}

/*
 * setupviewport() -
 *  создать новое окно
 *  c = 1 если окно имеет границы по краям
 */
setupviewport(ww,cl,cr,lt,lb,c)
struct viewport *ww;
int cl,cr,lt,lb,c;
{
    register int i,size;
    register struct viewport *w;
    w = ww;
    w->lmarg = cl;
    w->rmarg = cr;
    w->tmarg = lt;
    w->bmarg = lb;
    if (c)
    {
        w->ltext = cl + 1;
        w->rtext = cr - cl - 2;
        w->ttext = lt + 1;
        w->btext = lb - lt - 2;
    }
    else
    {
        w->ltext = cl;
        w->rtext = cr - cl;
        w->ttext = lt;
        w->btext = lb - lt;
    }
    w->ledit = w->tedit = 0;
    w->redit = w->rtext;
    w->bedit = w->btext;
    /* eventually this extra space may not be needed */
    w->wksp = (struct workspace *)salloc(SWKSP);
    w->altwksp = (struct workspace *)salloc(SWKSP);
    w->firstcol = salloc(size = NLINES - lt + 1);
    for (i=0;i<size;i++) (w->firstcol)[i] = w->rtext + 1;
    w->lastcol = salloc(size);
    w->lmchars = salloc(size);
    w->rmchars = salloc(size);
    w->wksp->curfsd = openfsds[2]; /* "#" - так как он всегда есть */
}


/*
 * Make new window.
 */
void makeport(file)
    char *file;
{
    register struct viewport *oldport, *newport;
    char horiz;             /* 1 - если горизонтально */
    register int i;
    int portnum;

    /* Есть ли место */
    if (nportlist >= MAXPORTLIST) {
        error("Can't make any more windows.");
        return;
    }
    if (cursorcol == 0 && cursorline > 0
        && cursorline < curport->btext) horiz = 1;
    else if (cursorline == 0 && cursorcol > 0
        && cursorcol < curport->rtext-1) horiz = 0;
    else {
        error("Can't put a window there.");
        return;
    }
    oldport = curport;
    newport = portlist[nportlist++] = (struct viewport *) salloc(SVIEWPORT);

    /* Find a port number */
    for (portnum=0; portlist[portnum] != curport; portnum++);
    newport->prevport = portnum;
    oldport->wksp->ccol = oldport->altwksp->ccol = 0;
    oldport->wksp->crow = oldport->altwksp->crow = 0;
    if (horiz) {
        setupviewport(newport, oldport->lmarg,
            oldport->rmarg,
            oldport->tmarg + cursorline + 1,
            oldport->bmarg, 1);
        oldport->bmarg = oldport->tmarg + cursorline + 1;
        oldport->btext = oldport->bedit = cursorline - 1;
        for (i=0; i <= newport->btext; i++) {
            newport->firstcol[i] =
                oldport->firstcol[i + cursorline + 1];
            newport->lastcol[i] =
                oldport->lastcol[i + cursorline + 1];
        }
    } else {
        setupviewport(newport, oldport->lmarg + cursorcol + 2,
            oldport->rmarg,
            oldport->tmarg,
            oldport->bmarg, 1);
        oldport->rmarg = oldport->lmarg + cursorcol + 1;
        oldport->rtext = oldport->redit = cursorcol - 1;
        for (i=0; i <= newport->btext; i++) {
            if (oldport->lastcol[i] > oldport->rtext + 1) {
                newport->firstcol[i] = 0;
                newport->lastcol[i] =
                    oldport->lastcol[i] - cursorcol - 2;
                oldport->lastcol[i] = oldport->rtext + 1;
                oldport->rmchars[i] = MRMCH;
            }
        }
    }
    switchport(newport);
    defplline = defmiline = (newport->bmarg - newport->tmarg)/ 4 + 1;
    if (editfile (file, 0, 0, 1, 1) <= 0 &&
        editfile (deffile, 0, 0, 0, 1) <= 0)
        error("Default file gone: notify sys admin.");
    drawport(oldport, 1);
    drawport(newport, 1);
    poscursor(0, 0);
}

/*
 * Remove last window.
 */
void removeport()
{
    int j, pnum;
    register int i;
    register struct viewport *theport, *pport;

    if (nportlist == 1) {
        error ("Can't remove remaining port.");
        return;
    }
    pport = portlist[pnum = (theport = portlist[--nportlist])->prevport];
    if (pport->bmarg != theport->bmarg) {
        /* Vertical */
        pport->firstcol[j = pport->btext+1] = 0;
        pport->lastcol[j++] = pport->rtext+1;
        for (i=0; i<=theport->btext; i++) {
            pport->firstcol[j+i] = theport->firstcol[i];
            pport->lastcol[j+i] = theport->lastcol[i];
        }
        pport->bmarg = theport->bmarg;
        pport->bedit = pport->btext = pport->bmarg - pport->tmarg - 2;
    } else {
        /* Горизонтальное */
        for (i=0; i<=pport->btext; i++) {
            pport->lastcol[i] = theport->lastcol[i] +
                theport->lmarg - pport->lmarg;
            if (pport->firstcol[i] > pport->rtext)
                pport->firstcol[i] = pport->rtext;
        }
        pport->rmarg = theport->rmarg;
        pport->redit = pport->rtext = pport->rmarg - pport->lmarg - 2;
    }
    drawport(pport, 1);
    chgport(pnum);
    putup(0, curport->btext);
    poscursor(0, 0);
    DEBUGCHECK;
    free(theport->firstcol);
    free(theport->lastcol);
    free(theport->lmchars);
    free(theport->rmchars);
    free((char *)theport->wksp);
    free((char *)theport->altwksp);
    free((char *)theport);
    DEBUGCHECK;
}

/*
 * Switch to another window.
 */
void chgport(portnum)
    int portnum;
{
    register struct viewport *oldport, *newport;

    oldport = curport;
    if (portnum < 0) {
        /* Find portnum. */
        for (portnum=0; portnum<nportlist &&
            oldport != portlist[portnum++]; );
    }
    oldport->wksp->ccol = cursorcol;
    oldport->wksp->crow = cursorline;
    newport = portlist[portnum % nportlist];
    if (newport == oldport)
        return;
    /* drawport(oldport, newport); */
    switchport(newport);
    defplline = defmiline = (newport->bmarg - newport->tmarg) / 4 + 1;
    poscursor(curport->wksp->ccol, curport->wksp->crow);
}

/*
 * redisplay(w,w->wfile,from,to,delta) -
 * Перевыдать изменившиеся окна после сдвига строк в файле.
 * w - Рабочее пространство;
 * fn - имя измененного файла;
 * from, to, delta - диапазон изменившихся строк и
 * общее изменение числа строк в нем
 *
 * Делается следующее:
 *  1. Перевыдаются все окна, в которые попал измененный участок;
 *  2. Получают новые ссылки на fsd в тех рабочих пространствах,
 *     которые изменялись (из за breakfsd они могли измениться);
 *  3. Пересчитываются текущие номера строк в рабочих областях, если они
 *     отображают хвосты изменившихся файлов.
 */
redisplay(w,fn,from,to,delta)
struct workspace *w;
int from,to,delta,fn;
{
    register struct workspace *tw;
    register int i,j;
    int k,l,m;
    struct viewport *oport;
    for (i = 0; i < nportlist; i++)
    {
        if ((tw = portlist[i]->altwksp)->wfile == fn && tw != w)
        {
            /* Исправим указатель на fsd. */
            tw->curlno = tw->curflno = 0;
            tw->curfsd = openfsds[fn];
            /* Исправить номер строки */
            j = delta >= 0 ? to : from;
            if (tw->ulhclno > j) tw->ulhclno += delta;
        }
        if ((tw = portlist[i]->wksp)->wfile == fn && tw != w)
        {
            /* Исправляем указатель на fsd. */
            tw->curlno = tw->curflno = 0;
            tw->curfsd = openfsds[fn];
            /* Исправляем номер строки и позиции на экране */
            j = delta >= 0 ? to : from;
            if (tw->ulhclno > j) tw->ulhclno += delta;
            /* Если изменилось, перевыдать окно */
            j = (from > tw->ulhclno ? from : tw->ulhclno);
            if ((k =  tw->ulhclno + portlist[i]->btext ) > to) k = to;
            if (j <= k)
            {
                oport = curport;
                l = cursorcol;
                m = cursorline;
                switchport(portlist[i]);
                putup(j - tw->ulhclno, delta == 0 ?
                k - tw->ulhclno : portlist[i]->btext);
                switchport(oport);
                poscursor(l,m);
            }
        }
    }
}
