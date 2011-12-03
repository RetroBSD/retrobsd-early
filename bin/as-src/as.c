/*
 * Assembler for MIPS.
 */
#include </usr/include/stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <a.out.h>

#define USER_CODE_START 0x7f008000
#define WORDSZ          4               /* длина слова в байтах */

/* типы лексем */

#define LEOF            1
#define LEOL            2
#define LNAME           3
#define LCMD            4
#define LACMD           5
#define LNUM            6
#define LLCMD           7
#define LSCMD           8
#define LLSHIFT         9
#define LRSHIFT         10
#define LINCR           11
#define LDECR           12

/* номера сегментов */

#define SCONST          0
#define STEXT           1
#define SDATA           2
#define SSTRNG          3
#define SBSS            4
#define SEXT            6
#define SABS            7               /* вырожденный случай для getexpr */

/* директивы ассемблера */

#define ASCII           0
#define BSS             1
#define COMM            2
#define DATA            3
#define GLOBL           4
#define SHORT            5
#define STRNG           6
#define TEXT            7
#define EQU             8
#define WORD            9

/* типы команд */

#define TLONG           01              /* длинноадресная команда */
#define TALIGN          02              /* после команды делать выравнивание */
#define TLIT            04              /* команда имеет литеральный аналог */
#define TINT            010             /* команда имеет целочисленный режим */
#define TCOMP           020             /* из команды можно сделать компонентную */

/* длины таблиц */
/* хэш-длины должны быть степенями двойки! */

#define HASHSZ          2048            /* длина хэша таблицы имен */
#define HCMDSZ          1024            /* длина хэша команд ассемблера */

#define STSIZE          (HASHSZ*9/10)   /* длина таблицы имен */
#define SPACESZ         (STSIZE*8)      /* длина массива под имена */

#define EMPCOM          0x3a00000L      /* пустая команда - заполнитель */
#define UTCCOM          0x3a00000L      /* команда <> */
#define WTCCOM          0x3b00000L      /* команда [] */

/* превращение команды в компонентную */

#define MAKECOMP(c)     ((c) & 0x2000000L ? (c)|0x4800000L : (c)|0x6000000L)

/* оптимальное значение хэш-множителя для 32-битного слова == 011706736335L */
/* то же самое для 16-битного слова = 067433 */

#define SUPERHASH(key,mask) (((key) * 067433) & (mask))

#define ISHEX(c)        (ctype[(c)&0377] & 1)
#define ISOCTAL(c)      (ctype[(c)&0377] & 2)
#define ISDIGIT(c)      (ctype[(c)&0377] & 4)
#define ISLETTER(c)     (ctype[(c)&0377] & 8)

/* на втором проходе hashtab не нужна, используем ее под именем
 * newindex для переиндексации настройки в случае флагов x или X */

#define newindex hashtab

const char ctype [256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,7,7,7,7,7,7,7,7,5,5,0,0,0,0,0,0,
    0,9,9,9,9,9,9,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,0,0,0,0,8,
    0,9,9,9,9,9,9,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,0,
};

/*
 * преобразование номера сегмента в тип символа
 */
const int segmtype [] = {
    N_TEXT,             /* STEXT */
    N_DATA,             /* SDATA */
    N_STRNG,            /* SSTRNG */
    N_BSS,              /* SBSS */
    N_UNDF,             /* SEXT */
    N_ABS,              /* SABS */
};

/*
 * преобразование номера сегмента в тип настройки
 */
const int segmrel [] = {
    RTEXT,              /* STEXT */
    RDATA,              /* SDATA */
    RSTRNG,             /* SSTRNG */
    RBSS,               /* SBSS */
    REXT,               /* SEXT */
    RABS,               /* SABS */
};

/*
 * преобразование типа символа в номер сегмента
 */
const int typesegm [] = {
    SEXT,               /* N_UNDF */
    SABS,               /* N_ABS */
    STEXT,              /* N_TEXT */
    SDATA,              /* N_DATA */
    SBSS,               /* N_BSS */
    SSTRNG,             /* N_STRNG */
};

struct table {
    unsigned val;
    const char *name;
    unsigned type;
};

