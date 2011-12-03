
#include </usr/include/stdio.h>
#include </usr/include/ctype.h>
#include </usr/include/getopt.h>
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
    h->a_magic = fgetword (text);
    h->a_text  = fgetword (text);
    h->a_data  = fgetword (text);
    h->a_bss   = fgetword (text);
    h->a_syms  = fgetword (text);
    h->a_entry = fgetword (text);
    fgetword (text);                    /* unused */
    h->a_flag  = fgetword (text);
    return (1);
}

void prrel (r)
    register long r;
{
    int indx;

    indx = RINDEX (r);
    switch ((int) r & RSMASK) {
    default:        putchar ('?');  break;
    case RTEXT:     putchar ('t');  break;
    case RDATA:     putchar ('d');  break;
    case RBSS:      putchar ('b');  break;
    case RABS:      putchar ('a');  break;
    case REXT:      printf ("%d", indx);
    }
    switch ((int) r & RFMASK) {
    case RLONG:     putchar ('l');  break;
    case RSHORT:    putchar ('s');  break;
    case RSHIFT:    putchar ('h');  break;
    case RTRUNC:    putchar ('t');  break;
    case 0:         putchar ('a');  break;
    default:        putchar ('?');  break;
    }
    printf ("(%d %d", (int) (r & RSMASK) >> 3, (int) (r & RFMASK));
    if (indx != 0)
        printf (" %d", indx);
    printf (")");
}

void prtext (n)
    register int n;
{
    unsigned opcode;

    while (n--) {
        printf ("%08x:", addr);
        opcode = fgetword (text);
        printf ("\t%08x\t", opcode);
        if (! dflag)
            print_insn_mips (addr, opcode, stdout);
        if (rflag) {
            putchar ('\t');
            prrel (fgetword (rel));
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
        printf ("%08x:\t%08x", addr++, fgetword (text));
        if (rflag) {
            putchar ('\t');
            prrel (fgetword (rel));
        }
        putchar ('\n');
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
        if (hdr.a_flag & 1) {
            fprintf (stderr, "aout: %s is not relocatable\n",
                fname);
            return;
        }
        rel = fopen (fname, "r");
        if (! rel) {
            fprintf (stderr, "aout: %s not found\n", fname);
            return;
        }
        fseek (rel, N_SYMOFF (hdr), 0);
    }
    printf ("%s: format %s\n\n", fname,
        hdr.a_magic == OMAGIC ? "OMAGIC" :
        hdr.a_magic == NMAGIC ? "NMAGIC" :
        hdr.a_magic == ZMAGIC ? "ZMAGIC" : "UNKNOWN");

    printf ("Header:\n");
    printf ("    a_magic = %08x\n", hdr.a_magic);   /* file format */
    printf ("    a_text  = %08x\n", hdr.a_text);    /* size of text segment */
    printf ("    a_data  = %08x\n", hdr.a_data);    /* size of initialized data */
    printf ("    a_bss   = %08x\n", hdr.a_bss);     /* size of uninitialized data */
    printf ("    a_syms  = %u\n", hdr.a_syms);      /* size of symbol table */
    printf ("    a_entry = %08x\n", hdr.a_entry);   /* entry point */
    printf ("    a_flag  = %08x %s\n", hdr.a_flag,  /* relocation info stripped */
        (hdr.a_flag & 1) ? "(fixed)" : "(relocatable)");

    printf ("\nSection .text:\n");
    addr = USER_CODE_START;
    prtext ((int) hdr.a_text / sizeof(int));

    printf ("\nSection .data:\n");
    prdata ((int) hdr.a_data / sizeof(int));
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
