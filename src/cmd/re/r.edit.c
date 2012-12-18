/*
 * Functions for manipulating file contents.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"
#include <signal.h>
#include <sys/wait.h>

#define NBYMAX 150      /* Макс. размер байтов для data, +1 */

static int charskl;     /* Положение символа (для chars) */

/*
 * Настройка программы чтения из файла
 */
#define CHARBUFSZ 512

static int charsoff;    /* position of next read from charsfi */
static int charsfi, charsi, charsn;

static char charsbuf[CHARBUFSZ];

/*
 * Настроить программу чтения символов "char" на
 * файл "fi" с позиции offset = сдвиг.
 */
static void charsin(fi, offset)
    int fi, offset;
{
    if (fi <= 0) {
        charsfi = fi;
        return;
    }
    if (charsfi == fi && offset <= charsoff && offset >= charsoff - charsn) {
        /* New offset is inside the last buffer. */
        charsi = charsn + offset - charsoff;
    } else {
        lseek(fi, (off_t) offset, 0);
        charsoff = offset;
        charsi = 0;
        charsn = 0;
    }
    cline_len = 0;
    charsfi = fi;
    charskl = charsoff + charsi - charsn;
}

/*
 * Преобразование строки из внешней во внутреннюю форму.
 * *si - Текущий входной символ
 * *(se-1) - Последний ранее считанный входной символ
 * *so - текущее место в строке вывода
 *  no - текущая колонка на выводе, +1 (1, если пусто)
 *  mo - макс. номер колонки в олученной строке
 *
 * Код ответа:
 *       0 - end of line
 *       1 - end of input string
 *       2 - the output line overflow
 */
static int exinss(si, se, so, no, mo)
    char **si, *se, **so;
    int *no, mo;
{
    register char *st, *sf;
    register unsigned sy;
    int s1, n, i;
    int ir = 0;

    sf = *si;
    st = *so;
    /*==== se +=1; ====*/
    n = *no;
    /* main loop */
    while ((sy = *sf) != '\n' && sf != se) {
        if (n+2 > mo) {
            ir = 2;
            break;
        }
        if (sy == '\11') {
            i = (n & ~7) + 8;
            if (i > mo) {
                ir = 2;
                break;
            }
            for (; n<i; n++)
                *st++ = ' ';
            goto next;
        }
        /* DEL */
        if (sy == '\177') {
            *st++ = COCURS;
            *st++ = '#';
            n += 2;
            goto next;
        }
        /* CNTRL X */
        s1 = sy & 0340;
        if (s1 == 0) {
            *st++ = COCURS;
            *st++ = sy | 0100;
            n += 2;
            goto next;
        }
        /* letters and sim. */
        if (s1 != 040) {
            if (s1 == 0200 || s1 == 0240) {
                /* \377 */
                if (n+4 > mo) {
                    ir = 2;
                    break;
                }
                *st++ = COCURS;
                *st++ = ((sy >> 6) & 3) + '0';
                *st++ = ((sy >> 3) & 7) + '0';
                *st++ = (sy & 7) + '0';
                n += 4;
                goto next;
            }
        }
        *st++ = sy;
        n++;
next:
        sf++;
    }
    if (ir == 0) {
        if (sy == '\n')
            sf++;
        else
            ir = 1;
    }
    *si = sf;
    *so = st;
    *no = n;
    *st = 0;
    return ir;
}

/*
 * Main function for reading next line from file.
 * Управляется глобальными переменными charsoff и charsfi.
 * (сдвиг в файле и дескриптор).
 * Возвращает: последний символ либо -1 при конце файла.
 * Если flg = 1 - заполняется строка "cline".
 * Длина строки возвращается в cline_len, с учетом символа конца.
 * Строка оканчивается символом LINE FEED или -1, если конец файла.
 * ============================================================
 * При вводе строка разворачивается из файловой в экранную форму.
 * При этом заменяются символы табуляции, неграфические символы,
 * а в режимах "lcase" или "lat" часть символов преобразуются в пару
 * символов.
 * Обратное преобразование (из экранной в файловую форму)
 * делается функцией "dechar".
 *
 * Если flg = 0 - строка не формируется, этот режим служит для
 * составления цепи дескрипторов.
 */
