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
 */
#define MTAG 1
#define MBUF 2

typedef struct {
    int line, col, nfile;
} tag_t;

typedef union {
    clipboard_t mclipboard;
    tag_t mtag;
} macro_t;

#define NMACRO ('z'-'a'+1)

static macro_t *mtab_data[NMACRO];

static char mtab_type[NMACRO];

/*
 * Функция поиска описателя по имени
 * если nbytes=0, то ищет и проверяет тип,
 * иначе создает новый описатель
 */
static macro_t *mfind(name, typ, nbytes)
    register char *name;
    int typ, nbytes;
{
    register int i;

    i = ((*name | 040) & 0177) - 'a';
    if (i < 0 || i > 'z'-'a' || name[1] != 0) {
        error("Invalid macro name");
        return 0;
    }
    if (nbytes == 0) {
        /* Check the type of macro. */
        if (mtab_type[i] != typ) {
            error (mtab_type[i] ? "Invalid type of macro" : "Macro undefined");
            return 0;
        }
    } else {
        /* Reallocate. */
        if (mtab_data[i]) {
            free(mtab_data[i]);
            telluser("macro redefined",0);
        }
        mtab_data[i] = (macro_t*) salloc(nbytes);
        mtab_type[i] = typ;
    }
    return mtab_data[i];
}

/*
 * Функция запоминает и выдает буфер вставки
 * op = 1 - выдать, 0 - запомнить
 * ответ 1, если хорошо, иначе 0
 */
int msrbuf(cb, name, op)
    register clipboard_t *cb;
    register char *name;
    int op;
{
    register macro_t *m;

    m = mfind(name, MBUF, (op ? 0 : sizeof(clipboard_t)));
    if (! m)
        return 0;

    if (op)
        *cb = m->mclipboard;
    else
        m->mclipboard = *cb;
    return 1;
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
    m = mfind(name, MTAG, sizeof(tag_t));
    if (! m)
        return 0;
    m->mtag.line  = cursorline + cws->topline;
    m->mtag.col   = cursorcol  + cws->offset;
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

    m = mfind(name, MTAG, 0);
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

    m = mfind(name, MTAG, 0);
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
    param_r0 += cws->topline;
    param_c0 += cws->offset;
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
    param_r0 -= cws->topline;
    param_r1 -= cws->topline;
    param_c0 -= cws->offset;
    param_c1 -= cws->offset;
    if (param_r1 == param_r0)
        telluser("**:columns defined by tag", 0);
    else if (param_c1 == param_c0)
        telluser("**:lines defined by tag", 0);
    else
        telluser("**:square defined by tag", 0);
    return 1;
}
