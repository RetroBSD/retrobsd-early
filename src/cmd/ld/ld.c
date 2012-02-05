/*
 * Linker for RetroBSD, MIPS32 architecture.
 *
 * Usage:
 *      -o filename     output file name
 *      -u symbol       'use'
 *      -e symbol       'entry'
 *      -D size         set data size
 *      -Taddress       base address of loading
 *      -llibname       library
 *      -x              discard local symbols
 *      -X              discard locals starting with 'L'
 *      -S              discard all symbols except locals and globals
 *      -s              discard all symbols
 *      -r              preserve relocation info, don't define commons
 *      -d              define commons even with rflag
 *      -t              debug tracing
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
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <a.out.h>
#include <ar.h>
#include <ranlib.h>

#define W      4               /* word size in bytes */
#define LOCSYM 'L'             /* strip local symbols 'L*' */
#define BADDR  0x7f008000      /* start address in memory */
#define SYMDEF "__.SYMDEF"

#define ishex(c)       ((c)<='9' && (c)>='0' || (c)<='F' && (c)>='A' || (c)<='f' && (c)>='a')
#define hexdig(c)      ((c)<='9' ? (c)-'0' : ((c)&7) + 9)

struct exec filhdr;             /* aout header */

#define HDRSZ sizeof(struct exec)

struct archdr {                 /* archive header */
	char *  ar_name;
	long	ar_date;
	int     ar_uid;
	int     ar_gid;
	int	ar_mode;
	long    ar_size;
} archdr;

FILE *text, *reloc;             /* input management */

                                /* output management */
FILE *outb, *toutb, *doutb, *troutb, *droutb, *soutb;

                                /* symbol management */
struct local {
	unsigned locindex;              /* index to symbol in file */
	struct nlist *locsymbol;        /* ptr to symbol table */
};

#define NSYM            1500
#define NSYMPR          500
#define NLIBS           256
#define RANTABSZ        500

struct nlist cursym;            /* текущий символ */
struct nlist symtab [NSYM];     /* собственно символы */
struct nlist **symhash [NSYM];  /* указатели на хэш-таблицу */
struct nlist *lastsym;          /* последний введенный символ */
struct nlist *hshtab [NSYM+2];  /* хэш-таблица для символов */
struct local local [NSYMPR];
int symindex;                   /* следующий свободный вход таб. символов */
unsigned basaddr = BADDR;       /* base address of loading */
struct ranlib rantab [RANTABSZ];
int tnum;                       /* number of elements in rantab */

/*
 * library management
 */
unsigned liblist [NLIBS], *libp;

/*
 * internal symbols
 */
struct nlist *p_etext, *p_edata, *p_end, *entrypt;

/*
 * options
 */
int     trace;                  /* internal trace flag */
int     xflag;                  /* discard local symbols */
int     Xflag;                  /* discard locals starting with LOCSYM */
int     Sflag;                  /* discard all except locals and globals*/
int     rflag;                  /* preserve relocation bits, don't define commons */
int     arflag;                 /* original copy of rflag */
int     sflag;                  /* discard all symbols */
int     dflag;                  /* define common even with rflag */
int     verbose;                /* verbose mode */

/*
 * cumulative sizes set in pass 1
 */
unsigned tsize, dsize, bsize, ssize, nsym;

/*
 * symbol relocation; both passes
 */
unsigned ctrel, cdrel, cbrel;

/*
 * used after pass 1
 */
unsigned torigin, dorigin, borigin;

int	ofilfnd;
char	*ofilename = "l.out";
char	*filname;
int	errlev;
int	delarg	= 4;
char    tfname [] = "/tmp/ldaXXXXX";

#define ALIGN(x,y)     ((x)+(y)-1-((x)+(y)-1)%(y))

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