const struct table table [] = {
    /* TODO */
    { 0x00000000,   "add",      0 },
    { 0x00000000,   "addi",     0 },
    { 0x00000000,   "addiu",    0 },
    { 0x00000000,   "addiupc",  0 },
    { 0x00000000,   "addu",     0 },
    { 0x00000000,   "and",      0 },
    { 0x00000000,   "andi",     0 },
    { 0x00000000,   "b",        0 },
    { 0x00000000,   "bal",      0 },
    { 0x00000000,   "beq",      0 },
    { 0x00000000,   "beql",	0 },
    { 0x00000000,   "bgez",	0 },
    { 0x00000000,   "bgezal",	0 },
    { 0x00000000,   "bgezall",	0 },
    { 0x00000000,   "bgezl",	0 },
    { 0x00000000,   "bgtz",	0 },
    { 0x00000000,   "bgtzl",	0 },
    { 0x00000000,   "blez",	0 },
    { 0x00000000,   "blezl",	0 },
    { 0x00000000,   "bltz",	0 },
    { 0x00000000,   "bltzal",	0 },
    { 0x00000000,   "bltzall",	0 },
    { 0x00000000,   "bltzl",	0 },
    { 0x00000000,   "bne",	0 },
    { 0x00000000,   "bnel",	0 },
    { 0x00000000,   "break",	0 },
    { 0x00000000,   "clo",	0 },
    { 0x00000000,   "clz",	0 },
    { 0x00000000,   "cop0",	0 },
    { 0x00000000,   "deret",	0 },
    { 0x00000000,   "di",	0 },
    { 0x00000000,   "div",	0 },
    { 0x00000000,   "divu",	0 },
    { 0x00000000,   "ehb",	0 },
    { 0x00000000,   "ei",	0 },
    { 0x00000000,   "eret",	0 },
    { 0x00000000,   "ext",	0 },
    { 0x00000000,   "ins",	0 },
    { 0x00000000,   "j",	0 },
    { 0x00000000,   "jal",	0 },
    { 0x00000000,   "jalr",	0 },
    { 0x00000000,   "jalr.hb",	0 },
    { 0x00000000,   "jalrc",	0 },
    { 0x00000000,   "jr",	0 },
    { 0x00000000,   "jr.hb",	0 },
    { 0x00000000,   "jrc",	0 },
    { 0x00000000,   "lb",	0 },
    { 0x00000000,   "lbu",	0 },
    { 0x00000000,   "lh",	0 },
    { 0x00000000,   "lhu",	0 },
    { 0x00000000,   "ll",	0 },
    { 0x00000000,   "lui",	0 },
    { 0x00000000,   "lw",	0 },
    { 0x00000000,   "lwl",	0 },
    { 0x00000000,   "lwpc",	0 },
    { 0x00000000,   "lwr",	0 },
    { 0x00000000,   "madd",	0 },
    { 0x00000000,   "maddu",	0 },
    { 0x00000000,   "mfc0",	0 },
    { 0x00000000,   "mfhi",	0 },
    { 0x00000000,   "mflo",	0 },
    { 0x00000000,   "movn",	0 },
    { 0x00000000,   "movz",	0 },
    { 0x00000000,   "msub",	0 },
    { 0x00000000,   "msubu",	0 },
    { 0x00000000,   "mtc0",	0 },
    { 0x00000000,   "mthi",	0 },
    { 0x00000000,   "mtlo",	0 },
    { 0x00000000,   "mul",	0 },
    { 0x00000000,   "mult",	0 },
    { 0x00000000,   "multu",	0 },
    { 0x00000000,   "nop",	0 },
    { 0x00000000,   "nor",	0 },
    { 0x00000000,   "or",	0 },
    { 0x00000000,   "ori",	0 },
    { 0x00000000,   "rdhwr",	0 },
    { 0x00000000,   "rdpgpr",	0 },
    { 0x00000000,   "restore",	0 },
    { 0x00000000,   "rotr",	0 },
    { 0x00000000,   "rotrv",	0 },
    { 0x00000000,   "save",	0 },
    { 0x00000000,   "sb",	0 },
    { 0x00000000,   "sc",	0 },
    { 0x00000000,   "sdbbp",	0 },
    { 0x00000000,   "seb",	0 },
    { 0x00000000,   "seh",	0 },
    { 0x00000000,   "sh",	0 },
    { 0x00000000,   "sll",	0 },
    { 0x00000000,   "sllv",	0 },
    { 0x00000000,   "slt",	0 },
    { 0x00000000,   "slti",	0 },
    { 0x00000000,   "sltiu",	0 },
    { 0x00000000,   "sltu",	0 },
    { 0x00000000,   "sra",	0 },
    { 0x00000000,   "srav",	0 },
    { 0x00000000,   "srl",	0 },
    { 0x00000000,   "srlv",	0 },
    { 0x00000000,   "ssnop",	0 },
    { 0x00000000,   "sub",	0 },
    { 0x00000000,   "subu",	0 },
    { 0x00000000,   "sw",	0 },
    { 0x00000000,   "swl",	0 },
    { 0x00000000,   "swr",	0 },
    { 0x00000000,   "sync",	0 },
    { 0x00000000,   "syscall",	0 },
    { 0x00000000,   "teq",	0 },
    { 0x00000000,   "teqi",	0 },
    { 0x00000000,   "tge",	0 },
    { 0x00000000,   "tgei",	0 },
    { 0x00000000,   "tgeiu",	0 },
    { 0x00000000,   "tgeu",	0 },
    { 0x00000000,   "tlt",	0 },
    { 0x00000000,   "tlti",	0 },
    { 0x00000000,   "tltiu",	0 },
    { 0x00000000,   "tltu",	0 },
    { 0x00000000,   "tne",	0 },
    { 0x00000000,   "tnei",	0 },
    { 0x00000000,   "wait",	0 },
    { 0x00000000,   "wrpgpr",	0 },
    { 0x00000000,   "wsbh",	0 },
    { 0x00000000,   "xor",	0 },
    { 0x00000000,   "xori",	0 },
    { 0x00000000,   "zeb",	0 },
    { 0x00000000,   "zeh",	0 },
    { 0,            0,          0 },
};