int chars(flg)
    int flg;
{
    register char *c,*se;
    register int ko;
    char *si, *so;

    if (charsfi <= 0) {
        if (cline_max == 0)
            excline(1);
        cline_len = 1;
        cline[0] = '\n';
        return ('\n');
    }
    if (! cline)
        excline(0);
    so = cline;
    ko = (charsi >= charsn) ? 1 : 2;
    do {
        if (ko == 1) {
            charsi = 0;
            charsn = read(charsfi, charsbuf, CHARBUFSZ);
            if (charsn < 0) {
                error("Read Error");
                charsn = 0;
            }
            charsoff += charsn;
        }
        if (charsn <= charsi) {
            ko = 1;
            break;
        } /* read buf empty */

        si = charsbuf + charsi;
        se = charsbuf + charsn;
        if (! flg) {
            c = si;
            while (*c++ != '\n' && c != se);
            charsi = c - charsbuf;
            ko = (*(c-1) == '\n') ? 0 : 1;
            continue;
        }
        while (! cline_max || (ko = exinss(&si, se, &so, &cline_len, cline_max)) == 2) {
            cline_len = so - cline;
            excline(0);
            so = cline + cline_len;
        }
        charsi = si - charsbuf;
    } while (ko);

    /* ko=0 - '\n', 1 - end of file */
    charskl = charsoff + charsi - charsn;
    if (ko == 1)
        charsfi = 0;

    /* Убрать хвостовые пробелы */
    if (flg && so) {
        *so = ' ';
        c = so;
        while (*c == ' ' && c-- != cline);
        *++c = '\n';
        cline_len = (c - cline) + 1;
    }
    return ko ? -1 : '\n';
}

/*
 * Разложить файл на segm-цепь.
 * работает с текущего места в файле chan
 */
static segment_t *temp2segm(chan)
    int chan;
{
    register segment_t *thissegm = 0, *lastsegm = 0;
    segment_t *firstsegm = 0;
    register int nby = 0;
    char *bpt;
    int c;
    char fby[NBYMAX+1];
    int i, lct, nl = 0, sl, kl;

    /* основной цикл. c - очередной символ, но -1 означает
     * конец файла, а -2 - вход в цикл.
     */
    c = -2;
    sl = 0;
    for (;;) {
        if ((c < 0) || nby >= NBYMAX || nl == FSDMAXL) {
            if (c != -2) {
                lastsegm = thissegm;
                thissegm = (segment_t *)salloc(nby + sizeof_segment_t);
                if (firstsegm == 0)
                    firstsegm = thissegm;
                else
                    lastsegm->next = thissegm;
                thissegm->prev = lastsegm;
                thissegm->next = 0;
                thissegm->nlines = nl;
                file[chan].nlines += nl;
                thissegm->fdesc = chan;
                thissegm->seek = sl;
                bpt = &(thissegm->data);
                for (i=0; i<nby; ++i)
                    *bpt++ = fby[i];
            }
            if (c == -1) {
                /* Поместим блок конца и выйдем */
                thissegm->next = lastsegm = (segment_t *)salloc(sizeof_segment_t);
                lastsegm->prev = thissegm;
                return (firstsegm);
            }
            sl = charskl;
            nl = nby = lct = 0;
        }
        kl = charskl;
        c = chars(0);
        lct = charskl - kl;
        if (c != -1 || lct) {
            if (lct > 127) {
                fby[nby++] = (lct / 128) | 0200;
                lct = lct % 128;
            }
            fby[nby++] = lct;
            ++nl;
        }
    }
}

/*
 * Create a file descriptor list for a file.
 */
segment_t *file2segm(fname)
    int fname;
{
    charsin(fname, 0);
    return temp2segm(fname);
}

/*
 * Conversion of line from internal to external form.
 * Преобразование делается прямо в строке line.
 * n - длина строки. В строке должно быть не менее n+1 символов,
 * так как в конец полученной строки пишется '\n'.
 * Возвращает число символов в полученной строке.
 * Хвостовые пробелы исключаются.
 */
int dechars(line, n)
    char *line;
    int n;
{
    register char *fm,*to;  /* pointers for move */
    register unsigned cc;   /* current character */
    int lnb;       /* 1 + last non-blank col */
    int cn;                 /* col number */
    int i, j;
    line[n] = '\n';
    fm = to = line;
    cn = -1;
    lnb = 0;
    while ((cc = *fm++) != '\n') {
        cn++;
        if (cc != ' ') {
            while (++lnb <= cn)
                *to++ = ' ';
            /* расшифровка символов */
            if (cc == COCURS) {
                if (*fm >= '@' || (unsigned)*fm > 0200) {
                    *to++ = *fm++ & 037;
                    continue;
                }
                if (*fm == '#') {
                    *to++ = '\177';
                    fm++;
                    continue;
                }
                /* <esc>XXX */
                i = 0;
                j = 0;
                while (j++ <3 && (*fm >= '0' && *fm <= '7'))
                    i = (i<<3) + (*fm++ - '0');
                *to++ = i & 0377;
                continue;
            }
            *to++ = cc;
        } /* while - continue - not skip */
    }
    *to++ = '\n';
    return (to - line);
}

/*
 * excline(n) -
 * расширить массив cline.
 * cline содержит текущую строку, причем
 * cline_max - содержит максимальную длину массива cline;
 * cline_len - длина текущей строки.
 */
