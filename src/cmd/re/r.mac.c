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
    m->mtag.line  = cursorline + cws->ulhclno;
    m->mtag.col   = cursorcol  + cws->ulhccno;
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
 *      paramtype = -2
 *      paramc1   =    соответствует точке "name"
 *      paramr1   =           -- // --
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
    paramtype = -2;
    paramr1 = m->mtag.line;
    paramc1 = m->mtag.col ;
    paramr0 += cws -> ulhclno;
    paramc0 += cws -> ulhccno;
    if (paramr0 > paramr1) {
        f++;
        ln = paramr1;
        paramr1 = paramr0;
        paramr0 = ln;
    } else
        ln = paramr0;
    if (paramc0 > paramc1) {
        f++;
        cl = paramc1;
        paramc1 = paramc0;
        paramc0 = cl;
    } else
        cl = paramc0;
    if (f) {
        cgoto(ln, cl, -1, 0);
    }
    paramr0 -= cws -> ulhclno;
    paramr1 -= cws -> ulhclno;
    paramc0 -= cws -> ulhccno;
    paramc1 -= cws -> ulhccno;
    if (paramr1 == paramr0)
        telluser("**:columns defined by tag", 0);
    else if (paramc1 == paramc0)
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
    if (paramtype == 1 && paramv) {
        m->mstring = paramv;
        paraml = 0;
        paramv = NULL;
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

        nm[0] = isy - (CCMAC + 1) + 'a';
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
    viewport_t *curp;
    int curl,curc;
    register char *c, *c1;

    curp = curport;
    curc = cursorcol;
    curl = cursorline;
    switchport(&paramport);
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
            keysym = CCMAC;

        else if(keysym >= 'a' && keysym <= 'z')
            keysym += CCMAC + 1 -'a';
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
    switchport(curp);
    poscursor(curc, curl);
    return lc;
}