FILE *sfile [SABS], *rfile [SABS];
unsigned count [SABS];
int segm;
char *infile, *outfile = "a.out";
char *tfilename = "/tmp/asXXXXXX";
int line;                             /* номер текущей строки */
int debug;                            /* флаг отладки */
int xflags, Xflag, uflag;
int stlength;                         /* длина таблицы символов в байтах */
int stalign;                          /* выравнивание таблицы символов */
unsigned tbase, dbase, adbase, bbase;
struct nlist stab [STSIZE];
int stabfree;
char space [SPACESZ];                   /* место под имена символов */
int lastfree;                         /* счетчик занятого места */
int regleft;                          /* номер регистра слева от команды */
char name [256];
unsigned intval;
int extref;
int blexflag, backlex, blextype;
short hashtab [HASHSZ], hashctab [HCMDSZ];
int aflag;                            /* не выравнивать на границу слова */

void getbitnum (int c);
void getbitmask (void);
unsigned getexpr (int *s);

void uerror (char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    fprintf (stderr, "as: ");
    if (infile)
        fprintf (stderr, "%s, ", infile);
    fprintf (stderr, "%d: ", line);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
    fprintf (stderr, "\n");
    exit (1);
}

unsigned int fgetword (f)
    register FILE *f;
{
    register unsigned int h;

    h = getc (f);
    h |= getc (f) << 8;
    h |= getc (f) << 16;
    h |= getc (f) << 24;
    return h;
}

void fputword (h, f)
    register unsigned int h;
    register FILE *f;
{
    putc (h, f);
    putc (h >> 8, f);
    putc (h >> 16, f);
    putc (h >> 24, f);
}

void fputhdr (filhdr, coutb)
    register struct exec *filhdr;
    register FILE *coutb;
{
    fputword (filhdr->a_magic, coutb);
    fputword (filhdr->a_text, coutb);
    fputword (filhdr->a_data, coutb);
    fputword (filhdr->a_bss, coutb);
    fputword (filhdr->a_syms, coutb);
    fputword (filhdr->a_entry, coutb);
    fputword (0, coutb);
    fputword (filhdr->a_flag, coutb);
}

void fputsym (s, file)
    register struct nlist *s;
    register FILE *file;
{
	register int i;

	putc (s->n_len, file);
	putc (s->n_type, file);
	fputword (s->n_value, file);
	for (i=0; i<s->n_len; i++)
		putc (s->n_name[i], file);
}

void startup ()
{
    register int i;

    mktemp (tfilename);
    for (i=STEXT; i<SBSS; i++) {
        if (! (sfile [i] = fopen (tfilename, "w+")))
            uerror ("cannot open %s", tfilename);
        unlink (tfilename);
        if (! (rfile [i] = fopen (tfilename, "w+")))
            uerror ("cannot open %s", tfilename);
        unlink (tfilename);
    }
    line = 1;
}

int chash (s)
    register const char *s;
{
    register int h, c;

    h = 12345;
    while ((c = *s++) != 0)
        h += h + c;
    return (SUPERHASH (h, HCMDSZ-1));
}

void hashinit ()
{
    register int i, h;
    register const struct table *p;

    for (i=0; i<HCMDSZ; i++)
        hashctab[i] = -1;
    for (p=table; p->name; p++) {
        h = chash (p->name);
        while (hashctab[h] != -1)
            if (--h < 0)
                h += HCMDSZ;
        hashctab[h] = p - table;
    }
    for (i=0; i<HASHSZ; i++)
        hashtab[i] = -1;
}

int hexdig (c)
    register int c;
{
    if (c <= '9')
        return (c - '0');
    else if (c <= 'F')
        return (c - 'A' + 10);
    else
        return (c - 'a' + 10);
}

/*
 * считать шестнадцатеричное число 'ZZZ
 */
void getlhex ()
{
    register int c;
    register char *cp, *p;

    c = getchar ();
    for (cp=name; ISHEX(c); c=getchar())
        *cp++ = hexdig (c);
    ungetc (c, stdin);
    intval = 0;
    p = name;
    for (c=28; c>=0; c-=4, ++p) {
        if (p >= cp)
            return;
        intval |= *p << c;
    }
}

/*
 * считать шестнадцатеричное число 0xZZZ
 */
void gethnum ()
{
    register int c;
    register char *cp;

    c = getchar ();
    for (cp=name; ISHEX(c); c=getchar())
        *cp++ = hexdig (c);
    ungetc (c, stdin);
    intval = 0;
    for (c=0; c<32; c+=4) {
        if (--cp < name)
            return;
        intval |= *cp << c;
    }
}

void getnum (c)
    register int c;
{
    register char *cp;
    int leadingzero;
    /* считать число */
    /* 1234 1234d 1234D - десятичное */
    /* 01234 1234. 1234o 1234O - восьмеричное */
    /* 1234' 1234h 1234H - шестнадцатеричное */

    leadingzero = (c=='0');
    for (cp=name; ISHEX(c); c=getchar())
        *cp++ = hexdig (c);
    intval = 0;
    if (c=='.' || c=='o' || c=='O') {
octal:
        for (c=0; c<=27; c+=3) {
            if (--cp < name)
                return;
            intval |= *cp << c;
        }
        if (--cp < name)
            return;
        intval |= *cp << 30;
        return;
    } else if (c=='h' || c=='H' || c=='\'') {
        for (c=0; c<32; c+=4) {
            if (--cp < name)
                return;
            intval |= *cp << c;
        }
        return;
    } else if (c!='d' && c!='D') {
        ungetc (c, stdin);
        if (leadingzero)
            goto octal;
    }
    for (c=1; ; c*=10) {
        if (--cp < name)
            return;
        intval += *cp * c;
    }
}