void excline(n)
    int n;
{
    register int j;
    register char *tmp;

    j = cline_max + cline_incr;
    if (j < n)
        j = n;
    tmp = salloc(j+1);
    cline_incr += cline_incr >> 1;
    while (--cline_max >= 0)
        tmp[cline_max] = cline[cline_max];
    cline_max = j;
    if (cline != 0)
        free(cline);
    cline = tmp;
}

/*
 * putbks(col,n) -
 * вставить n пробелов в колонке col текущей строки
 */
void putbks(col, n)
    int col, n;
{
    register int i;

    if (n <= 0)
        return;
    if (col > cline_len-1) {
        n += col - (cline_len-1);
        col = cline_len-1;
    }
    if (cline_max <= (cline_len += n))
        excline(cline_len);
    for (i = cline_len - (n + 1); i >= col; i--)
        cline[i + n] = cline[i];
    for (i = col + n - 1; i >= col; i--)
        cline[i] = ' ';
}

/*
 * Установить файл и смещение для ввода строки nlo
 * из рабочего пространства wksp.
 * После этого chars(0) считает требуемую строку.
 * Код ответа = 1, если такой строки нет, 0 если ОК
 */
int wksp_seek(wksp, lno)
    workspace_t *wksp;
    int lno;
{
    register char *cp;
    int i;
    register int j, l;

    /* 1. Получим segm, в котором "сидит" данная строка */
    if (wksp_position(wksp, lno))
        return (1);

    /* Теперь вычислим смещение */
    l = wksp->cursegm->seek;
    i = lno - wksp->segmline;
    cp = &(wksp->cursegm->data);
    while (i-- != 0) {
        if ((j = *(cp++)) & 0200) {
            l += 128*(j&0177);
            j = *(cp++);
        }
        l += j;
    }
    charsin(wksp->cursegm->fdesc, l);
    return (0);
}

/*
 * Set workspace position to segm with given line.
 */
int wksp_position(wk,lno)
    workspace_t *wk;
    int lno;
{
    register workspace_t *wksp;

    wksp = wk;
    if (lno < 0)
        fatal("Wposit neg arg");
    while (lno >= (wksp->segmline + wksp->cursegm->nlines)) {
        if (wksp->cursegm->fdesc == 0) {
            wksp->line = wksp->segmline;
            return (1);
        }
        wksp->segmline += wksp->cursegm->nlines;
        wksp->cursegm = wksp->cursegm->next;
    }
    while (lno < wksp->segmline) {
        if ((wksp->cursegm = wksp->cursegm->prev) == 0)
            fatal("Wposit 0 prev");
        wksp->segmline -= wksp->cursegm->nlines;
    }
    if (wksp->segmline < 0) fatal("WPOSIT LINE CT LOST");
    wksp->line = lno;
    return 0;
}

/*
 * Switch to alternative file.
 */
void switchfile()
{
    if (curwin->altwksp->wfile == 0) {
        if (editfile(deffile, 0, 0, 0, 1) <= 0)
            error("Default file gone: notify sys admin.");
        return;
    }
    wksp_switch();
    drawlines(0, curwin->text_height);
    poscursor(curwksp->cursorcol, curwksp->cursorrow);
}

/*
 * Switch to alternative workspace.
 */
void wksp_switch()
{
    register workspace_t *oldwksp;

    curwksp->cursorcol = cursorcol;
    curwksp->cursorrow = cursorline;

    oldwksp = curwksp;
    curwksp = curwin->altwksp;
    curfile = curwksp->wfile;

    curwin->wksp = curwksp;
    curwin->altwksp = oldwksp;
}

/*
 * создай дескриптор n пустых строк
 */
static segment_t *blanklines(n)
    int n;
{
    int i;
    register segment_t *f,*g;
    register char *c;

    f = (segment_t *)salloc(sizeof_segment_t);
    while (n) {
        i = n > FSDMAXL ? FSDMAXL : n;
        g = (segment_t *)salloc(sizeof_segment_t + i);
        g->next = f;
        f->prev = g;
        g->nlines = i;
        g->fdesc = -1;
        c = &g->data;
        n -= i;
        while (i--)
            *c++ = 1;
        f = g;
    }
    return (f);
}

/*
 * разломать segm по строке n в пространстве
 * w. line = segmline при возврате, и cursegm указывает на первую
 * строку после разрыва (которая может быть строкой из конечного
 * блока). Исходный segm остается, возможно, с неиспользуемым
 * остатком списка длин.
 * Если функция применяется к концу файла, текущая
 * позиция остается в конце файла, на конечном блоке.
 * Если указанная строка выходит за пределы файла, добавляются
 * описатели пустых строк (с каналом -1).
 * Если "reall=1" то блок перед разрывом переразмещается в памяти с
 * целью экономии места.
 * ВНИМАНИЕ: breaksegm может нарушить корректность указателей в  workspace.
 * Поэтому после всех операций нужно вызывать "wksp_redraw".
 */
