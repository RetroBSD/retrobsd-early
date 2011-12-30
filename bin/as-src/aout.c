/*
 * Utility for displaying a.out object file information.
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
#include <unistd.h>
#include <a.out.h>

#define USER_CODE_START 0x7f008000

struct exec hdr;                /* a.out header */
FILE *text, *rel;
int rflag, dflag;
int addr;

extern int print_insn_mips (unsigned memaddr,
    unsigned long int word, FILE *stream);

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

int fgethdr (text, h)
    register FILE *text;
    register struct exec *h;
{
    h->a_magic   = fgetword (text);
    h->a_text    = fgetword (text);
    h->a_data    = fgetword (text);
    h->a_bss     = fgetword (text);
    h->a_reltext = fgetword (text);
    h->a_reldata = fgetword (text);
    h->a_syms    = fgetword (text);
    h->a_entry   = fgetword (text);
    return (1);
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
 * Read a symbol table entry.
 * Return a number of bytes read, or -1 on EOF.
 * Format of symbol record:
 *  1 byte: length of name in bytes
 *  1 byte: type of symbol (N_UNDF, N_ABS, N_TEXT, etc)
 *  4 bytes: value
 *  N bytes: name
 */
int fgetsym (text, name, value, type)
    register FILE *text;
    register char *name;
    unsigned *value;
    unsigned *type;
{
    register int len;
    unsigned nbytes;

    len = getc (text);
    if (len <= 0)
        return -1;
    *type = getc (text);
    *value = fgetword (text);
    nbytes = len + 6;
    while (len-- > 0)
        *name++ = getc (text);
    *name = '\0';
    return nbytes;
}

void prrel (r)
    register unsigned r;
{
    int indx;

    printf ("<");
    indx = RINDEX (r);
    switch (r & RSMASK) {
    default:        putchar ('?');  break;
    case RABS:      putchar ('a');  break;
    case RTEXT:     putchar ('t');  break;
    case RDATA:     putchar ('d');  break;
    case RBSS:      putchar ('b');  break;
    case REXT:      printf ("%d", indx);
    }
    switch (r & RFMASK) {
    default:        putchar ('?');  break;
    case RWORD26:   putchar ('j');  break;
    case RWORD16:   putchar ('w');  break;
    case RHIGH16:   putchar ('h');  break;
    case 0:         break;
    }
    printf (">");
}

void prtext (n)
    register int n;
{
    unsigned opcode;

    while (n--) {
        printf ("    %8x:", addr);
        opcode = fgetword (text);
        printf ("\t%08x", opcode);
        if (rflag) {
            putchar (' ');
            prrel (fgetrel (rel));
        }
        if (! dflag) {
            putchar ('\t');
            print_insn_mips (addr, opcode, stdout);
        }
        putchar ('\n');

        /* Next address */
        addr += 4;
    }
}

void prdata (n)
    register int n;
{
    while (n--) {
        printf ("    %8x:\t%08x", addr++, fgetword (text));
        if (rflag) {
            putchar ('\t');
            prrel (fgetrel (rel));
        }
        putchar ('\n');
    }
}

void prsyms (nbytes)
    register int nbytes;
{
    register int n, c;
    unsigned value, type;
    char name [256];

    while (nbytes > 0) {
        n = fgetsym (text, name, &value, &type);
        if (n <= 0)
            break;
        nbytes -= n;

        switch (type & N_TYPE) {
        case N_ABS:     c = 'a'; break;
        case N_TEXT:    c = 't'; break;
        case N_DATA:    c = 'd'; break;
        case N_BSS:     c = 'b'; break;
        case N_STRNG:   c = 's'; break;
        case N_UNDF:    c = 'u'; break;
        case N_COMM:    c = 'c'; break;
        case N_FN:      c = 'f'; break;
        default:        c = '?'; break;
        }
        if (type & N_EXT)
            c += 'A' - 'a';

        if (c == 'U')
            printf ("             %c %s\n", c, name);
        else
            printf ("    %08x %c %s\n", value, c, name);
    }
}

void disasm (fname)
    register char *fname;
{
    text = fopen (fname, "r");
    if (! text) {
        fprintf (stderr, "aout: %s not found\n", fname);
        return;
    }
    if (! fgethdr (text, &hdr) || N_BADMAG (hdr)) {
        fprintf (stderr, "aout: %s not an object file\n", fname);
        return;
    }
    if (rflag) {
        if (hdr.a_magic != OMAGIC) {
            fprintf (stderr, "aout: %s is not relocatable\n",
                fname);
            rflag = 0;
        } else {
            rel = fopen (fname, "r");
            if (! rel) {
                fprintf (stderr, "aout: %s not found\n", fname);
                return;
            }
            fseek (rel, N_TRELOFF (hdr), 0);
        }
    }
    printf ("File %s:\n", fname);
    printf ("    a_magic   = %08x (%s)\n", hdr.a_magic,
        hdr.a_magic == OMAGIC ? "relocatable" :
        hdr.a_magic == XMAGIC ? "XMAGIC" :
        hdr.a_magic == NMAGIC ? "NMAGIC" : "unknown");
    printf ("    a_text    = %08x (%u bytes)\n", hdr.a_text, hdr.a_text);
    printf ("    a_data    = %08x (%u bytes)\n", hdr.a_data, hdr.a_data);
    printf ("    a_bss     = %08x (%u bytes)\n", hdr.a_bss, hdr.a_bss);
    printf ("    a_reltext = %08x (%u bytes)\n", hdr.a_reltext, hdr.a_reltext);
    printf ("    a_reldata = %08x (%u bytes)\n", hdr.a_reldata, hdr.a_reldata);
    printf ("    a_syms    = %08x (%u bytes)\n", hdr.a_syms, hdr.a_syms);
    printf ("    a_entry   = %08x\n", hdr.a_entry);

    addr = (hdr.a_magic == OMAGIC) ? 0 : USER_CODE_START;

    if (hdr.a_text > 0) {
        printf ("\nSection .text:\n");
        prtext (hdr.a_text / sizeof(int));
    }
    if (hdr.a_data > 0) {
        printf ("\nSection .data:\n");
        if (rflag)
            fseek (rel, N_DRELOFF (hdr), 0);
        prdata (hdr.a_data / sizeof(int));
    }
    if (hdr.a_syms > 0) {
        printf ("\nSymbols:\n");
        fseek (text, N_SYMOFF (hdr), 0);
        prsyms (hdr.a_syms);
    }
    printf ("\n");
}

int main (argc, argv)
    register char **argv;
{
    int ch;

    while ((ch = getopt (argc, argv, "rd")) != EOF)
        switch (ch) {
        case 'r':       /* print relocation info */
            rflag++;
            break;
        case 'd':       /* do not disassemble */
            dflag++;
            break;
        default:
usage:      fprintf (stderr, "Usage: aout [-rd] file...\n");
            return (1);
        }
    argc -= optind;
    argv += optind;
    if (argc <= 0)
        goto usage;

    for (; argc-- > 0; argv++)
        disasm (*argv);
    return (0);
}
