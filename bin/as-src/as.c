/*
 * Assembler for MIPS.
 *
 * Copyright (C) 2011 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <a.out.h>

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
 * Flags of instruction formats.
 */
#define FRD1    (1 << 0)        /* rd, ... */
#define FRD2    (1 << 1)        /* .., rd, ... */
#define FRT1    (1 << 2)        /* rt, ... */
#define FRT2    (1 << 3)        /* .., rt, ... */
#define FRT3    (1 << 4)        /* .., .., rt */
#define FRS1    (1 << 5)        /* rs, ... */
#define FRS2    (1 << 6)        /* .., rs, ... */
#define FRS3    (1 << 7)        /* .., .., rs */
#define FRSB    (1 << 8)        /* ... (rs) */
#define FCODE   (1 << 9)        /* immediate shifted <<6 */
#define FOFF16  (1 << 11)       /* 16-bit relocatable offset */
#define FHIGH16 (1 << 12)       /* high 16-bit relocatable offset */
#define FOFF18  (1 << 13)       /* 18-bit relocatable offset shifted >>2 */
#define FAOFF18 (1 << 14)       /* 18-bit relocatable absolute offset shifted >>2 */
#define FAOFF28 (1 << 15)       /* 28-bit relocatable absolute offset shifted >>2 */
#define FSA     (1 << 16)       /* 5-bit shift amount */
#define FSEL    (1 << 17)       /* optional 3-bit COP0 register select */
#define FSIZE   (1 << 18)       /* bit field size */

/*
 * Sizes of tables.
 * Hash sizes should be powers of 2!
 */
#define HASHSZ  2048            /* symbol name hash table size */
#define HCMDSZ  1024            /* instruction hash table size */
#define STSIZE  (HASHSZ*9/10)   /* symbol name table size */

/*
 * On second pass, hashtab[] is not needed.
 * We use it under name newindex[] to reindex symbol references
 * when -x or -X options are enabled.
 */
#define newindex hashtab