static int breaksegm(w, n, reall)
    workspace_t *w;
    int n, reall;
{
    int nby, i, j, jj, k, lfb0;
    register segment_t *f,*ff;
    segment_t *fn;
    register char *c;
    char *cc;
    off_t offs;

    DEBUGCHECK;
    if (wksp_position(w, n)) {
        f = w->cursegm;
        ff = f->prev;
        free((char *)f);
        fn = blanklines(n - w->line);
        w->cursegm = fn;
        fn->prev = ff;
        if (ff)
            ff->next = fn;
        else
            file[w->wfile].chain = fn;
        wksp_position(w, n);
        return (1);
    }
    f = w->cursegm;
    cc = c = &f->data;
    offs = 0;
    ff = f;
    nby = n - w->segmline;
    if (nby != 0) {
        /* get down to the nth line */
        for (i=0; i<nby; i++) {
            j = *c++;
            if (j & 0200) {
                offs += 128 * (j & 0177);
                j = *c++;
            }
            offs += j;
        }
        /* now make a new segm from the remainder of f */
        i = j = jj = f -> nlines - nby; /* number of lines in new segm */
        lfb0 = c - cc;
        cc = c;
        while (--i >= 0) {
            if (*cc++ < 0) {
                j++;
                cc++;
            }
        }
        ff = (segment_t *)salloc(sizeof_segment_t + j);
        ff->nlines = jj;
        ff->fdesc = f->fdesc;
        offs += f->seek;
        ff->seek = offs;
        cc = &ff->data;
        for (k=0; k<jj; k++)
            if ((*cc++ = *c++) < 0)
                *cc++ = *c++;
        if ((ff->next = f->next))
            ff->next->prev = ff;
        ff->prev = f;
        f->next = ff;
        f->nlines = nby;
        if (reall && jj > 4 && f->prev) {
            ff = (segment_t *)salloc(sizeof_segment_t+lfb0);
            *ff = *f;
            c = &(ff->data);
            cc = &(f->data);
            while (lfb0--) {
                *c++ = *cc++;
            }
            ff->prev->next = ff->next->prev = ff;
            free((char*) f);
            f = ff;
            ff = f->next;
        }
    }
    w->cursegm = ff;
    w->segmline = n;
    DEBUGCHECK;
    return (0);
}

/*
 * программа пытается слить несколько segm - блоков в один
 * для экономии памяти.
 * делается попытка слить в один блок w->cursegm->prev и
 * w->cursegm, если они описывают смежную информацию.
 */
static int catsegm(w)
    workspace_t *w;
{
    register segment_t *f0, *f;
    segment_t *f2;
    register char *c;
    char *cc;
    int i, j, l0=0, l1=0, lb0=0, lb1, dl, nl0, nl1, fd0, kod = 0;
    /* l0,  l1:  число байтов в участке файла, описываемом f0, f;
     * lb0, lb1: длина описателя длин в segm;
     * nl0, nl1: число строк в segm */

    f = w->cursegm;
    if ((f0 = f->prev) == 0) {
        file[w->wfile].chain = f;
        return(0);
    }
    f0->next = f;
    fd0 = f0->fdesc;
    nl0 = f0->nlines;
    while (fd0>0 && fd0==f->fdesc && (nl0+(nl1=f->nlines)< FSDMAXL)) {
        dl = f->seek - f0->seek;
        /*  подсчитываем длину блока, eсли она неизвестна */
        if (l0==0) {
            i = nl0;
            cc = c = &f0->data;
            while(i--) {
                if ((j = *c++) & 0200)
                    j = (j & 0177) * 128 + *c++;
                l0 += j;
            }
            lb0 = c - cc;
        }
        if (dl != l0)
            return(kod);
        /* сливаем два segm  и пытаемся повторить */
        i = nl1;
        cc = c = &(f0->data);
        while (i--) {
            if ((j = *c++) & 0200)
                j = (j & 0177) * 128 + *c++;
            l1 += j;
        }
        lb1 = c - cc;
        f2 = f;
        f = (segment_t*) salloc(sizeof_segment_t + lb0 + lb1);
        *f = *f0;
        f->next = f2->next;
        w->cursegm = f;
        w->segmline -= nl0;
        nl0 = f->nlines = nl0 + nl1;
        c = &(f->data);
        i =lb0;
        cc = &(f0->data);
        while (i--)
            *c++ = *cc++;
        i = lb1;
        cc = &(f2->data);
        while (i--)
            *c++ = *cc++;
        lb0 += lb1;
        l0 += l1;
        kod = 1;
        free((char *)f2);
        free((char *)f0);
        f->next->prev = f;
        f0 = f->prev;
        if (f0)
            f0->next = f;
        else
            file[w->wfile].chain = f;
        f0 = f;
        f = f0->next;
    }
    return(kod);
}

