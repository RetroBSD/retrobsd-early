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
#define WORDSZ          4               /* word size in bytes */

/*
 * Types of lexemes.
 */
enum {
    LEOF = 1,           /* end of file */
    LEOL,               /* end of line */
    LNAME,              /* identifier */
    LCMD,               /* machine instruction */
    LREG,               /* machine register */
    LNUM,               /* integer number */
    LLSHIFT,            /* << */
    LRSHIFT,            /* >> */
    LASCII,             /* .ascii */
    LBSS,               /* .bss */
    LCOMM,              /* .comm */
    LDATA,              /* .data */
    LGLOBL,             /* .globl */
    LSHORT,             /* .short */
    LSTRNG,             /* .strng */
    LTEXT,              /* .text */
    LEQU,               /* .equ */
    LWORD,              /* .word */
};

/*
 * Segment ids.
 */
enum {
    STEXT,
    SDATA,
    SSTRNG,
    SBSS,
    SEXT,
    SABS,               /* special case for getexpr() */
};

/*
 * Instruction formats.
 */
enum {
    FMT_CODE = 1,
    FMT_OFF18,
    FMT_OFF28,
    FMT_RD,
    FMT_RD_RS,
    FMT_RD_RS_RT,
    FMT_RD_RT,
    FMT_RD_RT_RS,
    FMT_RD_RT_SA,
    FMT_RR_RS_IMM16,
    FMT_RS,
    FMT_RS_IMM16,
    FMT_RS_OFF18,
    FMT_RS_RT,
    FMT_RS_RT_OFF18,
    FMT_RT,
    FMT_RT_IMM16,
    FMT_RT_OFF16_RS,
    FMT_RT_RD,
    FMT_RT_RD_SEL,
    FMT_RT_RS_POS_SIZE,
};

/*
 * Sizes of tables.
 * Hash sizes should be powers of 2!
 */
#define HASHSZ          2048            /* symbol name hash table size */
#define HCMDSZ          1024            /* instruction hash table size */
#define STSIZE          (HASHSZ*9/10)   /* symbol name table size */

#define SUPERHASH(key,mask) (((key) * 011706736335) & (mask))

#define ISHEX(c)        (ctype[(c)&0377] & 1)
#define ISOCTAL(c)      (ctype[(c)&0377] & 2)
#define ISDIGIT(c)      (ctype[(c)&0377] & 4)
#define ISLETTER(c)     (ctype[(c)&0377] & 8)

/*
 * On second pass, hashtab[] is not needed.
 * We use it under name newindex[] to reindex symbol references
 * when -x or -X options are enabled.
 */
#define newindex hashtab

const char ctype [256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,8,0,0,0,0,0,0,0,0,0,8,0,7,7,7,7,7,7,7,7,5,5,0,0,0,0,0,0,
    0,9,9,9,9,9,9,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,0,0,0,0,8,
    0,9,9,9,9,9,9,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,0,
};

/*
 * Convert segment index to symbol type.
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
 * Convert segment index to relocation type.
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
 * Convert symbol type to segment index.
 */
const int typesegm [] = {
    SEXT,               /* N_UNDF */
    SABS,               /* N_ABS */
    STEXT,              /* N_TEXT */
    SDATA,              /* N_DATA */
    SBSS,               /* N_BSS */
    SSTRNG,             /* N_STRNG */
};

struct optable {
    unsigned opcode;
    const char *name;
    unsigned type;
};