void getname (c)
    register int c;
{
    register char *cp;

    for (cp=name; ISLETTER (c) || ISDIGIT (c); c=getchar())
        *cp++ = c;
    *cp = 0;
    ungetc (c, stdin);
}

int lookacmd ()
{
    switch (name [1]) {
    case 'a':
        if (! strcmp (".ascii", name)) return (ASCII);
        break;
    case 'b':
        if (! strcmp (".bss", name)) return (BSS);
        break;
    case 'c':
        if (! strcmp (".comm", name)) return (COMM);
        break;
    case 'd':
        if (! strcmp (".data", name)) return (DATA);
        break;
    case 'e':
        if (! strcmp (".equ", name)) return (EQU);
        break;
    case 'g':
        if (! strcmp (".globl", name)) return (GLOBL);
        break;
    case 's':
        if (! strcmp (".short", name)) return (SHORT);
        if (! strcmp (".strng", name)) return (STRNG);
        break;
    case 't':
        if (! strcmp (".text", name)) return (TEXT);
        break;
    case 'w':
        if (! strcmp (".word", name)) return (WORD);
        break;
    }
    return (-1);
}

int lookcmd ()
{
    register int i, h;

    h = chash (name);
    while ((i = hashctab[h]) != -1) {
        if (!strcmp (table[i].name, name))
            return (i);
        if (--h < 0)
            h += HCMDSZ;
    }
    return (-1);
}

int hash (s)
    register char *s;
{
    register int h, c;

    h = 12345;
    while ((c = *s++) != 0)
        h += h + c;
    return (SUPERHASH (h, HASHSZ-1));
}

char *alloc (len)
{
    register int r;

    r = lastfree;
    if ((lastfree += len) > SPACESZ)
        uerror ("out of memory");
    return (space + r);
}

int lookname ()
{
    register int i, h;

    h = hash (name);
    while ((i = hashtab[h]) != -1) {
        if (! strcmp (stab[i].n_name, name))
            return (i);
        if (--h < 0)
            h += HASHSZ;
    }

    /* занесение в таблицу нового символа */

    if ((i = stabfree++) >= STSIZE)
        uerror ("symbol table overflow");
    stab[i].n_len = strlen (name);
    stab[i].n_name = alloc (1 + stab[i].n_len);
    strcpy (stab[i].n_name, name);
    stab[i].n_value = 0;
    stab[i].n_type = 0;
    hashtab[h] = i;
    return (i);
}

/*
 * int getlex (int *val) - считать лексему, вернуть тип лексемы,
 * записать в *val ее значение.
 * Возвращаемые типы лексем:
 *      LEOL    - конец строки. Значение - номер начавшейся строки.
 *      LEOF    - конец файла.
 *      LNUM    - целое число. Значение - в intval, *val не определено.
 *      LCMD    - машинная команда. Значение - ее индекс в table.
 *      LNAME   - идентификатор. Значение - индекс в stab.
 *      LACMD   - инструкция ассемблера. Значение - тип.
 *      LLCMD   - длинноадресная команда. Значение - код.
 *      LSCMD   - короткоадресная команда. Значение - код.
 */
int getlex (pval)
    register int *pval;
{
    register int c;

    if (blexflag) {
        blexflag = 0;
        *pval = blextype;
        return (backlex);
    }
    for (;;) {
        switch (c = getchar()) {
        case ';':
skiptoeol:  while ((c = getchar()) != '\n')
                if (c == EOF)
                    return (LEOF);
        case '\n':
            c = getchar ();
            if (c == '#')
                goto skiptoeol;
            ungetc (c, stdin);
            *pval = ++line;
            return (LEOL);
        case ' ':
        case '\t':
            continue;
        case EOF:
            return (LEOF);
        case '\\':
            c = getchar ();
            if (c=='<')
                return (LLSHIFT);
            if (c=='>')
                return (LRSHIFT);
            ungetc (c, stdin);
            return ('\\');
        case '+':
            if ((c = getchar ()) == '+')
                return (LINCR);
            ungetc (c, stdin);
            return ('+');
        case '-':
            if ((c = getchar ()) == '-')
                return (LINCR);
            ungetc (c, stdin);
            return ('-');
        case '^':       case '&':       case '|':       case '~':
        case '#':       case '*':       case '/':       case '%':
        case '"':       case ',':       case '[':       case ']':
        case '(':       case ')':       case '{':       case '}':
        case '<':       case '>':       case '=':       case ':':
            return (c);
        case '\'':
            getlhex (c);
            return (LNUM);
        case '0':
            if ((c = getchar ()) == 'x' || c=='X') {
                gethnum ();
                return (LNUM);
            }
            ungetc (c, stdin);
            c = '0';
        case '1':       case '2':       case '3':
        case '4':       case '5':       case '6':       case '7':
        case '8':       case '9':
            getnum (c);
            return (LNUM);
        case '@':
        case '$':
            *pval = hexdig (getchar ());
            *pval = *pval<<4 | hexdig (getchar ());
            return (c=='$' ? LSCMD : LLCMD);
        default:
            if (!ISLETTER (c))
                uerror ("bad character: \\%o", c & 0377);
            if (c=='.') {
                c = getchar();
                if (c == '[') {
                    getbitmask ();
                    return (LNUM);
                } else if (ISOCTAL (c)) {
                    getbitnum (c);
                    return (LNUM);
                }
                ungetc (c, stdin);
                c = '.';
            }
            getname (c);
            if (name[0]=='.') {
                if (name[1] == 0)
                    return ('.');
                if ((*pval = lookacmd()) != -1)
                    return (LACMD);
            }
            if ((*pval = lookcmd()) != -1)
                return (LCMD);
            *pval = lookname ();
            return (LNAME);
        }
    }
}