/*
 * Вставить описатель f в файл, описываемый wksp,
 * перед строкой at.
 * Вызывающая программа должна вызвать wksp_redraw с
 * соответствующими аргументами.
 */
static void insert(wksp, f, at)
    workspace_t *wksp;
    segment_t *f;
    int at;
{
    register segment_t *w0, *wf, *ff;

    DEBUGCHECK;
    ff = f;
    while (ff->next->fdesc) {
        ff = ff->next;
    }
    breaksegm(wksp, at, 1);
    wf = wksp->cursegm;
    w0 = wf->prev;
    free((char *)ff->next);
    ff->next = wf;
    wf->prev = ff;
    f->prev = w0;
    wksp->cursegm = f;
    wksp->line = wksp->segmline = at;
    if (file[wksp->wfile].writable)
        file[wksp->wfile].writable = EDITED;
    catsegm(wksp);
    DEBUGCHECK;
}

/*
 * Insert lines.
 */
void openlines(from, number)
    int from, number;
{
    if (from >= file[curfile].nlines)
        return;
    file[curfile].nlines += number;
    insert(curwksp, blanklines(number), from);
    wksp_redraw((workspace_t *)NULL, curfile,
        from, from + number - 1, number);
    poscursor(cursorcol, from - curwksp->toprow);
}

/*
 * Insert spaces.
 */
void openspaces(line, col, number, nl)
    int line, col, number, nl;
{
    register int i, j;

    for (i=line; i<line+nl; i++) {
        getlin(i);
        putbks(col, number);
        cline_modified = 1;
        putline();
        j = i - curwksp->toprow;
        if (j <= curwin->text_height)
            drawlines(j, j);
    }
    poscursor(col - curwksp->coloffset, line - curwksp->toprow);
}

/*
 * Дописать строку buf длиной n во врем. файл.
 * Возвращает описатель этой строки.
 */
static segment_t *writemp(buf,n)
    char *buf;
    int n;
{
    register segment_t *f1, *f2;
    register char *p;

    if (charsfi == tempfile)
        charsfi = 0;
    lseek(tempfile, tempseek, 0);

    n = dechars(buf, n-1);
    if (write(tempfile, buf, n) != n)
        return 0;

    /* now make segm */
    f1 = (segment_t *)salloc(2 + sizeof_segment_t);
    f2 = (segment_t *)salloc(sizeof_segment_t);
    f2->prev = f1;
    f1->next = f2;
    f1->nlines = 1;
    f1->fdesc = tempfile;
    f1->seek = tempseek;
    if (n <= 127)
        f1->data = n;
    else {
        p = &f1->data;
        *p++ = (n / 128)|0200;
        *p = n % 128;
    }
    tempseek += n;
    return (f1);
}

/*
 * Split a line at col position.
 */
void splitline(line, col)
    int line, col;
{
    register int nsave;
    register char csave;

    if (line >= file[curfile].nlines)
        return;
    file[curfile].nlines++;
    getlin(line);
    if (col >= cline_len - 1)
        openlines(line+1, 1);
    else {
        csave = cline[col];
        cline[col] = '\n';
        nsave = cline_len;
        cline_len = col+1;
        cline_modified = 1;
        putline();
        cline[col] = csave;
        insert(curwksp, writemp(cline+col, nsave-col), line+1);
        wksp_redraw((workspace_t *)NULL, curfile, line, line+1, 1);
    }
    poscursor(col - curwksp->coloffset, line - curwksp->toprow);
}

/*
 * segment_t *delete(wksp,from,to) -
 * Исключить указанные строки из wksp.
 * Возвращает указатель на segm - цепь исключенных строк,
 * с добавленнык концевым блоком.
 * Требует wksp_redraw.
 */
static segment_t *delete(wksp,from,to)
    workspace_t *wksp;
    int from,to;
{
    segment_t *w0;
    register segment_t *wf,*f0,*ff;
    breaksegm(wksp,to+1,1);
    DEBUGCHECK;
    wf = wksp->cursegm;
    breaksegm(wksp,from,1);
    f0 = wksp->cursegm;
    ff = wf->prev;
    w0 = f0->prev;
    wksp->cursegm = wf;
    wf->prev = w0;
    f0->prev = 0;
    ff->next = (segment_t *) salloc(sizeof_segment_t);
    ff->next->prev = ff;
    catsegm(wksp);
    file[wksp->wfile].writable = EDITED;
    DEBUGCHECK;
    return (f0);
}

/*
 * Delete lines from file.
 * frum < 0 - do not call wksp_redraw (used for "exec").
 */
