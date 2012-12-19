/*
 * Implementation of macro features.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"

/*
 * Типы макросов
 * TAG - точка в файле
 * BUF - буфер вставки
 * MAC - макро=вставка
 */
#define MTAG 1
#define MBUF 2
#define MMAC 3

typedef struct {
    int line, col, nfile;
} tag_t;

#define NMACRO ('z'-'a'+1)

typedef union {
    clipboard_t mbuf;
    tag_t mtag;
    char *mstring;
} macro_t;

macro_t *mtaba[NMACRO];

char mtabt[NMACRO];

/*
 * macro_t *mname(name,typ,l)
 * Функция поиска описателя по имени
 * если l=0, то ищет и проверяет тип,
 * иначе создает новый описатель
 */
macro_t *mname(name, typ, l)
    register char *name;
    int typ, l;
{
    register int i;char cname;
    cname = (*name|040) &0177;
    if((cname >'z') || (cname<'a') || (*(name+1) != 0))
    {
        error("ill.macro name");
        goto err;
    }
    i= cname -'a';
    if(l) {
        if(mtaba[i]) {
            if (mtabt[i] == MMAC)
                free(mtaba[i]->mstring);
            free(mtaba[i]);
            telluser("macro redefined",0);
        }
        mtabt[i]=typ;
        mtaba[i]=(macro_t *)salloc(l);
        goto retn;
    }
    if( mtabt[i] != typ) {
        error( mtabt[i]?"ill.macro type":"undefined");
        goto err;
    }
retn:
    return(mtaba[i]);
err:
    return(0);
}

/*
 * Функция запоминает и выдает буфер вставки
 * op = 1 - выдать, 0 - запомнить
 * ответ 1, если хорошо, иначе 0
 */
int msrbuf(sbuf, name, op)
    register clipboard_t *sbuf;
    register char *name;
    int op;
{
    register macro_t *m;

    m = mname(name, MBUF, (op ? 0 : sizeof(clipboard_t)));
    if (m) {
        if (op)
            *sbuf = m->mbuf;
        else
            m->mbuf = *sbuf;
        return 1;
    }
    return 0;
}

/*
 * Функция запоминает текущее положение курсора в файле под именем name.
 * Ее дефект в том, что tag (метка) не связана с файлом жестко и
 * перемещается при редактировании предыдущих строк файла
 */
int msvtag(name)
    register char *name;
{
    register macro_t *m;
    register workspace_t *cws;

    cws = curwksp;
    m = mname(name, MTAG, sizeof(tag_t));
    if (! m)
        return 0;
    m->mtag.line  = cursorline + cws->toprow;
    m->mtag.col   = cursorcol  + cws->coloffset;
    m->mtag.nfile = cws->wfile;
    return 1;
}

/*
 * Return a cursor back to named position.
 * cgoto is common for it and other functions.
 */
int mgotag(name)
    char *name;
{
    register int i;
    int fnew = 0;
    register macro_t *m;

    m = mname(name, MTAG, 0);
    if (! m)
            return 0;
    i = m->mtag.nfile;
    if (curwksp->wfile != i) {
        editfile(file[i].name, 0, 0, 0, 0);
        fnew = 1;
    }
    cgoto(m->mtag.line, m->mtag.col, -1, fnew);
    highlight_position = 1;
    return 1;
}

/*
 * Функция mdeftag вырабатывает параметры, описывающие область
 * между текущим положением курсора и меткой "name". Она заполняет:
 *      param_type = -2
 *      param_c1   =    соответствует точке "name"
 *      param_r1   =           -- // --
 */
int mdeftag(name)
    char *name;
{
    register macro_t *m;
    register workspace_t *cws;
    int cl, ln, f = 0;

    m = mname(name, MTAG, 0);
    if (! m)
        return 0;
    cws = curwksp;
    if (m->mtag.nfile != cws->wfile) {
        error("another file");
        return(0);
    }
    param_type = -2;
    param_r1 = m->mtag.line;
    param_c1 = m->mtag.col ;
    param_r0 += cws->toprow;
    param_c0 += cws->coloffset;
    if (param_r0 > param_r1) {
        f++;
        ln = param_r1;
        param_r1 = param_r0;
        param_r0 = ln;
    } else
        ln = param_r0;
    if (param_c0 > param_c1) {
        f++;
        cl = param_c1;
        param_c1 = param_c0;
        param_c0 = cl;
    } else
        cl = param_c0;
    if (f) {
        cgoto(ln, cl, -1, 0);
    }
    param_r0 -= cws->toprow;
    param_r1 -= cws->toprow;
    param_c0 -= cws->coloffset;
    param_c1 -= cws->coloffset;
    if (param_r1 == param_r0)
        telluser("**:columns defined by tag", 0);
    else if (param_c1 == param_c0)
        telluser("**:lines defined by tag", 0);
    else
        telluser("**:square defined by tag", 0);
    return 1;
}

/*
 * Define a macro sequence.
 */
int defmac(name)
    char *name;
{
    register macro_t *m;

    m = mname(name, MMAC, sizeof(char*));
    if (! m)
        return 0;
    param(1);
    if (param_type == 1 && param_str) {
        m->mstring = param_str;
        param_len = 0;
        param_str = NULL;
        return 1;
    }
    return 0;
}

/*
 * Get macro contents by name.
 * Name is a single symbol with code = isy - 0200 + 'a'
 */
char *rmacl(isy)
    int isy;
{
        char nm[2];
        register macro_t *m;

        nm[0] = isy - (CCMACRO + 1) + 'a';
        nm[1] = 0;
        m = mname(nm, MMAC, 0);
        if (! m)
            return 0;
        return m->mstring;
}

/*
 * Redefine a keyboard key.
 */
int defkey()
{
#define LKEY 20 /* Макс. число символов, генерируемых новой клавишей */
    char bufc[LKEY+1], *buf;
    register int lc;
    window_t *curp;
    int curl,curc;
    register char *c, *c1;

    curp = curwin;
    curc = cursorcol;
    curl = cursorline;
    win_switch(&paramwin);
    poscursor(22, 0);
    telluser(" enter <new key><del>:",0);
    lc = 0;
    while((bufc[lc] = rawinput()) != '\177' && lc++ < LKEY)
        continue;
    if (lc == 0 || lc == LKEY) {
reterr: lc = 0;
        error("illegal");
        goto ret;
    }
    bufc[lc] = 0;
    telluser("enter <command> or <macro name>:",1);
    poscursor(33, 0);
    keysym = -1;
    getkeysym();
    if (! CTRLCHAR(keysym)) {
        if (keysym == '$')
            keysym = CCMACRO;

        else if(keysym >= 'a' && keysym <= 'z')
            keysym += CCMACRO + 1 -'a';
        else
            goto reterr;
    }
    telluser("", 0);
    c = buf = salloc(lc + 1);
    c1 = bufc;
    while ((*c++ = *c1++) != 0);
    lc = addkey(keysym, buf);
ret:
    keysym = -1;
    win_switch(curp);
    poscursor(curc, curl);
    return lc;
}