void ungetlex (val, type)
{
    blexflag = 1;
    backlex = val;
    blextype = type;
}

int getterm ()
{
    register int ty;
    int cval, s;

    switch (getlex (&cval)) {
    default:
        uerror ("operand missed");
    case LNUM:
        return (SABS);
    case LNAME:
        intval = 0;
        ty = stab[cval].n_type & N_TYPE;
        if (ty==N_UNDF || ty==N_COMM) {
            extref = cval;
            return (SEXT);
        }
        intval = stab[cval].n_value;
        return (typesegm [ty]);
    case '.':
        intval = count[segm];
        return (segm);
    case '(':
        getexpr (&s);
        if (getlex (&cval) != ')')
            uerror ("bad () syntax");
        return (s);
    }
}

/*
 * считать число .N, где N - номер бита
 */
void getbitnum (c)
    register int c;
{
    getnum (c);
    c = intval;
    if (c < 0 || c >= 64)
        uerror ("bit number out of range 0..31");
    intval = 1 << c;
}

/*
 * считать число .[a:b], где a, b - номер бита
 * или .[a=b]
 */
void getbitmask ()
{
    register int c, a, b;
    int v, compl;

    a = getexpr (&v) - 1;
    if (v != SABS)
        uerror ("illegal expression in bit mask");
    c = getlex (&v);
    if (c != ':' && c != '=')
        uerror ("illegal bit mask delimiter");
    compl = c == '=';
    b = getexpr (&v) - 1;
    if (v != SABS)
        uerror ("illegal expression in bit mask");
    c = getlex (&v);
    if (c != ']')
        uerror ("illegal bit mask delimiter");
    if (a<0 || a>=64 || b<0 || b>=64)
        uerror ("bit number out of range 1..64");
    if (a < b)
        c = a, a = b, b = c;
    if (compl && --a < ++b) {
        intval = ~0;
        return;
    }
    /* a greater than or equal to b */
    intval = (unsigned) ~0 >> (31-a+b) << b;
    if (compl)
        intval ^= ~0;
}

/*
 * unsigned getexpr (int *s) - считать выражение.
 * Вернуть значение, номер базового сегмента записать в *s.
 * Возвращаются 4 младших байта значения,
 * полная копия остается в intval.
 *
 * выражение    = [операнд] {операция операнд}...
 * операнд      = LNAME | LNUM | "." | "(" выражение ")" | "{" выражение "}"
 * операция     = "+" | "-" | "&" | "|" | "^" | "~" | "\" | "/" | "*" | "%"
 */
unsigned getexpr (s)
    register int *s;
{
    register int clex;
    int cval, s2;
    unsigned rez;

    /* смотрим первую лексему */
    switch (clex = getlex (&cval)) {
    default:
        ungetlex (clex, cval);
        rez = 0;
        *s = SABS;
        break;
    case LNUM:
    case LNAME:
    case '.':
    case '(':
    case '{':
        ungetlex (clex, cval);
        *s = getterm ();
        rez = intval;
        break;
    }
    for (;;) {
        switch (clex = getlex (&cval)) {
        case '+':
            s2 = getterm ();
            if (*s == SABS)
                *s = s2;
            else if (s2 != SABS)
                uerror ("too complex expression");
            rez += intval;
            break;
        case '-':
            s2 = getterm ();
            if (s2 != SABS)
                uerror ("too complex expression");
            rez -= intval;
            break;
        case '&':
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez &= intval;
            break;
        case '|':
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez |= intval;
            break;
        case '^':
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez ^= intval;
            break;
        case '~':
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez ^= ~intval;
            break;
        case LLSHIFT:           /* сдвиг влево */
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez <<= intval & 037;
            break;
        case LRSHIFT:           /* сдвиг вправо */
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez >>= intval & 037;
            break;
        case '*':
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez *= intval;
            break;
        case '/':
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            if (intval == 0)
                uerror ("division by zero");
            rez /= intval;
            break;
        case '%':
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            if (intval == 0)
                uerror ("division (%%) by zero");
            rez %= intval;
            break;
        default:
            ungetlex (clex, cval);
            intval = rez;
            return (rez);
        }
    }
    /* NOTREACHED */
}

