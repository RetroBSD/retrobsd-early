/*
 * Редактор RED. ИАЭ им. И.В. Курчатова, ОС ДЕМОС
 * Файл содержит программы, реализующие новые возможности
 * редактора "RED", связанные с макропеременными
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

struct tag {
    int line, col, nfile;
};

#define MSBUF SSAVEBUF
#define MSTAG sizeof(struct tag)
#define MSMAC sizeof(char *)

#define LMAC ('z'-'a'+1)

int csrsw; /* Для яркой отметки на экране */

union macro {
    struct savebuf mbuf;
    struct tag mtag;
    char *mstring;
} *mtaba[LMAC];

char mtabt[LMAC];

/*
 * union macro *mname(name,typ,l)
 * Функция поиска описателя по имени
 * если l=0, то ищет и проверяет тип,
 * иначе создает новый описатель
 */
union macro *mname(name,typ,l)
register char *name;
int typ,l;
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
        mtaba[i]=(union macro *)salloc(l);
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
    register struct savebuf *sbuf;
    register char *name;
    int op;
{
    register union macro *m;

    m = mname(name, MBUF, (op ? 0 : MSBUF));
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
    register union macro *m;
    register struct workspace *cws;

    cws = curwksp;
    m = mname(name, MTAG, MSTAG);
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
    register union macro *m;

    m = mname(name, MTAG, 0);
    if (! m)
            return 0;
    i = m->mtag.nfile;
    if (curwksp->wfile != i) {
        editfile(openfnames[i], 0, 0, 0, 0);
        fnew = 1;
    }
    cgoto(m->mtag.line, m->mtag.col, -1, fnew);
    csrsw = 1;
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
    register union macro *m;
    register struct workspace *cws;
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
    register union macro *m;

    m = mname(name, MMAC, MSMAC);
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
 * char *rmacl(isy)
 * Выдается макро = последовательность или 0;
 * Имя определяется как один символ с кодом isy - 0200 + 'a'
 */
char *rmacl(isy)
int isy;
{
        char nm[2];
        register union macro *m;
        nm[0]=isy - (CCMAC+1) + 'a';
        nm[1] =0;
        if (!(m=mname(nm,MMAC,0))) return(0);
        return(m->mstring);
}

/*
 * Redefine a keyboard key.
 */
int defkey()
{
#define LKEY 20 /* Макс. число символов, генерируемых новой клавишей */
    char bufc[LKEY+1], *buf;
    register int lc;
    struct viewport *curp;
    int curl,curc;
    register char *c, *c1;

    curp = curport;
    curc = cursorcol;
    curl = cursorline;
    switchport(&paramport);
    poscursor(22,0);
    telluser(" enter <new key><del>:",0);
    lc = 0;
    while((bufc[lc] = read2()) !='\177'  && lc++ < LKEY);
    if ( lc ==0 || lc == LKEY )
    {
        goto reterr;
    }
    bufc[lc] = 0;
    telluser("enter <command> or <macro name>:",1);
    poscursor(33,0);
    lread1= -1;
    read1();
    if (!CTRLCHAR) {
        if(lread1 == '$') lread1 = CCMAC;
        else
            if(lread1 >= 'a' && lread1 <= 'z') lread1 += CCMAC+1 -'a';
            else {
                goto reterr;
            }
    }
    telluser("",0);
    c = buf = salloc(lc + 1);
    c1 = bufc;
    while ( *c++ = *c1++);
    lc = addkey(lread1,buf);
ret:
    lread1 = -1;
    switchport(curp);
    poscursor(curc,curl);
    return(lc);
reterr:
    lc = 0;
    error("illegal");
    goto ret;
}