/*
 * Convert segment id to symbol type.
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
 * Convert segment id to relocation type.
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
 * Convert symbol type to segment id.
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
    { 0x00000020,   "add",      FRD1 | FRS2 | FRT3 },
    { 0x20000000,   "addi",     FRT1 | FRS2 | FOFF16 },
    { 0x24000000,   "addiu",    FRT1 | FRS2 | FOFF16 },
    { 0x00000021,   "addu",     FRD1 | FRS2 | FRT3 },
    { 0x00000024,   "and",      FRD1 | FRS2 | FRT3 },
    { 0x30000000,   "andi",     FRT1 | FRS2 | FOFF16 },
    { 0x10000000,   "b",        FAOFF18 },
    { 0x04110000,   "bal",      FAOFF18 },
    { 0x10000000,   "beq",      FRS1 | FRT2 | FOFF18 },
    { 0x50000000,   "beql",	FRS1 | FRT2 | FOFF18 },
    { 0x04010000,   "bgez",	FRS1 | FOFF18 },
    { 0x04110000,   "bgezal",	FRS1 | FOFF18 },
    { 0x04130000,   "bgezall",	FRS1 | FOFF18 },
    { 0x04030000,   "bgezl",	FRS1 | FOFF18 },
    { 0x1c000000,   "bgtz",	FRS1 | FOFF18 },
    { 0x5c000000,   "bgtzl",	FRS1 | FOFF18 },
    { 0x18000000,   "blez",	FRS1 | FOFF18 },
    { 0x58000000,   "blezl",	FRS1 | FOFF18 },
    { 0x04000000,   "bltz",	FRS1 | FOFF18 },
    { 0x04100000,   "bltzal",	FRS1 | FOFF18 },
    { 0x04120000,   "bltzall",	FRS1 | FOFF18 },
    { 0x04020000,   "bltzl",	FRS1 | FOFF18 },
    { 0x14000000,   "bne",	FRS1 | FRT2 | FOFF18 },
    { 0x54000000,   "bnel",	FRS1 | FRT2 | FOFF18 },
    { 0x0000000d,   "break",	FCODE },
    { 0x70000021,   "clo",	FRD1 | FRS2 },
    { 0x70000020,   "clz",	FRD1 | FRS2 },
    { 0x4200001f,   "deret",	0 },
    { 0x41606000,   "di",	FRT1 },
    { 0x0000001a,   "div",	FRS1 | FRT2 },
    { 0x0000001b,   "divu",	FRS1 | FRT2 },
    { 0x000000c0,   "ehb",	0 },
    { 0x41606020,   "ei",	FRT1 },
    { 0x42000018,   "eret",	0 },
    { 0x7c000000,   "ext",	FRT1 | FRS2 | FSA | FSIZE },
    { 0x7c000004,   "ins",	FRT1 | FRS2 | FSA | FSIZE },
    { 0x08000000,   "j",	FAOFF28 },
    { 0x0c000000,   "jal",	FAOFF28 },
    { 0x00000009,   "jalr",	FRD1 | FRS2 },
    { 0x00000409,   "jalr.hb",	FRD1 | FRS2 },
    { 0x00000008,   "jr",	FRS1 },
    { 0x00000408,   "jr.hb",	FRS1 },
    { 0x80000000,   "lb",	FRT1 | FOFF16 | FRSB },
    { 0x90000000,   "lbu",	FRT1 | FOFF16 | FRSB },
    { 0x84000000,   "lh",	FRT1 | FOFF16 | FRSB },
    { 0x94000000,   "lhu",	FRT1 | FOFF16 | FRSB },
    { 0xc0000000,   "ll",	FRT1 | FOFF16 | FRSB },
    { 0x3c000000,   "lui",	FRT1 | FHIGH16 },
    { 0x8c000000,   "lw",	FRT1 | FOFF16 | FRSB },
    { 0x88000000,   "lwl",	FRT1 | FOFF16 | FRSB },
    { 0x98000000,   "lwr",	FRT1 | FOFF16 | FRSB },
    { 0x70000000,   "madd",	FRS1 | FRT2 },
    { 0x70000001,   "maddu",	FRS1 | FRT2 },
    { 0x40000000,   "mfc0",	FRT1 | FRD2 | FSEL },
    { 0x00000010,   "mfhi",	FRD1 },
    { 0x00000012,   "mflo",	FRD1 },
    { 0x0000000b,   "movn",	FRD1 | FRS2 | FRT3 },
    { 0x0000000a,   "movz",	FRD1 | FRS2 | FRT3 },
    { 0x70000004,   "msub",	FRS1 | FRT2 },
    { 0x70000005,   "msubu",	FRS1 | FRT2 },
    { 0x40800000,   "mtc0",	FRT1 | FRD2 | FSEL },
    { 0x00000011,   "mthi",	FRS1 },
    { 0x00000013,   "mtlo",	FRS1 },
    { 0x70000002,   "mul",	FRD1 | FRS2 | FRT3 },
    { 0x00000018,   "mult",	FRS1 | FRT2 },
    { 0x00000019,   "multu",	FRS1 | FRT2 },
    { 0x00000000,   "nop",	0 },
    { 0x00000027,   "nor",	FRD1 | FRS2 | FRT3 },
    { 0x00000025,   "or",	FRD1 | FRS2 | FRT3 },
    { 0x34000000,   "ori",	FRT1 | FRS2 | FOFF16 },
    { 0x7c00003b,   "rdhwr",	FRT1 | FRD2 },
    { 0x41400000,   "rdpgpr",	FRD1 | FRT2 },
    { 0x00200002,   "rotr",	FRD1 | FRT2 | FSA },
    { 0x00000046,   "rotrv",	FRD1 | FRT2 | FRS3 },
    { 0xa0000000,   "sb",	FRT1 | FOFF16 | FRSB },
    { 0xe0000000,   "sc",	FRT1 | FOFF16 | FRSB },
    { 0x7000003f,   "sdbbp",	FCODE },
    { 0x7c000420,   "seb",	FRD1 | FRT2 },
    { 0x7c000620,   "seh",	FRD1 | FRT2 },
    { 0xa4000000,   "sh",	FRT1 | FOFF16 | FRSB },
    { 0x00000000,   "sll",	FRD1 | FRT2 | FSA },
    { 0x00000004,   "sllv",	FRD1 | FRT2 | FRS3 },
    { 0x0000002a,   "slt",	FRD1 | FRS2 | FRT3 },
    { 0x28000000,   "slti",	FRT1 | FRS2 | FOFF16 },
    { 0x2c000000,   "sltiu",	FRT1 | FRS2 | FOFF16 },
    { 0x0000002b,   "sltu",	FRD1 | FRS2 | FRT3 },
    { 0x00000003,   "sra",	FRD1 | FRT2 | FSA },
    { 0x00000007,   "srav",	FRD1 | FRT2 | FRS3 },
    { 0x00000002,   "srl",	FRD1 | FRT2 | FSA },
    { 0x00000006,   "srlv",	FRD1 | FRT2 | FRS3 },
    { 0x00000040,   "ssnop",	0 },
    { 0x00000022,   "sub",	FRD1 | FRS2 | FRT3 },
    { 0x00000023,   "subu",	FRD1 | FRS2 | FRT3 },
    { 0xac000000,   "sw",	FRT1 | FOFF16 | FRSB },
    { 0xa8000000,   "swl",	FRT1 | FOFF16 | FRSB },
    { 0xb8000000,   "swr",	FRT1 | FOFF16 | FRSB },
    { 0x0000000f,   "sync",	FCODE },
    { 0x0000000c,   "syscall",	FCODE },
    { 0x00000034,   "teq",	FRS1 | FRT2 },
    { 0x040c0000,   "teqi",	FRS1 | FOFF16 },
    { 0x00000030,   "tge",	FRS1 | FRT2 },
    { 0x04080000,   "tgei",	FRS1 | FOFF16 },
    { 0x04090000,   "tgeiu",	FRS1 | FOFF16 },
    { 0x00000031,   "tgeu",	FRS1 | FRT2 },
    { 0x00000032,   "tlt",	FRS1 | FRT2 },
    { 0x040a0000,   "tlti",	FRS1 | FOFF16 },
    { 0x040b0000,   "tltiu",	FRS1 | FOFF16 },
    { 0x00000033,   "tltu",	FRS1 | FRT2 },
    { 0x00000036,   "tne",	FRS1 | FRT2 },
    { 0x040e0000,   "tnei",	FRS1 | FOFF16 },
    { 0x42000020,   "wait",	FCODE },
    { 0x41c00000,   "wrpgpr",	FRD1 | FRT2 },
    { 0x7c0000a0,   "wsbh",	FRD1 | FRT2 },
    { 0x00000026,   "xor",	FRD1 | FRS2 | FRT3 },
    { 0x38000000,   "xori",	FRT1 | FRS2 | FOFF16 },
    { 0,            0,          0 },
};

/*
 * Character classes.
 */