const struct optable optable [] = {
    { 0x00000000,   "add",      FMT_RD_RS_RT },
    { 0x00000000,   "addi",     FMT_RR_RS_IMM16 },
    { 0x00000000,   "addiu",    FMT_RR_RS_IMM16 },
    { 0x00000000,   "addu",     FMT_RD_RS_RT },
    { 0x00000000,   "and",      FMT_RD_RS_RT },
    { 0x00000000,   "andi",     FMT_RR_RS_IMM16 },
    { 0x00000000,   "b",        FMT_OFF18 },            // 16 bits << 2
    { 0x00000000,   "bal",      FMT_OFF18 },
    { 0x00000000,   "beq",      FMT_RS_RT_OFF18 },
    { 0x00000000,   "beql",	FMT_RS_RT_OFF18 },
    { 0x00000000,   "bgez",	FMT_RS_OFF18 },
    { 0x00000000,   "bgezal",	FMT_RS_OFF18 },
    { 0x00000000,   "bgezall",	FMT_RS_OFF18 },
    { 0x00000000,   "bgezl",	FMT_RS_OFF18 },
    { 0x00000000,   "bgtz",	FMT_RS_OFF18 },
    { 0x00000000,   "bgtzl",	FMT_RS_OFF18 },
    { 0x00000000,   "blez",	FMT_RS_OFF18 },
    { 0x00000000,   "blezl",	FMT_RS_OFF18 },
    { 0x00000000,   "bltz",	FMT_RS_OFF18 },
    { 0x00000000,   "bltzal",	FMT_RS_OFF18 },
    { 0x00000000,   "bltzall",	FMT_RS_OFF18 },
    { 0x00000000,   "bltzl",	FMT_RS_OFF18 },
    { 0x00000000,   "bne",	FMT_RS_RT_OFF18 },
    { 0x00000000,   "bnel",	FMT_RS_RT_OFF18 },
    { 0x00000000,   "break",	FMT_CODE },
    { 0x00000000,   "clo",	FMT_RD_RS },
    { 0x00000000,   "clz",	FMT_RD_RS },
    { 0x00000000,   "deret",	0 },
    { 0x00000000,   "di",	FMT_RT },
    { 0x00000000,   "div",	FMT_RS_RT },
    { 0x00000000,   "divu",	FMT_RS_RT },
    { 0x00000000,   "ehb",	0 },
    { 0x00000000,   "ei",	FMT_RT },
    { 0x00000000,   "eret",	0 },
    { 0x00000000,   "ext",	FMT_RT_RS_POS_SIZE },
    { 0x00000000,   "ins",	FMT_RT_RS_POS_SIZE },
    { 0x00000000,   "j",	FMT_OFF28 },            // 26 bits << 2
    { 0x00000000,   "jal",	FMT_OFF28 },
    { 0x00000000,   "jalr",	FMT_RD_RS },
    { 0x00000000,   "jalr.hb",	FMT_RD_RS },
    { 0x00000000,   "jr",	FMT_RS },
    { 0x00000000,   "jr.hb",	FMT_RS },
    { 0x00000000,   "lb",	FMT_RT_OFF16_RS },
    { 0x00000000,   "lbu",	FMT_RT_OFF16_RS },
    { 0x00000000,   "lh",	FMT_RT_OFF16_RS },
    { 0x00000000,   "lhu",	FMT_RT_OFF16_RS },
    { 0x00000000,   "ll",	FMT_RT_OFF16_RS },
    { 0x00000000,   "lui",	FMT_RT_IMM16 },
    { 0x00000000,   "lw",	FMT_RT_OFF16_RS },
    { 0x00000000,   "lwl",	FMT_RT_OFF16_RS },
    { 0x00000000,   "lwr",	FMT_RT_OFF16_RS },
    { 0x00000000,   "madd",	FMT_RS_RT },
    { 0x00000000,   "maddu",	FMT_RS_RT },
    { 0x00000000,   "mfc0",	FMT_RT_RD_SEL },
    { 0x00000000,   "mfhi",	FMT_RD },
    { 0x00000000,   "mflo",	FMT_RD },
    { 0x00000000,   "movn",	FMT_RD_RS_RT },
    { 0x00000000,   "movz",	FMT_RD_RS_RT },
    { 0x00000000,   "msub",	FMT_RS_RT },
    { 0x00000000,   "msubu",	FMT_RS_RT },
    { 0x00000000,   "mtc0",	FMT_RT_RD_SEL },
    { 0x00000000,   "mthi",	FMT_RS },
    { 0x00000000,   "mtlo",	FMT_RS },
    { 0x00000000,   "mul",	FMT_RD_RS_RT },
    { 0x00000000,   "mult",	FMT_RS_RT },
    { 0x00000000,   "multu",	FMT_RS_RT },
    { 0x00000000,   "nop",	0 },
    { 0x00000000,   "nor",	FMT_RD_RS_RT },
    { 0x00000000,   "or",	FMT_RD_RS_RT },
    { 0x00000000,   "ori",	FMT_RR_RS_IMM16 },
    { 0x00000000,   "rdhwr",	FMT_RT_RD },
    { 0x00000000,   "rdpgpr",	FMT_RD_RT },
    { 0x00000000,   "rotr",	FMT_RD_RT_SA },
    { 0x00000000,   "rotrv",	FMT_RD_RT_RS },
    { 0x00000000,   "sb",	FMT_RT_OFF16_RS },
    { 0x00000000,   "sc",	FMT_RT_OFF16_RS },
    { 0x00000000,   "sdbbp",	FMT_CODE },
    { 0x00000000,   "seb",	FMT_RD_RT },
    { 0x00000000,   "seh",	FMT_RD_RT },
    { 0x00000000,   "sh",	FMT_RT_OFF16_RS },
    { 0x00000000,   "sll",	FMT_RD_RT_SA },
    { 0x00000000,   "sllv",	FMT_RD_RT_RS },
    { 0x00000000,   "slt",	FMT_RD_RS_RT },
    { 0x00000000,   "slti",	FMT_RR_RS_IMM16 },
    { 0x00000000,   "sltiu",	FMT_RR_RS_IMM16 },
    { 0x00000000,   "sltu",	FMT_RD_RS_RT },
    { 0x00000000,   "sra",	FMT_RD_RT_SA },
    { 0x00000000,   "srav",	FMT_RD_RT_RS },
    { 0x00000000,   "srl",	FMT_RD_RT_SA },
    { 0x00000000,   "srlv",	FMT_RD_RT_RS },
    { 0x00000000,   "ssnop",	0 },
    { 0x00000000,   "sub",	FMT_RD_RS_RT },
    { 0x00000000,   "subu",	FMT_RD_RS_RT },
    { 0x00000000,   "sw",	FMT_RT_OFF16_RS },
    { 0x00000000,   "swl",	FMT_RT_OFF16_RS },
    { 0x00000000,   "swr",	FMT_RT_OFF16_RS },
    { 0x00000000,   "sync",	FMT_CODE },
    { 0x00000000,   "syscall",	FMT_CODE },
    { 0x00000000,   "teq",	FMT_RS_RT },
    { 0x00000000,   "teqi",	FMT_RS_IMM16 },
    { 0x00000000,   "tge",	FMT_RS_RT },
    { 0x00000000,   "tgei",	FMT_RS_IMM16 },
    { 0x00000000,   "tgeiu",	FMT_RS_IMM16 },
    { 0x00000000,   "tgeu",	FMT_RS_RT },
    { 0x00000000,   "tlt",	FMT_RS_RT },
    { 0x00000000,   "tlti",	FMT_RS_IMM16 },
    { 0x00000000,   "tltiu",	FMT_RS_IMM16 },
    { 0x00000000,   "tltu",	FMT_RS_RT },
    { 0x00000000,   "tne",	FMT_RS_RT },
    { 0x00000000,   "tnei",	FMT_RS_IMM16 },
    { 0x00000000,   "wait",	FMT_CODE },
    { 0x00000000,   "wrpgpr",	FMT_RD_RT },
    { 0x00000000,   "wsbh",	FMT_RD_RT },
    { 0x00000000,   "xor",	FMT_RD_RS_RT },
    { 0x00000000,   "xori",	FMT_RR_RS_IMM16 },
    { 0,            0,          0 },
};