void emitword (w, r)
    register unsigned w;
    register unsigned r;
{
    fputword (w, sfile[segm]);
    fputword (r, rfile[segm]);
    count[segm] += WORDSZ;
}

void makecmd (val, type)
    unsigned val;
{
    register int clex, index, incr;
    register unsigned addr, reltype;
    int cval, segment;

    index = regleft;
    reltype = RABS;
    for (;;) {
        switch (clex = getlex (&cval)) {
        case LEOF:
        case LEOL:
            ungetlex (clex, cval);
            addr = 0;
            goto putcom;
        case '[':
            makecmd (WTCCOM, TLONG);
            if (getlex (&cval) != ']')
                uerror ("bad [] syntax");
            continue;
        case '<':
            makecmd (UTCCOM, TLONG);
            if (getlex (&cval) != '>')
                uerror ("bad <> syntax");
            continue;
        default:
            ungetlex (clex, cval);
            addr = getexpr (&segment);
            reltype = segmrel [segment];
            if (reltype == REXT)
                reltype |= RSETINDEX (extref);
            break;
        }
        break;
    }
    if ((clex = getlex (&cval)) == ',') {
        index = getexpr (&segment);
        if (segment != SABS)
            uerror ("bad register number");
        if ((type & TCOMP) && addr==0 && reltype==RABS) {
            if ((clex = getlex (&cval)) == LINCR || clex==LDECR) {
                incr = getexpr (&segment);
                if (segment != SABS)
                    uerror ("bad register increment");
                if (incr == 0)
                    incr = 1;
                /* делаем компонентную команду */
                addr = clex==LINCR ? incr : -incr;
                val = MAKECOMP (val);
            } else
                ungetlex (clex, cval);
        }
    } else
        ungetlex (clex, cval);
putcom:
    if (type & TLONG) {
        addr &= 0xfffff;
        emitword ((index << 28) | val | (addr & 0xfffff),
            reltype | RLONG);
    } else {
        emitword ((index << 28) | val | (addr & 07777),
            reltype | RSHORT);
    }
}

void makeascii ()
{
    register int c, n;
    int cval;

    c = getlex (&cval);
    if (c != '"')
        uerror ("no .ascii parameter");
    n = 0;
    for (;;) {
        switch (c = getchar ()) {
        case EOF:
            uerror ("EOF in text string");
        case '"':
            break;
        case '\\':
            switch (c = getchar ()) {
            case EOF:
                uerror ("EOF in text string");
            case '\n':
                continue;
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
                cval = c & 07;
                c = getchar ();
                if (c>='0' && c<='7') {
                    cval = (cval << 3) | (c & 7);
                    c = getchar ();
                    if (c>='0' && c<='7') {
                        cval = (cval << 3) | (c & 7);
                    } else
                        ungetc (c, stdin);
                } else
                    ungetc (c, stdin);
                c = cval;
                break;
            case 't':
                c = '\t';
                break;
            case 'b':
                c = '\b';
                break;
            case 'r':
                c = '\r';
                break;
            case 'n':
                c = '\n';
                break;
            case 'f':
                c = '\f';
                break;
            }
        default:
            fputc (c, sfile[segm]);
            n++;
            continue;
        }
        break;
    }
    c = WORDSZ - n % WORDSZ;
    count[segm] += n + c;
    n = (n + c) / WORDSZ;
    while (c--)
        fputc (0, sfile[segm]);
    while (n--)
        fputword (0L, rfile[segm]);
}