#define ISHEX(c)        (ctype[(c)&0377] & 1)
#define ISOCTAL(c)      (ctype[(c)&0377] & 2)
#define ISDIGIT(c)      (ctype[(c)&0377] & 4)
#define ISLETTER(c)     (ctype[(c)&0377] & 8)

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

unsigned fgetword (f)
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

/*
 * Read a relocation record: 1 or 4 bytes.
 */
unsigned int fgetrel (f)
    register FILE *f;
{
    register unsigned int r;

    r = getc (f);
    if ((r & RSMASK) == REXT) {
        r |= getc (f) << 8;
        r |= getc (f) << 16;
        r |= getc (f) << 24;
    }
    return r;
}

/*
 * Emit a relocation record: 1 or 4 bytes.
 * Return a written length.
 */
unsigned int fputrel (r, f)
    register unsigned int r;
    register FILE *f;
{
    putc (r, f);
    if ((r & RSMASK) != REXT) {
        return 1;
    }
    putc (r >> 8, f);
    putc (r >> 16, f);
    putc (r >> 24, f);
    return 4;
}

void fputhdr (filhdr, coutb)
    register struct exec *filhdr;
    register FILE *coutb;
{
    fputword (filhdr->a_magic, coutb);
    fputword (filhdr->a_text, coutb);
    fputword (filhdr->a_data, coutb);
    fputword (filhdr->a_bss, coutb);
    fputword (filhdr->a_reltext, coutb);
    fputword (filhdr->a_reldata, coutb);
    fputword (filhdr->a_syms, coutb);
    fputword (filhdr->a_entry, coutb);
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
        sfile [i] = fopen (tfilename, "w+");
        if (! sfile [i])
            uerror ("cannot open %s", tfilename);
        unlink (tfilename);
        rfile [i] = fopen (tfilename, "w+");
        if (! rfile [i])
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
            ++line;
            c = getchar ();
            if (c == '#')
                goto skiptoeol;
            ungetc (c, stdin);
            *pval = line;
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
 * Return a value, put a base segment id to *s.
 * A copy of value is saved in intval.
 *
 * expression = [term] {op term}...
 * term       = LNAME | LNUM | "." | "(" expression ")"
 * op         = "+" | "-" | "&" | "|" | "^" | "~" | "<<" | ">>" | "/" | "*" | "%"
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
    fputrel (r, rfile[segm]);
    count[segm] += WORDSZ;
}