FILE *sfile [SABS], *rfile [SABS];
unsigned count [SABS];
int segm;
char *infile, *outfile = "a.out";
char tfilename[] = "/tmp/asXXXXXX";
int line;                               /* Source line number */
int xflags, Xflag, uflag;
int stlength;                           /* Symbol table size in bytes */
int stalign;                            /* Symbol table alignment */
unsigned tbase, dbase, adbase, bbase;
struct nlist stab [STSIZE];
int stabfree;
char space [STSIZE*8];                  /* Area for symbol names */
int lastfree;                           /* Free space offset */
char name [256];
unsigned intval;
int extref;
int blexflag, backlex, blextype;
short hashtab [HASHSZ], hashctab [HCMDSZ];

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

/*
 * Suboptimal 32-bit hash function.
 * Copyright (C) 2006 Serge Vakulenko.
 */
unsigned hash_rot13 (s)
    register const char *s;
{
    register unsigned hash, c;

    hash = 0;
    while ((c = (unsigned char) *s++) != 0) {
        hash += c;
        hash -= (hash << 13) | (hash >> 19);
    }
    return hash;
}

void hashinit ()
{
    register int i, h;
    register const struct optable *p;

    for (i=0; i<HCMDSZ; i++)
        hashctab[i] = -1;
    for (p=optable; p->name; p++) {
        h = hash_rot13 (p->name) & (HCMDSZ-1);
        while (hashctab[h] != -1)
            if (--h < 0)
                h += HCMDSZ;
        hashctab[h] = p - optable;
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
 * Get hexadecimal number 'ZZZ
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
 * Get hexadecimal number 0xZZZ
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

/*
 * Get a number.
 * 1234 1234d 1234D - decimal
 * 01234 1234. 1234o 1234O - octal
 * 1234' 1234h 1234H - hexadecimal
 */
void getnum (c)
    register int c;
{
    register char *cp;
    int leadingzero;

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
        if (! strcmp (".ascii", name)) return (LASCII);
        break;
    case 'b':
        if (! strcmp (".bss", name)) return (LBSS);
        break;
    case 'c':
        if (! strcmp (".comm", name)) return (LCOMM);
        break;
    case 'd':
        if (! strcmp (".data", name)) return (LDATA);
        break;
    case 'e':
        if (! strcmp (".equ", name)) return (LEQU);
        break;
    case 'g':
        if (! strcmp (".globl", name)) return (LGLOBL);
        break;
    case 's':
        if (! strcmp (".short", name)) return (LSHORT);
        if (! strcmp (".strng", name)) return (LSTRNG);
        break;
    case 't':
        if (! strcmp (".text", name)) return (LTEXT);
        break;
    case 'w':
        if (! strcmp (".word", name)) return (LWORD);
        break;
    }
    return (-1);
}

int lookreg ()
{
    int val;
    char *cp;

    switch (name [1]) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        val = 0;
        for (cp=name+1; ISDIGIT (*cp); cp++) {
            val *= 10;
            val += *cp - '0';
        }
        if (*cp != 0)
            break;
        return val;
        break;
    case 'a':
        if (name[3] == 0) {
            switch (name[2]) {
            case '0': return 4;         /* $a0 */
            case '1': return 5;         /* $a1 */
            case '2': return 6;         /* $a2 */
            case '3': return 7;         /* $a3 */
            case 't': return 1;         /* $at */
            }
        }
        break;
    case 'f':
        if (name[3] == 0 && name[2] == 'p')
            return 30;                  /* $fp */
        break;
    case 'g':
        if (name[3] == 0 && name[2] == 'p')
            return 28;                  /* $gp */
        break;
    case 'k':
        if (name[3] == 0) {
            switch (name[2]) {
            case '0': return 26;        /* $k0 */
            case '1': return 27;        /* $k1 */
            }
        }
        break;
    case 'r':
        if (name[3] == 0 && name[2] == 'a')
            return 31;                  /* $ra */
        break;
    case 's':
        if (name[3] == 0) {
            switch (name[2]) {
            case '0': return 16;        /* $s0 */
            case '1': return 17;        /* $s1 */
            case '2': return 18;        /* $s2 */
            case '3': return 19;        /* $s3 */
            case '4': return 20;        /* $s4 */
            case '5': return 21;        /* $s5 */
            case '6': return 22;        /* $s6 */
            case '7': return 23;        /* $s7 */
            case '8': return 30;        /* $s8 */
            case 'p': return 29;        /* $sp */
            }
        }
        break;
    case 't':
        if (name[3] == 0) {
            switch (name[2]) {
            case '0': return 8;         /* $t0 */
            case '1': return 9;         /* $t1 */
            case '2': return 10;        /* $t2 */
            case '3': return 11;        /* $t3 */
            case '4': return 12;        /* $t4 */
            case '5': return 13;        /* $t5 */
            case '6': return 14;        /* $t6 */
            case '7': return 15;        /* $t7 */
            case '8': return 24;        /* $t8 */
            case '9': return 25;        /* $t9 */
            }
        }
        break;
    case 'v':
        if (name[3] == 0) {
            switch (name[2]) {
            case '0': return 2;         /* $v0 */
            case '1': return 3;         /* $v1 */
            }
        }
        break;
    case 'z':
        if (! strcmp (name+2, "ero"))
            return 0;                   /* $zero */
        break;
    }
    uerror ("bad register name '%s'", name);
    return 0;
}