void pass1 ()
{
    register int clex;
    int cval, tval, csegm;
    register unsigned addr;

    segm = STEXT;
    while ((clex = getlex (&cval)) != LEOF) {
        switch (clex) {
        case LEOF:
            return;
        case LEOL:
            regleft = 0;
            continue;
        case ':':
            continue;
        case LNUM:
            ungetlex (clex, cval);
            getexpr (&cval);
            if (cval != SABS)
                uerror ("bad register number");
            regleft = intval & 017;
            continue;
        case LCMD:
            makecmd (table[cval].val, table[cval].type);
            break;
        case LSCMD:
            makecmd (cval << 12 | 0x3f00000L, 0);
            break;
        case LLCMD:
            makecmd (cval << 20, TLONG);
            break;
        case '.':
            if (getlex (&cval) != '=')
                uerror ("bad command");
            addr = 2 * getexpr (&csegm);
            if (csegm != segm)
                uerror ("bad count assignment");
            if (addr < count[segm])
                uerror ("negative count increment");
            if (segm == SBSS)
                count [segm] = addr;
            else {
                while (count[segm] < addr) {
                    fputword (segm==STEXT? EMPCOM: 0L, sfile[segm]);
                    fputword (0L, rfile[segm]);
                    count[segm]++;
                }
            }
            break;
        case LNAME:
            if ((clex = getlex (&tval)) == ':') {
                stab[cval].n_value = count[segm] / 2;
                stab[cval].n_type &= ~N_TYPE;
                stab[cval].n_type |= segmtype [segm];
                continue;
            } else if (clex=='=' || (clex==LACMD && tval==EQU)) {
                stab[cval].n_value = getexpr (&csegm);
                if (csegm == SEXT)
                    uerror ("indirect equivalence");
                stab[cval].n_type &= N_EXT;
                stab[cval].n_type |= segmtype [csegm];
                break;
            } else if (clex==LACMD && tval==COMM) {
                /* name .comm len */
                if (stab[cval].n_type != N_UNDF &&
                    stab[cval].n_type != (N_EXT|N_COMM))
                    uerror ("name already defined");
                stab[cval].n_type = N_EXT | N_COMM;
                getexpr (&tval);
                if (tval != SABS)
                    uerror ("bad length .comm");
                stab[cval].n_value = intval;
                break;
            }
            uerror ("bad command");
        case LACMD:
            switch (cval) {
            case TEXT:
                segm = STEXT;
                break;
            case DATA:
                segm = SDATA;
                break;
            case STRNG:
                segm = SSTRNG;
                break;
            case BSS:
                segm = SBSS;
                break;
            case SHORT:
                for (;;) {
                    getexpr (&cval);
                    addr = segmrel [cval];
                    if (cval == SEXT)
                        addr |= RSETINDEX (extref);
                    emitword (intval, addr);
                    if ((clex = getlex (&cval)) != ',') {
                        ungetlex (clex, cval);
                        break;
                    }
                }
                break;
            case WORD:
                for (;;) {
                    getexpr (&cval);
                    addr = segmrel [cval];
                    if (cval == SEXT)
                        addr |= RSETINDEX (extref);
                    fputword (intval, sfile[segm]);
                    fputword (addr, rfile[segm]);
                    count[segm] += 2;
                    if ((clex = getlex (&cval)) != ',') {
                        ungetlex (clex, cval);
                        break;
                    }
                }
                break;
            case ASCII:
                makeascii ();
                break;
            case GLOBL:
                for (;;) {
                    if ((clex = getlex (&cval)) != LNAME)
                        uerror ("bad parameter .globl");
                    stab[cval].n_type |= N_EXT;
                    if ((clex = getlex (&cval)) != ',') {
                        ungetlex (clex, cval);
                        break;
                    }
                }
                break;
            case COMM:
                /* .comm name,len */
                tval = cval;
                if (getlex (&cval) != LNAME)
                    uerror ("bad parameter .comm");
                if (stab[cval].n_type != N_UNDF &&
                    stab[cval].n_type != (N_EXT|N_COMM))
                    uerror ("name already defined");
                stab[cval].n_type = N_EXT | N_COMM;
                if ((clex = getlex (&tval)) == ',') {
                    getexpr (&tval);
                    if (tval != SABS)
                        uerror ("bad length .comm");
                } else {
                    ungetlex (clex, cval);
                    intval = 1;
                }
                stab[cval].n_value = intval;
                break;
            }
            break;
        default:
            uerror ("bad syntax");
        }
        if ((clex = getlex (&cval)) != LEOL) {
            if (clex == LEOF)
                return;
            uerror ("bad command end");
        }
        regleft = 0;
    }
}

void middle ()
{
    register int i, snum;

    stlength = 0;
    for (snum=0, i=0; i<stabfree; i++) {
        /* если не установлен флаг uflag,
         * неопределенное имя считается внешним */
        if (stab[i].n_type == N_UNDF) {
            if (uflag)
                uerror ("name undefined", stab[i].n_name);
            stab[i].n_type |= N_EXT;
        }
        if (xflags)
            newindex[i] = snum;
        if (! xflags || (stab[i].n_type & N_EXT) ||
            (Xflag && stab[i].n_name[0] != 'L'))
        {
            stlength += 2 + WORDSZ + stab[i].n_len;
            snum++;
        }
    }
    stalign = WORDSZ - stlength % WORDSZ;
    stlength += stalign;
}

void makeheader ()
{
    struct exec hdr;

    hdr.a_magic = OMAGIC;
    hdr.a_text = count [STEXT];
    hdr.a_data = count [SDATA] + count [SSTRNG];
    hdr.a_bss = count [SBSS];
    hdr.a_syms = stlength;
    hdr.a_entry = USER_CODE_START;
    hdr.a_flag = 0;
    fputhdr (&hdr, stdout);
}

unsigned adjust (h, a, hr)
    register unsigned h, a;
    register int hr;
{
    switch (hr & RSHORT) {
    case 0:
        a += h & 0777777777;
        h &= ~0777777777;
        h |= a & 0777777777;
        break;
    case RSHORT:
        a += h & 07777;
        h &= ~07777;
        h |= a & 07777;
        break;
    case RSHIFT:
        a >>= 12;
        goto rlong;
    case RTRUNC:
        a &= 07777;
    case RLONG:
rlong:  a += h & 0xfffff;
        h &= ~0xfffff;
        h |= a & 0xfffff;
        break;
    }
    return (h);
}