void fputhdr (hdr, coutb)
        register struct exec *hdr;
        register FILE *coutb;
{
        fputword (hdr->a_magic, coutb);
        fputword (hdr->a_text, coutb);
        fputword (hdr->a_data, coutb);
        fputword (hdr->a_bss, coutb);
        fputword (hdr->a_reltext, coutb);
        fputword (hdr->a_reldata, coutb);
        fputword (hdr->a_syms, coutb);
        fputword (hdr->a_entry, coutb);
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

void delexit (int sig)
{
	unlink ("l.out");
	if (! delarg && !arflag)
                chmod (ofilename, 0777 & ~umask(0));
	exit (delarg);
}

void error (int n, const char *s, ...)
{
        va_list ap;

        va_start (ap, s);
	if (! errlev)
                printf ("ld: ");
	if (filname)
                printf ("%s: ", filname);
	vprintf (s, ap);
	va_end (ap);
	printf ("\n");
	if (n > 1)
                delexit (0);
	errlev = n;
}

int fgetsym (text, sym)
        register FILE *text;
        register struct nlist *sym;
{
	register int c;

	c = getc (text);
	if (c <= 0)
		return 0;
	sym->n_len = c;
	sym->n_name = malloc (sym->n_len + 1);
	if (! sym->n_name) {
		error (2, "out of memory");
		return 0;
        }
	sym->n_type = getc (text);
	sym->n_value = fgetword (text);
	for (c=0; c<sym->n_len; c++)
		sym->n_name [c] = getc (text);
	sym->n_name [sym->n_len] = '\0';
	return sym->n_len + 6;
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

/*
 * Read the archive header.
 */
int fgetarhdr (fd, h)
        register FILE *fd;
        register struct archdr *h;
{
	struct ar_hdr hdr;
	register int len, nr;
	register char *p;
	char buf[20];

        /* Read file magic. */
	nr = read(fd, buf, SARMAG);
	if (nr != SARMAG || strncmp (buf, ARMAG, SARMAG) != 0)
                return 0;

	/* Read arhive name.  Spaces should never happen. */
	nr = fread (buf, 1, sizeof (hdr.ar_name), fd);
	if (nr != sizeof (hdr.ar_name) || buf[0] == ' ')
		return 0;
        buf[nr] = 0;

	/* Long name support.  Set the "real" size of the file,
	 * and the long name flag/size. */
	h->ar_size = 0;
	if (strncmp (buf, AR_EFMT1, sizeof(AR_EFMT1) - 1) == 0) {
		len = atoi (buf + sizeof(AR_EFMT1) - 1);
		if (len <= 0 || len > MAXNAMLEN)
			return 0;
                h->ar_name = malloc (len + 1);
                if (! h->ar_name)
                        return 0;
		nr = fread (h->ar_name, 1, len, fd);
		if (nr != len) {
failed:                 free (h->ar_name);
                        return 0;
		}
		h->ar_name[len] = 0;
		h->ar_size -= len;
	} else {
		/* Strip trailing spaces, null terminate. */
		p = buf + nr - 1;
		while (*p == ' ')
                        --p;
		*++p = '\0';

	        len = p - buf;
                h->ar_name = malloc (len + 1);
                if (! h->ar_name)
                        return 0;
                strcpy (h->ar_name, buf);
	}

	/* Read arhive date. */
	nr = fread (buf, 1, sizeof (hdr.ar_date), fd);
	if (nr != sizeof (hdr.ar_date))
		goto failed;
        buf[nr] = 0;
        h->ar_date = strtol (buf, 0, 10);

	/* Read user id. */
	nr = fread (buf, 1, sizeof (hdr.ar_uid), fd);
	if (nr != sizeof (hdr.ar_uid))
		goto failed;
        buf[nr] = 0;
        h->ar_uid = strtol (buf, 0, 10);

	/* Read group id. */
	nr = fread (buf, 1, sizeof (hdr.ar_gid), fd);
	if (nr != sizeof (hdr.ar_gid))
		goto failed;
        buf[nr] = 0;
        h->ar_gid = strtol (buf, 0, 10);

	/* Read mode (octal). */
	nr = fread (buf, 1, sizeof (hdr.ar_mode), fd);
	if (nr != sizeof (hdr.ar_mode))
		goto failed;
        buf[nr] = 0;
        h->ar_mode = strtol (buf, 0, 8);

	/* Read archive size. */
	nr = fread (buf, 1, sizeof (hdr.ar_size), fd);
	if (nr != sizeof (hdr.ar_size))
		goto failed;
        buf[nr] = 0;
        h->ar_size = strtol (buf, 0, 10);

	/* Check secondary magic. */
	nr = fread (buf, 1, sizeof (hdr.ar_fmag), fd);
	if (nr != sizeof (hdr.ar_fmag))
		goto failed;
        buf[nr] = 0;
	if (strcmp (buf, ARFMAG) != 0)
		goto failed;

	return 1;
}

void freerantab ()
{
	register struct ranlib *p;

	for (p=rantab; p<rantab+tnum; ++p)
		free (p->ran_name);
}

int fgetran (text, sym)
        register FILE *text;
        register struct ranlib *sym;
{
	register int c;

	/* read struct ranlib from file */
	/* 1 byte - length of name */
	/* 4 bytes - seek in archive */
	/* 'len' bytes - symbol name */
	/* if len == 0 then eof */
	/* return 1 if ok, 0 if eof, -1 if out of memory */

	sym->ran_len = getc (text);
	if (sym->ran_len <= 0)
		return (0);
	sym->ran_name = malloc (sym->ran_len + 1);
	if (! sym->ran_name)
		return (-1);
	sym->ran_off = fgetword (text);
	for (c=0; c<sym->ran_len; c++)
		sym->ran_name [c] = getc (text);
	sym->ran_name [sym->ran_len] = '\0';
	return (1);
}

void getrantab ()
{
	register struct ranlib *p;
	register int n;

	for (p=rantab; p<rantab+RANTABSZ; ++p) {
		n = fgetran (text, p);
		if (n < 0)
			error (2, "out of memory");
		if (n == 0) {
			tnum = p-rantab;
			return;
		}
	}
	error (2, "ranlib buffer overflow");
}

void ldrsym (sp, val, type)
        register struct nlist *sp;
        unsigned val;
{
	if (sp == 0)
                return;
	if (sp->n_type != N_EXT+N_UNDF) {
		printf ("%s: ", sp->n_name);
		error (1, "name redefined");
		return;
	}
	sp->n_type = type;
	sp->n_value = val;
}

void tcreat (buf, tempflg)
        register FILE **buf;
        register int tempflg;
{
	*buf = fopen (tempflg ? tfname : ofilename, "w+");
	if (! *buf)
		error (2, tempflg ?
			"cannot create temporary file" :
			"cannot create output file");
	if (tempflg)
                unlink (tfname);
}

int reltype (stype)
        int stype;
{
	switch (stype & N_TYPE) {
	case N_UNDF:    return (0);
	case N_ABS:     return (RABS);
	case N_TEXT:    return (RTEXT);
	case N_DATA:    return (RDATA);
	case N_BSS:     return (RBSS);
	case N_STRNG:   return (RDATA);
	case N_COMM:    return (RBSS);
	case N_FN:      return (0);
	default:        return (0);
	}
}

struct nlist *lookloc (lp, sn)
        register struct local *lp;
        register int sn;
{
	register struct local *clp;

	for (clp=local; clp<lp; clp++)
		if (clp->locindex == sn)
			return (clp->locsymbol);
	if (trace) {
		fprintf (stderr, "*** %d ***\n", sn);
		for (clp=local; clp<lp; clp++)
			fprintf (stderr, "%u, ", clp->locindex);
		fprintf (stderr, "\n");
	}
	error (2, "bad symbol reference");
	return 0;
}

void relword (lp, t, r, pt, pr)
        struct local *lp;
        register unsigned t, r;
        unsigned *pt, *pr;
{
	register unsigned a, ad;
	register struct nlist *sp;

	if (trace > 2)
		printf ("%08x %08x", t, r);

	/* extract address from command */

	switch (r & RFMASK) {
	case 0:
		a = t & 0xffff;
		break;
	case RWORD16:
		a = t & 0xffff;
		break;
	case RWORD26:
		a = t & 0x3ffffff;
		a <<= 2;
		break;
	case RHIGH16:
		a = t & 0xffff;
                a <<= 16;
		break;
	default:
		a = 0;
		break;
	}

	/* compute address shift `ad' */
	/* update relocation word */

	ad = 0;
	switch (r & RSMASK) {
	case RTEXT:
		ad = ctrel;
		break;
	case RDATA:
		ad = cdrel;
		break;
	case RBSS:
		ad = cbrel;
		break;
	case REXT:
		sp = lookloc (lp, (int) RINDEX (r));
		r &= RFMASK;
		if (sp->n_type == N_EXT+N_UNDF ||
		    sp->n_type == N_EXT+N_COMM)
		{
			r |= REXT | RSETINDEX (nsym+(sp-symtab));
			break;
		}
		r |= reltype (sp->n_type);
		ad = sp->n_value;
		break;
	}

	/* add updated address to command */

	switch (r & RFMASK) {
	case 0:
		t &= ~0xffff;
		t |= (a + ad) & 0xffff;
		break;
	case RWORD16:
		t &= ~0xffff;
		t |= ((a + ad) >> 2) & 0xffff;
		break;
	case RWORD26:
		t &= ~0x3ffffff;
		t |= ((a + ad) >> 2) & 0x3ffffff;
		break;
	case RHIGH16:
		t &= ~0xffff;
		t |= ((a + ad) >> 16) & 0xffff;
		break;
	}

	if (trace > 2)
		printf (" -> %08x %08x\n", t, r);

	*pt = t;
	*pr = r;
}

void relocate (lp, b1, b2, len)
        struct local *lp;
        FILE *b1, *b2;
        unsigned len;
{
	unsigned r, t;

	len /= W;
	while (len--) {
		t = fgetword (text);
		r = fgetrel (reloc);
		relword (lp, t, r, &t, &r);
		fputword (t, b1);
		if (rflag)
                        fputrel (r, b2);
	}
}

unsigned copy_and_close (buf)
        register FILE *buf;
{
	register int c;
	unsigned nbytes;

	rewind (buf);
	nbytes = 0;
	while ((c = getc (buf)) != EOF) {
                putc (c, outb);
                nbytes++;
        }
	fclose (buf);
	return nbytes;
}

int mkfsym (s, wflag)
        register char *s;
{
	register char *p;

	if (sflag || xflag)
                return (0);
	for (p=s; *p;)
                if (*p++ == '/')
                        s = p;
	if (! wflag)
                return (p - s + 6);
	cursym.n_len = p - s;
	cursym.n_name = malloc (cursym.n_len + 1);
	if (! cursym.n_name)
		error (2, "out of memory");
	for (p=cursym.n_name; *s; p++, s++)
                *p = *s;
	cursym.n_type = N_FN;
	cursym.n_value = torigin;
	fputsym (&cursym, soutb);
	free (cursym.n_name);
	return (cursym.n_len + 6);
}

int getfile (cp)
        register char *cp;
{
	int c;
	struct stat x;
        static char libname [] = "/lib/libxxxxxxxxxxxxxxx";

	text = 0;
	filname = cp;
	if (cp[0] == '-' && cp[1] == 'l') {
		if (cp[2] == '\0')
                        cp = "-la";
                filname = libname;
		for (c = 0; cp [c+2]; c++)
                        filname [c + 8] = cp [c+2];
		filname [c + 8] = '.';
		filname [c + 8 + 1] = 'a';
		filname [c + 8 + 2] = '\0';
	}
	text = fopen (filname, "r");
	if (! text)
		error (2, "cannot open");
	reloc = fopen (filname, "r");
	if (! reloc)
		error (2, "cannot open");

	if (! fgetarhdr (text, &archdr))
		return (0);     /* regular file */
	if (strncmp (archdr.ar_name, SYMDEF, sizeof (archdr.ar_name)) != 0)
		return (1);     /* regular archive */
	fstat (fileno (text), &x);
	if (x.st_mtime > archdr.ar_date + 2)
		return (3);     /* out of date archive */
	return (2);             /* randomized archive */
}

int enter (hp)
        register struct nlist **hp;
{
	register struct nlist *sp;

	if (*hp) {
		lastsym = *hp;
		return (0);
	}
        if (symindex >= NSYM)
                error (2, "symbol table overflow");

        symhash [symindex] = hp;
        *hp = lastsym = sp = &symtab[symindex++];
        sp->n_len = cursym.n_len;
        sp->n_name = cursym.n_name;
        sp->n_type = cursym.n_type;
        sp->n_value = cursym.n_value;
        return (1);
}

void symreloc()
{
	switch (cursym.n_type) {
	case N_TEXT:
	case N_EXT+N_TEXT:
		cursym.n_value += ctrel;
		return;
	case N_DATA:
	case N_EXT+N_DATA:
		cursym.n_value += cdrel;
		return;
	case N_BSS:
	case N_EXT+N_BSS:
		cursym.n_value += cbrel;
		return;
	case N_EXT+N_UNDF:
	case N_EXT+N_COMM:
		return;
	}
	if (cursym.n_type & N_EXT)
                cursym.n_type = N_EXT+N_ABS;
}

struct nlist **lookup()
{
	register int i;
	int clash;
	register char *cp, *cp1;
	register struct nlist **hp;

	i = 0;
	for (cp = cursym.n_name; *cp; cp++);
	        i = (i << 1) + *cp;

        hp = &hshtab[(i & 077777) % NSYM + 2];
	while (*hp != 0) {
		cp1 = (*hp)->n_name;
		clash = 0;
		for (cp = cursym.n_name; *cp;)
			if (*cp++ != *cp1++) {
				clash = 1;
				break;
			}
		if (clash) {
			if (++hp >= &hshtab[NSYM+2])
				hp = hshtab;
		} else
			break;
	}
	return (hp);
}

struct nlist **slookup (s)
        char *s;
{
	cursym.n_len = strlen (s) + 1;
	cursym.n_name = s;
	cursym.n_type = N_EXT+N_UNDF;
	cursym.n_value = 0;
	return (lookup ());
}

void readhdr (loc)
        unsigned loc;
{
	fseek (text, loc, 0);
	if (! fgethdr (text, &filhdr))
		error (2, "bad format");
	if (filhdr.a_magic != RMAGIC)
		error (2, "bad magic");
	if (filhdr.a_text % W)
		error (2, "bad length of text");
	if (filhdr.a_data % W)
		error (2, "bad length of data");
	if (filhdr.a_bss % W)
		error (2, "bad length of bss");
}

/*
 * single file
 */
int load1 (loc, libflg, nloc)
        unsigned loc;
{
	register struct nlist *sp;
	int savindex, ndef, type, symlen, nsymbol;

	readhdr (loc);
	if (filhdr.a_magic != RMAGIC) {
		error (1, "file not relocatable");
		return (0);
	}
	fseek (reloc, loc + N_SYMOFF (filhdr), 0);

	ctrel = tsize;
	cdrel = dsize - filhdr.a_text;
	cbrel = bsize - (filhdr.a_text + filhdr.a_data);

	loc += HDRSZ + filhdr.a_text + filhdr.a_data  +
                filhdr.a_reltext + filhdr.a_reldata;
	fseek (text, loc, 0);
	ndef = 0;
	savindex = symindex;
	if (nloc)
                nsymbol = 1;
        else
                nsymbol = 0;
	for (;;) {
		symlen = fgetsym (text, &cursym);
		if (symlen == 0)
			break;
		type = cursym.n_type;
		if (Sflag && ((type & N_TYPE) == N_ABS ||
			(type & N_TYPE) > N_COMM))
		{
			free (cursym.n_name);
			continue;
		}
		if (! (type & N_EXT)) {
			if (!sflag && !xflag &&
			    (!Xflag || cursym.n_name[0] != LOCSYM)) {
				nsymbol++;
				nloc += symlen;
			}
			free (cursym.n_name);
			continue;
		}
		symreloc ();
		if (enter (lookup ()))
                        continue;
		free (cursym.n_name);
		if (cursym.n_type == N_EXT+N_UNDF)
                        continue;
		sp = lastsym;
		if (sp->n_type == N_EXT+N_UNDF ||
			sp->n_type == N_EXT+N_COMM)
		{
			if (cursym.n_type == N_EXT+N_COMM) {
				sp->n_type = cursym.n_type;
				if (cursym.n_value > sp->n_value)
					sp->n_value = cursym.n_value;
			}
			else if (sp->n_type==N_EXT+N_UNDF ||
				cursym.n_type==N_EXT+N_DATA ||
				cursym.n_type==N_EXT+N_BSS)
			{
				ndef++;
				sp->n_type = cursym.n_type;
				sp->n_value = cursym.n_value;
			}
		}
	}
	if (! libflg || ndef) {
		tsize += filhdr.a_text;
		dsize += filhdr.a_data;
		bsize += filhdr.a_bss;
		ssize += nloc;
		nsym += nsymbol;
		return (1);
	}

	/*
	 * No symbols defined by this library member.
	 * Rip out the hash table entries and reset the symbol table.
	 */
	while (symindex > savindex) {
		register struct nlist **p;

		p = symhash[--symindex];
		free ((*p)->n_name);
		*p = 0;
	}
	return (0);
}

void checklibp ()
{
	if (libp >= &liblist[NLIBS])
		error (2, "library table overflow");
}

int step (nloc)
        register unsigned nloc;
{
	register char *cp;

	fseek (text, nloc, 0);
	if (! fgetarhdr (text, &archdr)) {
		*libp++ = -1;
		checklibp ();
		return (0);
	}
	cp = malloc (15);
	strncpy (cp, archdr.ar_name, 14);
	cp [14] = '\0';
	if (load1 (nloc + ARHDRSZ, 1, mkfsym (cp, 0)))
		*libp++ = nloc;
	free (cp);
	checklibp ();
	return (1);
}

int ldrand ()
{
	register struct ranlib *p;
	struct nlist **pp;
	unsigned *oldp = libp;

	for (p=rantab; p<rantab+tnum; ++p) {
		pp = slookup (p->ran_name);
		if (! *pp)
			continue;
		if ((*pp)->n_type == N_EXT+N_UNDF)
			step (p->ran_off);
	}
	return (oldp != libp);
}

/*
 * scan file to find defined symbols
 */
void load1arg (cp)
        register char *cp;
{
	register unsigned nloc;

	switch (getfile (cp)) {
	case 0:                 /* regular file */
		load1 (0L, 0, mkfsym (cp, 0));
		break;
	case 1:                 /* regular archive */
		nloc = W;
archive:
		while (step (nloc))
			nloc += archdr.ar_size + ARHDRSZ;
		break;
	case 2:                 /* table of contents */
		getrantab ();
		while (ldrand ())
                        continue;
		freerantab ();
		*libp++ = -1;
		checklibp ();
		break;
	case 3:                 /* out of date archive */
		error (0, "out of date (warning)");
		nloc = W + archdr.ar_size + ARHDRSZ;
		goto archive;
	}
	fclose (text);
	fclose (reloc);
}

void pass1 (argc, argv)
        char **argv;
{
	register int c, i;
	unsigned num;
	register char *ap, **p;
	char save;

	/* scan files once to find symdefs */

	p = argv + 1;
	libp = liblist;
	for (c=1; c<argc; ++c) {
		filname = 0;
		ap = *p++;

		if (*ap != '-') {
			load1arg (ap);
			continue;
		}
		for (i=1; ap[i]; i++) {
			switch (ap [i]) {

				/* output file name */
			case 'o':
				if (++c >= argc)
					error (2, "-o: argument missed");
				ofilename = *p++;
				ofilfnd++;
				continue;

				/* 'use' */
			case 'u':
				if (++c >= argc)
					error (2, "-u: argument missed");
				enter (slookup (*p++));
				continue;

				/* 'entry' */
			case 'e':
				if (++c >= argc)
					error (2, "-e: argument missed");
				enter (slookup (*p++));
				entrypt = lastsym;
				continue;

				/* set data size */
			case 'D':
				if (++c >= argc)
					error (2, "-D: argument missed");
				num = atoi (*p++);
				if (dsize > num)
					error (2, "-D: too small");
				dsize = num;
				continue;

				/* base address of loading */
			case 'T':
				basaddr = atol (ap+i+1);
				break;

				/* library */
			case 'l':
				save = ap [--i];
				ap [i] = '-';
				load1arg (&ap[i]);
				ap [i] = save;
				break;

				/* discard local symbols */
			case 'x':
				xflag++;
				continue;

				/* discard locals starting with LOCSYM */
			case 'X':
				Xflag++;
				continue;

				/* discard all except locals and globals*/
			case 'S':
				Sflag++;
				continue;

				/* preserve rel. bits, don't define common */
			case 'r':
				rflag++;
				arflag++;
				continue;

				/* discard all symbols */
			case 's':
				sflag++;
				xflag++;
				continue;

				/* define common even with rflag */
			case 'd':
				dflag++;
				continue;

				/* tracing */
			case 't':
				trace++;
				continue;

				/* verbose */
			case 'v':
				verbose++;
				continue;

			default:
				error (2, "unknown flag");
			}
			break;
		}
	}
}

void middle()
{
	register struct nlist *sp, *symp;
	register unsigned t, cmsize;
	int nund;
	unsigned cmorigin;

	p_etext = *slookup ("_etext");
	p_edata = *slookup ("_edata");
	p_end = *slookup ("_end");

	/*
	 * If there are any undefined symbols, save the relocation bits.
	 */
	symp = &symtab[symindex];
	if (!rflag) {
		for (sp=symtab; sp<symp; sp++)
			if (sp->n_type == N_EXT+N_UNDF &&
				sp != p_end && sp != p_edata && sp != p_etext)
			{
				rflag++;
				dflag = 0;
				break;
			}
	}
	if (rflag)
                sflag = 0;

	/*
	 * Assign common locations.
	 */
	cmsize = 0;
	if (dflag || !rflag) {
		ldrsym (p_etext, tsize, N_EXT+N_TEXT);
		ldrsym (p_edata, dsize, N_EXT+N_DATA);
		ldrsym (p_end, bsize, N_EXT+N_BSS);
		for (sp=symtab; sp<symp; sp++) {
			if ((sp->n_type & N_TYPE) == N_COMM) {
				t = sp->n_value;
				sp->n_value = cmsize;
				cmsize += t;
			}
                }
	}

	/*
	 * Now set symbols to their final value
	 */
	torigin = basaddr;
	dorigin = torigin + tsize;
	cmorigin = dorigin + dsize;
	borigin = cmorigin + cmsize;
	nund = 0;
	for (sp=symtab; sp<symp; sp++) {
		switch (sp->n_type) {
		case N_EXT+N_UNDF:
			if (! arflag) {
                                errlev |= 1;
				if (! nund)
					printf ("Undefined:\n");
				nund++;
				printf ("\t%s\n", sp->n_name);
			}
			break;
		default:
		case N_EXT+N_ABS:
			break;
		case N_EXT+N_TEXT:
			sp->n_value += torigin;
			break;
		case N_EXT+N_DATA:
			sp->n_value += dorigin;
			break;
		case N_EXT+N_BSS:
			sp->n_value += borigin;
			break;
		case N_COMM:
		case N_EXT+N_COMM:
			sp->n_type = N_EXT+N_BSS;
			sp->n_value += cmorigin;
			break;
		}
	}
	if (sflag || xflag)
                ssize = 0;
	bsize += cmsize;

	/*
	 * Compute ssize; add length of local symbols, if need,
	 * and one more zero byte. Alignment will be taken at setupout.
	 */
	if (sflag)
                ssize = 0;
	else {
		if (xflag)
                        ssize = 0;
		for (sp = symtab; sp < &symtab[symindex]; sp++)
			ssize += sp->n_len + 6;
		ssize++;
	}
}

void setupout ()
{
	tcreat (&outb, 0);
	mktemp (tfname);
	tcreat (&toutb, 1);
	tcreat (&doutb, 1);

	if (! sflag || ! xflag)
                tcreat (&soutb, 1);
	if (rflag) {
		tcreat (&troutb, 1);
		tcreat (&droutb, 1);
	}
	fseek (outb, sizeof(filhdr), 0);
}

void load2 (loc)
        unsigned loc;
{
	register struct nlist *sp;
	register struct local *lp;
	register int symno;
	int type;
	unsigned count;

	readhdr (loc);
	ctrel = torigin;
	cdrel = dorigin - filhdr.a_text;
	cbrel = borigin - (filhdr.a_text + filhdr.a_data);

	if (trace > 1)
		printf ("ctrel=%08x, cdrel=%08x, cbrel=%08x\n",
			ctrel, cdrel, cbrel);
	/*
	 * Reread the symbol table, recording the numbering
	 * of symbols for fixing external references.
	 */
	lp = local;
	symno = -1;
	loc += HDRSZ;
	fseek (text, loc + filhdr.a_text + filhdr.a_data +
                filhdr.a_reltext + filhdr.a_reldata, 0);
	for (;;) {
		symno++;
		count = fgetsym (text, &cursym);
		if (count == 0)
			break;
		symreloc ();
		type = cursym.n_type;
		if (Sflag && ((type & N_TYPE) == N_ABS ||
			(type & N_TYPE) > N_COMM))
		{
			free (cursym.n_name);
			continue;
		}
		if (! (type & N_EXT)) {
			if (!sflag && !xflag &&
			    (!Xflag || cursym.n_name [0] != LOCSYM))
				fputsym (&cursym, soutb);
			free (cursym.n_name);
			continue;
		}
		if (! (sp = *lookup()))
			error (2, "internal error: symbol not found");
		free (cursym.n_name);
		if (cursym.n_type == N_EXT+N_UNDF ||
			cursym.n_type == N_EXT+N_COMM)
		{
			if (lp >= &local [NSYMPR])
				error (2, "local symbol table overflow");
			lp->locindex = symno;
			lp++->locsymbol = sp;
			continue;
		}
		if (cursym.n_type != sp->n_type ||
			cursym.n_value != sp->n_value)
		{
			printf ("%s: ", cursym.n_name);
			error (1, "name redefined");
		}
	}

	count = loc + filhdr.a_text + filhdr.a_data;

	if (trace > 1)
		printf ("** TEXT **\n");
	fseek (text, loc, 0);
	fseek (reloc, count, 0);
	relocate (lp, toutb, troutb, filhdr.a_text);

	if (trace > 1)
		printf ("** DATA **\n");
	fseek (text, loc + filhdr.a_text, 0);
	fseek (reloc, count + filhdr.a_reltext, 0);
	relocate (lp, doutb, droutb, filhdr.a_data);

	torigin += filhdr.a_text;
	dorigin += filhdr.a_data;
	borigin += filhdr.a_bss;
}

void load2arg (acp)
        register char *acp;
{
	register unsigned *lp;

	if (getfile (acp) == 0) {
		if (trace || verbose)
			printf ("%s:\n", acp);
		mkfsym (acp, 1);
		load2 (0L);
	} else {
		/* scan archive members referenced */
		char *arname = acp;

		for (lp = libp; *lp != -1; lp++) {
			fseek (text, *lp, 0);
			fgetarhdr (text, &archdr);
			acp = malloc (15);
			strncpy (acp, archdr.ar_name, 14);
			acp [14] = '\0';
			if (trace || verbose)
				printf ("%s(%s):\n", arname, acp);
			mkfsym (acp, 1);
			free (acp);
			load2 (*lp + ARHDRSZ);
		}
		libp = ++lp;
	}
	fclose (text);
	fclose (reloc);
}

void pass2 (argc, argv)
        int argc;
        char **argv;
{
	register int c, i;
	unsigned dnum;
	register char *ap, **p;

	p = argv + 1;
	libp = liblist;
	for (c=1; c<argc; c++) {
		ap = *p++;
		if (*ap != '-') {
			load2arg (ap);
			continue;
		}
		for (i=1; ap[i]; i++) {
			switch (ap[i]) {

			case 'D':
                                /* ??? for (dnum=atoi(*p); dorigin<dnum; dorigin++) { */
				for (dnum=atoi(*p); dnum>0; --dnum) {
					fputword (0L, doutb);
					fputword (0L, doutb);
					if (rflag) {
						fputword (0L, droutb);
						fputword (0L, droutb);
					}
				}
			case 'u':
			case 'e':
			case 'o':
				++c;
				++p;

			default:
				continue;

			case 'l':
				ap [--i] = '-';
				load2arg (&ap[i]);
				break;

			}
			break;
		}
	}
}

void finishout ()
{
	register struct nlist *p;
        unsigned rtsize = 0, rdsize = 0;

	copy_and_close (toutb);
	copy_and_close (doutb);
	if (rflag) {
		rtsize = copy_and_close (troutb);
		while (rtsize % W) {
			putc (0, outb);
			rtsize++;
                }
		rdsize = copy_and_close (droutb);
		while (rdsize % W) {
			putc (0, outb);
			rdsize++;
                }
	}
	if (! sflag) {
		if (! xflag)
                        copy_and_close (soutb);
		for (p=symtab; p<&symtab[symindex]; ++p)
			fputsym (p, outb);
		putc (0, outb);
		while (ssize++ % W)
			putc (0, outb);
	}
	filhdr.a_magic = rflag ? RMAGIC : OMAGIC;
	filhdr.a_text = tsize;
	filhdr.a_data = dsize;
	filhdr.a_bss = bsize;
	filhdr.a_reltext = rtsize;
	filhdr.a_reldata = rdsize;
	filhdr.a_syms = ALIGN (ssize, W);
	if (entrypt) {
		if (entrypt->n_type != N_EXT+N_TEXT &&
		    entrypt->n_type != N_EXT+N_UNDF)
			error (1, "entry out of text");
		else filhdr.a_entry = entrypt->n_value;
	} else
		filhdr.a_entry = basaddr;

	fseek (outb, 0, 0);
	fputhdr (&filhdr, outb);
	fclose (outb);
}

int main (argc, argv)
        char **argv;
{
	if (argc == 1) {
		printf ("Usage: %s [-xXsSrndt] [-lname] [-D num] [-u name] [-e name] [-o file] file...\n",
			argv [0]);
		exit (4);
	}
	if (signal (SIGINT, SIG_IGN) != SIG_IGN)
                signal (SIGINT, delexit);
	if (signal (SIGTERM, SIG_IGN) != SIG_IGN)
                signal (SIGTERM, delexit);

	/*
	 * First pass: compute lengths of segments, symbol name table
         * and entry address.
	 */
	pass1 (argc, argv);
	filname = 0;

	/*
	 * Compute name table.
	 */
	middle ();

	/*
	 * Create temporary files.
	 */
	setupout ();

	/*
	 * Second pass: relocation.
	 */
	pass2 (argc, argv);

	/*
	 * Flush buffers, write a header.
	 */
	finishout ();

	if (! ofilfnd) {
		unlink ("a.out");
		link ("l.out", "a.out");
		ofilename = "a.out";
	}
	delarg = errlev;
	delexit (0);
	return (0);
}