int lookcmd ()
{
    register int i, h;

    h = hash_rot13 (name) & (HCMDSZ-1);
    while ((i = hashctab[h]) != -1) {
        if (! strcmp (optable[i].name, name))
            return (i);
        if (--h < 0)
            h += HCMDSZ;
    }
    return (-1);
}

char *alloc (len)
{
    register int r;

    r = lastfree;
    lastfree += len;
    if (lastfree > sizeof(space))
        uerror ("out of memory");
    return (space + r);
}

int lookname ()
{
    register int i, h;

    /* Search for symbol name. */
    h = hash_rot13 (name) & (HASHSZ-1);
    while ((i = hashtab[h]) != -1) {
        if (! strcmp (stab[i].n_name, name))
            return (i);
        if (--h < 0)
            h += HASHSZ;
    }

    /* Add a new symbol to table. */
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
 * Read a lexical element, return it's type and store a value into *val.
 * Returned type codes:
 * LEOL    - End of line.  Value is a line number.
 * LEOF    - End of file.
 * LNUM    - Integer value (into intval), *val undefined.
 * LCMD    - Machine opcode.  Value is optable[] index.
 * LNAME   - Identifier.  Value is stab[] index.
 * LREG    - Machine register.  Value is a register number.
 * LLSHIFT - << operator.
 * LRSHIFT - >> operator.
 * LASCII  - .ascii assembler instruction.
 * LBSS    - .bss assembler instruction.
 * LCOMM   - .comm assembler instruction.
 * LDATA   - .data assembler instruction.
 * LGLOBL  - .globl assembler instruction.
 * LSHORT  - .short assembler instruction.
 * LSTRNG  - .strng assembler instruction.
 * LTEXT   - .text assembler instruction.
 * LEQU    - .equ assembler instruction.
 * LWORD   - .word assembler instruction.
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
        case '#':
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
        case '+':       case '-':
        case '^':       case '&':       case '|':       case '~':
        case ';':       case '*':       case '/':       case '%':
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
        default:
            if (! ISLETTER (c))
                uerror ("bad character: \\%o", c & 0377);
            if (c == '.') {
                c = getchar();
                if (c == '[') {
                    getbitmask ();
                    return (LNUM);
                } else if (ISOCTAL (c)) {
                    getnum (c);
                    c = intval;
                    if (c < 0 || c >= 32)
                        uerror ("bit number out of range 0..31");
                    intval = 1 << c;
                    return (LNUM);
                }
                ungetc (c, stdin);
                c = '.';
            }
            getname (c);
            if (name[0] == '.') {
                if (name[1] == 0)
                    return ('.');
                *pval = lookacmd();
                if (*pval != -1)
                    return (*pval);
            }
            if (name[0] == '$') {
                *pval = lookreg();
                if (*pval != -1)
                    return (LREG);
            }
            *pval = lookcmd();
            if (*pval != -1)
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
 * Get a number .[a:b], where a, b are bit numbers 0..31
 */
void getbitmask ()
{
    register int c, a, b;
    int v;

    a = getexpr (&v) - 1;
    if (v != SABS)
        uerror ("illegal expression in bit mask");
    c = getlex (&v);
    if (c != ':')
        uerror ("illegal bit mask delimiter");
    b = getexpr (&v) - 1;
    if (v != SABS)
        uerror ("illegal expression in bit mask");
    c = getlex (&v);
    if (c != ']')
        uerror ("illegal bit mask delimiter");
    if (a<0 || a>=32 || b<0 || b>=32)
        uerror ("bit number out of range 0..31");
    if (a < b)
        c = a, a = b, b = c;
    /* a greater than or equal to b */
    intval = (unsigned) ~0 >> (31 - a + b) << b;
}

/*
 * Get an expression.
 * Return a value, put a base segment index to *s.
 * A copy of value is saved in intval.
 *
 * expression = [term] {op term}...
 * term       = LNAME | LNUM | "." | "(" expression ")"
 * op         = "+" | "-" | "&" | "|" | "^" | "~" | "\" | "/" | "*" | "%"
 */
unsigned getexpr (s)
    register int *s;
{
    register int clex;
    int cval, s2;
    unsigned rez;

    /* look a first lexeme */
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

/*
 * Build and emit a machine instruction code.
 */
void makecmd (opcode, type)
    unsigned opcode;
{
    register int clex, reg;
    register unsigned addr, reltype;
    int cval, segment;

    reg = 0;
    reltype = RABS;

    /* TODO */
    for (;;) {
        switch (clex = getlex (&cval)) {
        case LEOF:
        case LEOL:
            ungetlex (clex, cval);
            addr = 0;
            goto putcom;
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
    clex = getlex (&cval);
    if (clex == ',') {
        reg = getexpr (&segment);
        if (segment != SABS)
            uerror ("bad register number");
    } else
        ungetlex (clex, cval);
putcom:
#if 0
    if (type & TLONG) {
        addr &= 0xfffff;
        opcode |= (reg << 28) | (addr & 0xfffff);
        reltype |= RLONG;
    } else
#endif
    opcode |= (reg << 28) | (addr & 07777);

    emitword (opcode, reltype);
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
            continue;
        case ':':
            continue;
        case LCMD:
            makecmd (optable[cval].opcode, optable[cval].type);
            break;
        case '.':
            if (getlex (&cval) != '=')
                uerror ("bad instruction");
            addr = 2 * getexpr (&csegm);
            if (csegm != segm)
                uerror ("bad count assignment");
            if (addr < count[segm])
                uerror ("negative count increment");
            if (segm == SBSS)
                count [segm] = addr;
            else {
                while (count[segm] < addr) {
                    fputword (0, sfile[segm]);
                    fputword (0L, rfile[segm]);
                    count[segm]++;
                }
            }
            break;
        case LNAME:
            clex = getlex (&tval);
            if (clex == ':') {
                stab[cval].n_value = count[segm] / 2;
                stab[cval].n_type &= ~N_TYPE;
                stab[cval].n_type |= segmtype [segm];
                continue;
            } else if (clex=='=' || clex==LEQU) {
                stab[cval].n_value = getexpr (&csegm);
                if (csegm == SEXT)
                    uerror ("indirect equivalence");
                stab[cval].n_type &= N_EXT;
                stab[cval].n_type |= segmtype [csegm];
                break;
            } else if (clex==LCOMM) {
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
            uerror ("bad instruction");
        case LTEXT:
            segm = STEXT;
            break;
        case LDATA:
            segm = SDATA;
            break;
        case LSTRNG:
            segm = SSTRNG;
            break;
        case LBSS:
            segm = SBSS;
            break;
        case LSHORT:
            for (;;) {
                getexpr (&cval);
                addr = segmrel [cval];
                if (cval == SEXT)
                    addr |= RSETINDEX (extref);
                emitword (intval, addr);
                clex = getlex (&cval);
                if (clex != ',') {
                    ungetlex (clex, cval);
                    break;
                }
            }
            break;
        case LWORD:
            for (;;) {
                getexpr (&cval);
                addr = segmrel [cval];
                if (cval == SEXT)
                    addr |= RSETINDEX (extref);
                fputword (intval, sfile[segm]);
                fputword (addr, rfile[segm]);
                count[segm] += 2;
                clex = getlex (&cval);
                if (clex != ',') {
                    ungetlex (clex, cval);
                    break;
                }
            }
            break;
        case LASCII:
            makeascii ();
            break;
        case LGLOBL:
            for (;;) {
                clex = getlex (&cval);
                if (clex != LNAME)
                    uerror ("bad parameter .globl");
                stab[cval].n_type |= N_EXT;
                clex = getlex (&cval);
                if (clex != ',') {
                    ungetlex (clex, cval);
                    break;
                }
            }
            break;
        case LCOMM:
            /* .comm name,len */
            if (getlex (&cval) != LNAME)
                uerror ("bad parameter .comm");
            if (stab[cval].n_type != N_UNDF &&
                stab[cval].n_type != (N_EXT|N_COMM))
                uerror ("name already defined");
            stab[cval].n_type = N_EXT | N_COMM;
            clex = getlex (&tval);
            if (clex == ',') {
                getexpr (&tval);
                if (tval != SABS)
                    uerror ("bad length .comm");
            } else {
                ungetlex (clex, cval);
                intval = 1;
            }
            stab[cval].n_value = intval;
            break;
        default:
            uerror ("bad syntax");
        }
        clex = getlex (&cval);
        if (clex != LEOL) {
            if (clex == LEOF)
                return;
            uerror ("bad instruction arguments");
        }
    }
}

void middle ()
{
    register int i, snum;

    stlength = 0;
    for (snum=0, i=0; i<stabfree; i++) {
        /* Without -u option, undefined symbol is considered external */
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

    /* Adjust indexes in symbol name */
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
 * Convert symbol type to relocation type.
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
            /* Reindexing */
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
     * Parse options.
     */
    for (i=1; i<argc; i++) {
        switch (argv[i][0]) {
        case '-':
            for (cp=argv[i]; *cp; cp++) {
                switch (*cp) {
                case 'X':       /* strip L* locals */
                    Xflag++;
                case 'x':       /* strip local symbols */
                    xflags++;
                    break;
                case 'u':       /* treat undefines as error */
                    uflag++;
                    break;
                case 'o':       /* output file name */
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
     * Setup input-output.
     */
    if (infile && ! freopen (infile, "r", stdin))
        uerror ("cannot open %s", infile);
    if (! freopen (outfile, "w", stdout))
        uerror ("cannot open %s", outfile);

    startup ();     /* Open temporary files */
    hashinit ();    /* Initialize hash tables */
    pass1 ();       /* First pass */
    middle ();      /* Prepare symbol table */
    makeheader ();  /* Write a.out header */
    pass2 ();       /* Second pass */
    makereloc ();   /* Emit relocation data */
    makesymtab ();  /* Emit symbol table */
    exit (0);
}