/*
 * Build and emit a machine instruction code.
 */
void makecmd (opcode, type)
    unsigned opcode;
{
    register int clex;
    register unsigned offset, relinfo;
    int cval, segment;

    offset = 0;
    relinfo = RABS;

    /*
     * First register.
     */
    if (type & FRD1) {
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rd register");
        opcode |= cval << 11;           /* rd, ... */
    }
    if (type & FRT1) {
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rt register");
        opcode |= cval << 16;           /* rt, ... */
    }
    if (type & FRS1) {
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rs register");
        opcode |= cval << 21;           /* rs, ... */
    }

    /*
     * Second register.
     */
    if (type & FRD2) {
        if (getlex (&cval) != ',')
            uerror ("comma expected");
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rd register");
        opcode |= cval << 11;           /* .., rd, ... */
    }
    if (type & FRT2) {
        if (getlex (&cval) != ',')
            uerror ("comma expected");
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rt register");
        opcode |= cval << 16;           /* .., rt, ... */
    }
    if (type & FRS2) {
        if (getlex (&cval) != ',')
            uerror ("comma expected");
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rs register");
        opcode |= cval << 21;           /* .., rs, ... */
    }

    /*
     * Third register.
     */
    if (type & FRT3) {
        if (getlex (&cval) != ',')
            uerror ("comma expected");
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rt register");
        opcode |= cval << 16;           /* .., .., rt */
    }
    if (type & FRS3) {
        if (getlex (&cval) != ',')
            uerror ("comma expected");
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rs register");
        opcode |= cval << 21;           /* .., .., rs */
    }

    /*
     * Immediate argument.
     */
    if (type & FSEL) {
        /* optional COP0 register select */
        clex = getlex (&cval);
        if (getlex (&cval) == ',') {
            offset = getexpr (&segment);
            if (segment != SABS)
                uerror ("absolute value required");
            opcode |= offset & 7;
        } else
            ungetlex (clex, cval);

    } else if (type & (FCODE | FSA)) {
        /* Non-relocatable offset */
        if ((type & FSA) && getlex (&cval) != ',')
            uerror ("comma expected");
        offset = getexpr (&segment);
        if (segment != SABS)
            uerror ("absolute value required");
        switch (type & (FCODE | FSA)) {
        case FCODE:                     /* immediate shifted <<6 */
            opcode |= offset << 6;
            break;
        case FSA:                       /* shift amount */
            opcode |= (offset & 0x1f) << 6;
            break;
        }
    } else if (type & (FOFF16 | FOFF18 | FAOFF18 | FAOFF28)) {
        /* Relocatable offset */
        if ((type & (FOFF16 | FOFF18)) && getlex (&cval) != ',')
            uerror ("comma expected");
        offset = getexpr (&segment);
        relinfo = segmrel [segment];
        if (relinfo == REXT)
            relinfo |= RSETINDEX (extref);
        switch (type & (FOFF16 | FOFF18 | FAOFF18 | FAOFF28)) {
        case FOFF16:                    /* low 16-bit byte address */
            opcode |= offset & 0xffff;
            break;
        case FHIGH16:                   /* high 16-bit byte address */
            opcode |= (offset >> 16) & 0xffff;
            relinfo |= RHIGH16;
            /* TODO: keep full offset in relinfo value */
            break;
        case FOFF18:                    /* 18-bit word address */
        case FAOFF18:
            opcode |= (offset >> 2) & 0xffff;
            relinfo |= RWORD16;
            break;
        case FAOFF28:                   /* 28-bit word address */
            opcode |= (offset >> 2) & 0x3ffffff;
            relinfo |= RWORD26;
            break;
        }
    }

    /*
     * Last argument.
     */
    if (type & FRSB) {
        if (getlex (&cval) != '(')
            uerror ("left par expected");
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rs register");
        if (getlex (&cval) != ')')
            uerror ("right par expected");
        opcode |= cval << 21;           /* ... (rs) */
    }
    if (type & FSIZE) {
        if (getlex (&cval) != ',')
            uerror ("comma expected");
        offset = getexpr (&segment);
        if (segment != SABS)
            uerror ("absolute value required");
        opcode |= ((offset - 1) & 0x1f) << 11; /* bit field size */
    }

    /* Output resulting values. */
    emitword (opcode, relinfo);
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
        fputrel (0, rfile[segm]);
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
                    fputrel (0, rfile[segm]);
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
                fputrel (addr, rfile[segm]);
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

void makeheader (rtsize, rdsize)
{
    struct exec hdr;

    hdr.a_magic = OMAGIC;
    hdr.a_text = count [STEXT];
    hdr.a_data = count [SDATA] + count [SSTRNG];
    hdr.a_bss = count [SBSS];
    hdr.a_reltext = rtsize;
    hdr.a_reldata = rdsize;
    hdr.a_syms = stlength;
    hdr.a_entry = 0;
    fseek (stdout, 0, 0);
    fputhdr (&hdr, stdout);
}

unsigned relocate (opcode, offset, relinfo)
    register unsigned opcode, offset;
    register int relinfo;
{
    switch (relinfo & RFMASK) {
    case 0:                             /* low 16 bits of byte address */
        offset += opcode & 0xffff;
        opcode &= ~0xffff;
        opcode |= offset & 0xffff;
        break;
    case RHIGH16:                       /* high 16 bits of byte address */
        /* TODO: keep full 32-offset in relinfo */
        offset += (opcode & 0xffff) << 16;
        opcode &= ~0xffff;
        opcode |= (offset >> 16) & 0xffff;
        break;
    case RWORD16:                       /* 16 bits of word address */
        offset += (opcode & 0xffff) << 2;
        opcode &= ~0xffff;
        opcode |= (offset >> 2) & 0xffff;
        break;
    case RWORD26:                       /* 26 bits of word address */
        offset += (opcode & 0x3ffffff) << 2;
        opcode &= ~0x3ffffff;
        opcode |= (offset >> 2) & 0x3ffffff;
        break;
    }
    return (opcode);
}

unsigned makeword (h, relinfo)
    register unsigned h, relinfo;
{
    register int i;

    switch ((int) relinfo & REXT) {
    case RABS:
        break;
    case RTEXT:
        h = relocate (h, tbase, (int) relinfo);
        break;
    case RDATA:
        h = relocate (h, dbase, (int) relinfo);
        break;
    case RSTRNG:
        h = relocate (h, adbase, (int) relinfo);
        break;
    case RBSS:
        h = relocate (h, bbase, (int) relinfo);
        break;
    case REXT:
        i = RINDEX (relinfo);
        if (stab[i].n_type != N_EXT+N_UNDF &&
            stab[i].n_type != N_EXT+N_COMM)
            h = relocate (h, stab[i].n_value, (int) relinfo);
        break;
    }
    return (h);
}

void pass2 ()
{
    register int i;
    register unsigned h;

    tbase = 0;
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
            h = relocate (h, tbase, 0);
            break;
        case N_DATA:
            h = relocate (h, dbase, 0);
            break;
        case N_STRNG:
            h = relocate (h, adbase, 0);
            stab[i].n_type += N_DATA - N_STRNG;
            break;
        case N_BSS:
            h = relocate (h, bbase, 0);
            break;
        }
        stab[i].n_value = h;
    }
    fseek (stdout, sizeof(struct exec), 0);
    for (segm=STEXT; segm<SBSS; segm++) {
        rewind (sfile [segm]);
        rewind (rfile [segm]);
        for (h=count[segm]; h>0; h-=WORDSZ)
            fputword (makeword (fgetword (sfile[segm]),
                fgetrel (rfile[segm])), stdout);
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

/*
 * Relocate a relocation info word.
 * Remap symbol indexes.
 * Put string pseudo-section to data section.
 */
unsigned relrel (relinfo)
    register long relinfo;
{
    register int i;

    switch ((int) relinfo & REXT) {
    case RSTRNG:
        relinfo = RDATA | (relinfo & RFMASK);
        break;
    case REXT:
        i = RINDEX (relinfo);
        if (stab[i].n_type == N_EXT+N_UNDF ||
            stab[i].n_type == N_EXT+N_COMM)
        {
            /* Reindexing */
            if (xflags)
                relinfo = (relinfo & (RFMASK | REXT)) | RSETINDEX (newindex [i]);
        } else
            relinfo = (relinfo & RFMASK) | typerel (stab[i].n_type);
        break;
    }
    return (relinfo);
}

/*
 * Emit a relocation info for a given segment.
 * Copy it from scratch file to output.
 * Return a size of relocation data in bytes.
 */
unsigned makereloc (s)
    register int s;
{
    register unsigned i, nbytes;
    unsigned r, n;

    if (count [s] <= 0)
        return 0;
    rewind (rfile [s]);
    nbytes = 0;
    for (i=0; i<count[s]; i+=WORDSZ) {
        r = fgetrel (rfile[s]);
        n = relrel (r);
        nbytes += fputrel (n, stdout);
    }
    while (nbytes % WORDSZ) {
        putchar (0);
        nbytes++;
    }
    return nbytes;
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
    unsigned rtsize, rdsize;

    /*
     * Parse options.
     */
    for (i=1; i<argc; i++) {
        switch (argv[i][0]) {
        case '-':
            for (cp=argv[i]+1; *cp; cp++) {
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
                default:
usage:              fprintf (stderr, "Usage: as [-uxX] [infile] [-o outfile]\n");
                    return (1);
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
    if (! infile && isatty(0))
        goto usage;

    /*
     * Setup input-output.
     */
    if (infile && ! freopen (infile, "r", stdin))
        uerror ("cannot open %s", infile);
    if (! freopen (outfile, "w", stdout))
        uerror ("cannot open %s", outfile);

    startup ();                         /* Open temporary files */
    hashinit ();                        /* Initialize hash tables */
    pass1 ();                           /* First pass */
    middle ();                          /* Prepare symbol table */
    pass2 ();                           /* Second pass */
    rtsize = makereloc (STEXT);         /* Emit relocation info */
    rdsize = makereloc (SDATA);
    makesymtab ();                      /* Emit symbol table */
    makeheader (rtsize, rdsize);        /* Write a.out header */
    exit (0);
}