void closelines(frum, number)
    int frum, number;
{
    register int n,from;
    register segment_t *f;

    if ((from = frum) < 0)
        from = -from-1;
    if (from < file[curfile].nlines)
        if ((file[curfile].nlines -= number) <= from)
            file[curfile].nlines = from + 1;
    f = delete(curwksp, from, from + number - 1);
    if (frum >= 0)
        wksp_redraw((workspace_t *)NULL, curfile, from,
            from + number - 1, -number);
    n = file[2].nlines;
    insert(pickwksp, f, n);
    wksp_redraw((workspace_t *)NULL, 2, n, n, number);
    deletebuf->linenum = n;
    deletebuf->nrows = number;
    deletebuf->ncolumns = 0;
    file[2].nlines += number;
    poscursor(cursorcol, from - curwksp->toprow);
}

/*
 * Pick (flg=0) / delete (flg=1).
 */
static void pcspaces(line, col, number, nl, flg)
    int line, col, number, nl, flg;
{
    register segment_t *f1,*f2;
    segment_t *f0;
    char *linebuf, *bp;
    register int i;
    int j, n, line0, line1;

    if (charsfi == tempfile)
        charsfi = 0;
    linebuf = salloc(number+1);
    f0 = f2 = 0;
    line0 = line;
    line1 = line0 + nl;
    while ((nl = (line1 - line0)) != 0) {
        if (nl > FSDMAXL)
            nl = FSDMAXL;
        f1 = (segment_t*) salloc(sizeof_segment_t + (number>127 ? nl*2 : nl));
        if (f2) {
            f2->next = f1;
            f1->prev = f2;
        } else
            f0 = f1;
        bp = &(f1->data);
        f1->nlines = nl;
        f1->fdesc = tempfile;
        f1->seek = tempseek;
        for (j=line0; j<line0+nl; j++) {
            getlin(j);
            if (col+number >= cline_len) {
                if (col+number >= cline_max)
                    excline(col+number+1);
                for (i=cline_len-1; i<col+number; i++)
                    cline[i] = ' ';
                cline[col+number] = '\n';
                cline_len = col + number + 1;
            }
            for (i=0; i<number; i++)
                linebuf[i] = cline[col+i];
            linebuf[number] = '\n';
            lseek(tempfile, tempseek, 0);
            if (charsfi == tempfile)
                charsfi = 0;
            n = dechars(linebuf, number);
            if (write(tempfile, linebuf, n) != n)
                puts1("Error writing temp file!\n");
            if (n > 127)
                *bp++ = (n/128) | 0200;
            *bp++ = n % 128;
            tempseek += n;
        }
        f2 = f1;
        line0 = line0 + nl;
    }
    f2->next = (segment_t*) salloc(sizeof_segment_t);
    f2->next->prev = f2;
    nl = line1 - line;
    if (flg) {
        for (j=line; j<line+nl; j++) {
            getlin(j);
            if (col+number >= cline_len) {
                if (col+number >= cline_max)
                    excline(col+number+1);
                for (i=cline_len-1; i<col+number; i++)
                    cline[i] = ' ';
                cline[col+number] = '\n';
                cline_len = col + number + 1;
            }
            for (i=col+number; i<cline_len; i++)
                cline[i-number] = cline[i];
            cline_len -= number;
            cline_modified = 1;
            putline();
            i = j - curwksp->toprow;
            if (i <= curwin->text_height)
                drawlines(i, i);
        }
    }
    n = file[2].nlines;
    insert(pickwksp, f0, n);
    wksp_redraw((workspace_t *)NULL, 2, n, n, nl);
    if (flg) {
        deletebuf->linenum = n;
        deletebuf->nrows = nl;
        deletebuf->ncolumns = number;
    } else {
        pickbuf->linenum = n;
        pickbuf->nrows = nl;
        pickbuf->ncolumns = number;
    }
    file[2].nlines += nl;
    free(linebuf);
    poscursor(col - curwksp->coloffset, line - curwksp->toprow);
}

/*
 * Delete rectangular area.
 */
void closespaces(line, col, number, nl)
    int line, col, number, nl;
{
    pcspaces(line, col, number, nl, 1);
}

/*
 * Merge a line with next one.
 */
void combineline(line, col)
    int line, col;
{
    register char *temp;
    register int nsave, i;

    if (file[curfile].nlines <= line-2)
        file[curfile].nlines--;
    getlin(line+1);
    temp = salloc(cline_len);
    for (i=0; i<cline_len; i++)
        temp[i] = cline[i];
    nsave = cline_len;
    getlin(line);
    if (col+nsave > cline_max)
        excline(col+nsave);
    for (i=cline_len-1; i<col; i++)
        cline[i] = ' ';
    for (i=0; i<nsave; i++)
        cline[col+i] = temp[i];
    cline_len = col + nsave;
    cline_modified = 1;
    putline();
    free((char *)temp);
    delete(curwksp, line+1, line+1);
    wksp_redraw((workspace_t *)NULL, curfile, line, line+1, -1);
    poscursor(col - curwksp->coloffset, line - curwksp->toprow);
}