unsigned makeword (h, hr)
    register unsigned h, hr;
{
    register int i;

    switch ((int) hr & REXT) {
    case RABS:
        break;
    case RTEXT:
        h = adjust (h, tbase, (int) hr);
        break;
    case RDATA:
        h = adjust (h, dbase, (int) hr);
        break;
    case RSTRNG:
        h = adjust (h, adbase, (int) hr);
        break;
    case RBSS:
        h = adjust (h, bbase, (int) hr);
        break;
    case REXT:
        i = RINDEX (hr);
        if (stab[i].n_type != N_EXT+N_UNDF &&
            stab[i].n_type != N_EXT+N_COMM)
            h = adjust (h, stab[i].n_value, (int) hr);
        break;
    }
    return (h);
}

void pass2 ()
{
    register int i;
    register unsigned h;

    tbase = 0x7f008000; // TODO: user code start
    dbase = tbase + count[STEXT]/2;
    adbase = dbase + count[SDATA]/2;
    bbase = adbase + count[SSTRNG]/2;

    /* обработка таблицы символов */
    for (i=0; i<stabfree; i++) {
        h = stab[i].n_value;
        switch (stab[i].n_type & N_TYPE) {
        case N_UNDF:
        case N_ABS:
            break;
        case N_TEXT:
            h = adjust (h, tbase, 0);
            break;
        case N_DATA:
            h = adjust (h, dbase, 0);
            break;
        case N_STRNG:
            h = adjust (h, adbase, 0);
            stab[i].n_type += N_DATA - N_STRNG;
            break;
        case N_BSS:
            h = adjust (h, bbase, 0);
            break;
        }
        stab[i].n_value = h;
    }
    for (segm=STEXT; segm<SBSS; segm++) {
        rewind (sfile [segm]);
        rewind (rfile [segm]);
        h = count [segm];
        while (h--)
            fputword (makeword (fgetword (sfile[segm]),
                fgetword (rfile[segm])), stdout);
    }
}

/*
 * преобразование типа символа в тип настройки
 */
int typerel (t)
{
    switch (t & N_TYPE) {
    case N_ABS:     return (RABS);
    case N_TEXT:    return (RTEXT);
    case N_DATA:    return (RDATA);
    case N_BSS:     return (RBSS);
    case N_STRNG:   return (RDATA);
    case N_UNDF:
    case N_COMM:
    case N_FN:
    default:        return (0);
    }
}

unsigned relword (hr)
    register long hr;
{
    register int i;

    switch ((int) hr & REXT) {
    case RSTRNG:
        hr = RDATA | (hr & RSHORT);
        break;
    case REXT:
        i = RINDEX (hr);
        if (stab[i].n_type == N_EXT+N_UNDF ||
            stab[i].n_type == N_EXT+N_COMM)
        {
            /* переиндексация */
            if (xflags)
                hr = (hr & (RSHORT|REXT)) | RSETINDEX (newindex [i]);
        } else
            hr = (hr & RSHORT) | typerel (stab[i].n_type);
        break;
    }
    return (hr);
}

void makereloc ()
{
    register unsigned len;

    for (segm=STEXT; segm<SBSS; segm++) {
        rewind (rfile [segm]);
        len = count [segm];
        while (len--)
            fputword (relword (fgetword (rfile[segm])), stdout);
    }
}

void makesymtab ()
{
    register int i;

    for (i=0; i<stabfree; i++) {
        if (! xflags || (stab[i].n_type & N_EXT) ||
            (Xflag && stab[i].n_name[0] != 'L'))
        {
            fputsym (&stab[i], stdout);
        }
    }
    while (stalign--)
        putchar (0);
}

int main (argc, argv)
    register char *argv[];
{
    register int i;
    register char *cp;
    int ofile = 0;

    /*
     * разбор флагов
     */
    for (i=1; i<argc; i++) {
        switch (argv[i][0]) {
        case '-':
            for (cp=argv[i]; *cp; cp++) {
                switch (*cp) {
                case 'd':       /* флаг отладки */
                    debug++;
                    break;
                case 'X':
                    Xflag++;
                case 'x':
                    xflags++;
                    break;
                case 'a':       /* не выравнивать на границу слова */
                    aflag++;
                    break;
                case 'u':
                    uflag++;
                    break;
                case 'o':       /* выходной файл */
                    if (ofile)
                        uerror ("too many -o flags");
                    ofile = 1;
                    if (cp [1]) {
                        /* -ofile */
                        outfile = cp+1;
                        while (*++cp);
                        --cp;
                    } else if (i+1 < argc)
                        /* -o file */
                        outfile = argv[++i];
                    break;
                }
            }
            break;
        default:
            if (infile)
                uerror ("too many input files");
            infile = argv[i];
            break;
        }
    }

    /*
     * настройка ввода-вывода
     */
    if (infile && ! freopen (infile, "r", stdin))
        uerror ("cannot open %s", infile);
    if (! freopen (outfile, "w", stdout))
        uerror ("cannot open %s", outfile);

    i = getchar ();
    ungetc (i=='#' ? ';' : i, stdin);

    startup ();     /* открытие временных файлов */
    hashinit ();    /* инициализация хэш-таблиц */
    pass1 ();       /* первый проход */
    middle ();      /* промежуточные действия */
    makeheader ();  /* запись заголовка */
    pass2 ();       /* второй проход */
    makereloc ();   /* запись файлов настройки */
    makesymtab ();  /* запись таблицы символов */
    exit (0);
}