/*
 * Возвращает копию цепи segm, от f до end, не включая end.
 * Если end = NULL - до конца файла.
 */
static segment_t *copysegm(f, end)
    segment_t *f,*end;
{
    segment_t *res, *ff, *rend = 0;
    register int i;
    register char *c1, *c2;

    res = 0;
    while (f->fdesc && f != end) {
        c1 = &f->data;
        for (i = f->nlines; i; i--)
            if (*c1++ & 0200)
                c1++;
        c2 = (char*) f; /* !!! Подсчет места !!!*/
        i = c1 - c2;
        c2 = salloc(i);
        ff = (segment_t*) c2;
        c2 += i;
        while (i--)
            *--c2 = *--c1;
        if (res) {
            rend->next = ff;
            ff->prev = rend;
            rend = ff;
        } else
            res = rend = ff;
        f = f->next;
    }
    if (res) {
        rend->next = (segment_t *)salloc(sizeof_segment_t);
        rend->next->prev = rend;
        rend = rend->next;
    } else
        res = rend = (segment_t *)salloc(sizeof_segment_t);

    if (f->fdesc == 0)
        rend->seek = f->seek;
    return res;
}

/*
 * segment_t *pick(wksp,from,to) -
 * Возвращает указатель на segm - цепь указанных строк,
 * с добавленным концевым блоком.
 * Требует wksp_redraw.
 */
static segment_t *pick(wksp,from,to)
    workspace_t *wksp;
    int from,to;
{
    segment_t *wf;

    breaksegm(wksp,to+1,1);
    wf = wksp->cursegm;
    breaksegm(wksp, from, 1);
    return copysegm(wksp->cursegm, wf);
}

/*
 * Get lines from file to pick workspace.
 */
void picklines(from, number)
    int from, number;
{
    register int n;
    register segment_t *f;

    f = pick(curwksp, from, from + number - 1);
    /* because of breaksegm */
    wksp_redraw((workspace_t *)NULL, curfile, from, from + number - 1, 0);
    n = file[2].nlines;
    insert(pickwksp, f, n);
    wksp_redraw((workspace_t *)NULL, 2, n, n, number);
    pickbuf->linenum = n;
    pickbuf->nrows = number;
    pickbuf->ncolumns = 0;
    file[2].nlines += number;
    poscursor(cursorcol, from - curwksp->toprow);
}

/*
 * Get rectangular area to pick buffer.
 */
void pickspaces(line, col, number, nl)
    int line, col, number, nl;
{
    pcspaces(line, col, number, nl, 0);
}

/*
 * Поместить на указанное место строки из buf.
 * Должно быть buf->ncolumns == 0 .
 */
static void plines(buf, line)
    clipboard_t *buf;
    int line;
{
    int lbuf, cc, cl;
    segment_t *w0, *w1;
    register segment_t *f, *g;
    register int j;

    cc = cursorcol;
    cl = cursorline;
    breaksegm(pickwksp, buf->linenum + buf->nrows,1);
    w1 = pickwksp->cursegm;
    breaksegm(pickwksp, buf->linenum,1);
    w0 = pickwksp->cursegm;
    f = g = copysegm(w0,w1);
    lbuf = 0;
    while (g->fdesc) {
        lbuf += g->nlines;
        g = g->next;
    }
    insert(curwksp,f,line);
    wksp_redraw((workspace_t *)NULL,curfile,line,line,lbuf);
    poscursor(cc,cl);
    if ((file[curfile].nlines += lbuf) <= (j = line + lbuf))
        file[curfile].nlines = j+1;
}

/*
 * Поместить на указанное место взятое "buf"
 * Должно быть buf->ncolumns != 0 .
 */
static void pspaces(buf, line, col)
    clipboard_t *buf;
    int line, col;
{
    workspace_t *oldwksp;
    char *linebuf;
    int nc, i, j;

    linebuf = salloc(nc = buf->ncolumns);
    oldwksp = curwksp;
    for (i=0; i<buf->nrows; i++) {
        curwksp = pickwksp;
        getlin(buf->linenum + i);
        if (cline_len-1 < nc)
            for (j=cline_len-1; j<nc; j++)
                cline[j] = ' ';
        for (j=0; j<nc; j++)
            linebuf[j] = cline[j];
        curwksp = oldwksp;
        getlin(line + i);
        putbks(col, nc);
        for (j=0; j<nc; j++)
            cline[col+j] = linebuf[j];
        cline_modified = 1;
        putline();
        j = line+i-curwksp->toprow;
        if (j <= curwin->text_height)
            drawlines(j, j);
    }
    free(linebuf);
    poscursor(col - curwksp->coloffset, line - curwksp->toprow);
}

/*
 * Поместить на указанное место взятое "buf"
 * Если buf->ncolumns == 0, вставляются строки,
 * иначе прямоугольная область.
 */
void put(buf, line, col)
    clipboard_t *buf;
    int line, col;
{
    if (buf->ncolumns == 0)
        plines(buf, line);
    else
        pspaces(buf, line, col);
}

/*
 * getlin(ln) -
 * Дай строку ln из текущего curwksp.
 * строка попадает в cline, длина - в cline_len.
 */
void getlin(ln)
    int ln;
{
    cline_modified = 0;
    clineno = ln;
    if (wksp_seek(curwksp, ln)) {
        if (cline_max == 0)
            excline(1);
        cline[0] = '\n';
        cline_len = 1;
    } else
        chars(1);
}

/*
 * Поместить строку из cline в curwksp, строка nl.
 */
void putline()
{
    segment_t *w0,*cl;
    register segment_t *wf, *wg;
    register workspace_t *w;
    int i;
    char flg;

    DEBUGCHECK;
    if (cline_modified == 0) {
        clineno = -1;
        return;
    }
    if (file[curfile].nlines <= clineno)
        file[curfile].nlines = clineno + 1;
    cline_modified = 0;
    cline[cline_len-1] = '\n';
    cl = writemp(cline, cline_len);
    w = curwin->wksp;             /* w s can be replaced by curwksp */
    i = clineno;
    flg = breaksegm(w,i,1);
    wg = w->cursegm;
    w0 = wg->prev;
    if (flg == 0) {
        breaksegm(w, i+1, 0);
        wf = w->cursegm;
        free((char *)cl->next);
        cl->next = wf;
        wf->prev = cl;
    }
    free((char *)wg);
    cl->prev = w0;
    w->cursegm = cl;
    w->line = w->segmline = i;
    file[w->wfile].writable = EDITED;
    clineno = -1;
    catsegm(w);
    wksp_redraw(w, w->wfile, i, i, 0);
    DEBUGCHECK;
}

/*
 * Почистить цепочку segm.
 */
void freesegm(f)
    segment_t *f;
{
    register segment_t *g;

    while (f) {
        g = f;
        f = f->next;
        free((char *)g);
    }
}

/*
 * Replace m lines from "line", via jproc pipe.
 */
void doreplace(line, m, jproc, pipef)
    int line, m, jproc, *pipef;
{
    register segment_t *e, *ee;
    register int l;
    int n;

    close(pipef[0]);
    breaksegm(curwksp, line, 0);
    if (m == 0)
        close(pipef[1]);
    else {
        m = segmwrite(curwksp->cursegm, m, pipef[1]);
        if (m == -1) {
            error("Can't write on pipe.");
            kill(jproc, 9);
        }
    }
    while (wait(&n) != jproc);          /* wait for completion */
    if ((n & 0177400) == 0157400) {
        error("Can't find program to execute.");
        return;
    }
    if ((n & 0177400) == 0177000 || (n & 0377) != 0) {
        error("Abnormal termination of program.");
        return;
    }
    charsfi = -1;                   /* forget old position before fork */
    if (m)
        closelines(-1-line,m);
    charsin(tempfile, tempseek);
    ee = e = temp2segm(tempfile);
    tempseek = charskl;
    l = 0;
    if (e->nlines) {
        while (e->fdesc) {
            l += e->nlines;
            e = e->next;
        }
        insert(curwksp, ee, line);
        file[curfile].nlines += l;
    }
    wksp_redraw((workspace_t *)NULL, curfile, line, line + m, l - m);
    poscursor(cursorcol, cursorline);
}

/*
 * Run command with parameters args.
 */
void execr(args)
    char **args;
{
    execvp(*args, args);
    exit(0337); /* Код ответа согласован с doreplace */
}

#ifdef DEBUG
/*
 * Debug output of segm chains.
 */
void printsegm(f)
    segment_t *f;
{
    int i;
    register char *c;

    printf("\n**********");
    while (f) {
        printf("\nsegmnl=%d chan=%d seek=%lu at %p",
            f->nlines, f->fdesc, (long) f->seek, f);
        if (f->next && f != f->next->prev)
            printf("\n*** next block bad backpointer ***");
        c = &(f->data);
        for (i=0; i<f->nlines; i++) {
            if ((i % 20) == 0)
                putchar('\n');
            printf(" %d", *c++);
        }
        f = f->next;
    }
}

/*
 * Check segm correctness.
 */
void checksegm()
{
    register segment_t *f;
    register int nl;

    nl = 0;
    f = file[curfile].chain;
    while (f) {
        if (curwksp->line >= nl &&
            curwksp->line < nl + f->nlines &&
            curwksp->cursegm != f && curwksp->cursegm->prev)
            fatal("CKFSD CURFSD LOST");

        if (f->next && f->next->prev != f)
            fatal("CKFSD BAD CHAIN");

        nl += f->nlines;
        f = f->next;
    }
}
#endif
