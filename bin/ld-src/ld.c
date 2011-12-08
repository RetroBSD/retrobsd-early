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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
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
	long	ar_size;
} archdr;

FILE *text, *reloc;             /* input management */

                                /* output management */
FILE *outb, *toutb, *doutb, *troutb, *droutb, *soutb;

				/* symbol management */
struct local {
	long locindex;          /* index to symbol in file */
	struct nlist *locsymbol; /* ptr to symbol table */
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

long liblist [NLIBS], *libp;    /* library management */

				/* internal symbols */

struct nlist *p_etext, *p_edata, *p_end, *entrypt;

				/* flags */
int     trace;                  /* internal trace flag */
int     xflag;                  /* discard local symbols */
int     Xflag;                  /* discard locals starting with LOCSYM */
int     Sflag;                  /* discard all except locals and globals*/
int     rflag;                  /* preserve relocation bits, don't define commons */
int     arflag;                 /* original copy of rflag */
int     sflag;                  /* discard all symbols */
int     dflag;                  /* define common even with rflag */

				/* cumulative sizes set in pass 1 */

long tsize, dsize, bsize, ssize, nsym;

				/* symbol relocation; both passes */

long ctrel, cdrel, cbrel;

int	ofilfnd;
char	*ofilename = "l.out";
char	*filname;
int	errlev;
int	delarg	= 4;
char    tfname [] = "/tmp/ldaXXXXX";
char    libname [] = "/lib/libxxxxxxxxxxxxxxx";

#define LNAMLEN 17             /* originally 12 */

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

/*
 * Read the archive header.
 */
int
fgetarhdr (fd, h)
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

	/* Long name support.  Set the "real" size of the file, and the
	 * long name flag/size. */
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

void
delexit (int sig)
{
	unlink ("l.out");
	if (! delarg && !arflag)
                chmod (ofilename, 0777 & ~umask(0));
	exit (delarg);
}

main (argc, argv)
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
	 * Первый проход: вычисление длин сегментов и таблицы имен,
	 * а также адреса входа.
	 */

	pass1 (argc, argv);
	filname = 0;

	/*
	 * Обработка таблицы имен.
	 */

	middle ();

	/*
	 * Создание буферных файлов и запись заголовка
	 */

	setupout ();

	/*
	 * Второй проход: настройка связей.
	 */

	pass2 (argc, argv);

	/*
	 * Сброс буферов.
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

struct nlist **lookup()
{
	register i;
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

pass1 (argc, argv)
char **argv;
{
	register c, i;
	long num;
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

			default:
				error (2, "unknown flag");
			}
			break;
		}
	}
}

/* scan file to find defined symbols */
load1arg (cp)
register char *cp;
{
	register long nloc;

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

ldrand ()
{
	register struct ranlib *p;
	struct nlist **pp;
	long *oldp = libp;

	for (p=rantab; p<rantab+tnum; ++p) {
		pp = slookup (p->ran_name);
		if (! *pp)
			continue;
		if ((*pp)->n_type == N_EXT+N_UNDF)
			step (p->ran_off);
	}
	return (oldp != libp);
}

step (nloc)
register long nloc;
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

checklibp ()
{
	if (libp >= &liblist[NLIBS])
		error (2, "library table overflow");
}

freerantab ()
{
	register struct ranlib *p;

	for (p=rantab; p<rantab+tnum; ++p)
		free (p->ran_name);
}

fgetran (text, sym)
        register FILE *text;
        register struct ranlib *sym;
{
	register c;

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

getrantab ()
{
	register struct ranlib *p;
	register n;

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

fgetsym (text, sym)
        register FILE *text;
        register struct nlist *sym;
{
	register c;

	if ((sym->n_len = getc (text)) <= 0)
		return (1);
	sym->n_name = malloc (sym->n_len + 1);
	if (! sym->n_name)
		return (0);
	sym->n_type = getc (text);
	sym->n_value = fgetword (text);
	for (c=0; c<sym->n_len; c++)
		sym->n_name [c] = getc (text);
	sym->n_name [sym->n_len] = '\0';
	return (sym->n_len + 6);
}

fputsym (s, file)
register struct nlist *s;
register FILE *file;
{
	register i;

	putc (s->n_len, file);
	putc (s->n_type, file);
	fputword (s->n_value, file);
	for (i=0; i<s->n_len; i++)
		putc (s->n_name[i], file);
}

/* single file */
load1 (loc, libflg, nloc)
long loc;
{
	register struct nlist *sp;
	int savindex, ndef, type, symlen, nsymbol;

	readhdr (loc);
	if (filhdr.a_flag & 1) {
		error (1, "file stripped");
		return (0);
	}
	fseek (reloc, loc + N_SYMOFF (filhdr), 0);
	ctrel += tsize;
	cdrel += dsize;
	cbrel += bsize;
	loc += HDRSZ + (filhdr.a_text + filhdr.a_data) * 2;
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
			error (2, "out of memory");
		if (symlen == 1)
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

/* used after pass 1 */
long    torigin;
long    dorigin;
long    borigin;

middle()
{
	register struct nlist *sp, *symp;
	register long t;
	register long cmsize;
	int nund;
	long cmorigin;

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
		if (sp->n_value & ~0777777777)
			error (1, "long address: %s=0%lo",
				sp->n_name, sp->n_value);
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

ldrsym (sp, val, type)
register struct nlist *sp;
long val;
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

setupout ()
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
	filhdr.a_magic = OMAGIC;
	filhdr.a_text = tsize;
	filhdr.a_data = dsize;
	filhdr.a_bss = bsize;
	filhdr.a_syms = ALIGN (ssize, W);
	if (entrypt) {
		if (entrypt->n_type != N_EXT+N_TEXT &&
		    entrypt->n_type != N_EXT+N_UNDF)
			error (1, "entry out of text");
		else filhdr.a_entry = entrypt->n_value;
	} else
		filhdr.a_entry = torigin;

	if (rflag)
		filhdr.a_flag &= ~1;
	else
		filhdr.a_flag |= 1;

	fputhdr (&filhdr, outb);
}

tcreat (buf, tempflg)
register FILE **buf;
register tempflg;
{
	*buf = fopen (tempflg ? tfname : ofilename, "w+");
	if (! *buf)
		error (2, tempflg ?
			"cannot create temporary file" :
			"cannot create output file");
	if (tempflg)
                unlink (tfname);
}

pass2 (argc, argv)
int argc;
char **argv;
{
	register int c, i;
	long dnum;
	register char *ap, **p;

	p = argv+1;
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
			case 'v':
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

load2arg (acp)
register char *acp;
{
	register long *lp;

	if (getfile (acp) == 0) {
		if (trace)
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
			if (trace)
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

load2 (loc)
long loc;
{
	register struct nlist *sp;
	register struct local *lp;
	register int symno;
	int type;
	long count;

	readhdr (loc);
	ctrel += torigin;
	cdrel += dorigin;
	cbrel += borigin;

	if (trace > 1)
		printf ("ctrel=%lxh, cdrel=%lxh, cbrel=%lxh\n",
			ctrel, cdrel, cbrel);
	/*
	 * Reread the symbol table, recording the numbering
	 * of symbols for fixing external references.
	 */

	lp = local;
	symno = -1;
	loc += HDRSZ;
	fseek (text, loc + (filhdr.a_text + filhdr.a_data) * 2, 0);
	for (;;) {
		symno++;
		count = fgetsym (text, &cursym);
		if (count == 0)
			error (2, "out of memory");
		if (count == 1)
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
	fseek (reloc, count + filhdr.a_text, 0);
	relocate (lp, doutb, droutb, filhdr.a_data);

	torigin += filhdr.a_text;
	dorigin += filhdr.a_data;
	borigin += filhdr.a_bss;
}

relocate (lp, b1, b2, len)
struct local *lp;
FILE *b1, *b2;
long len;
{
	long r, t;

	len /= W;
	while (len--) {
		t = fgetword (text);
		r = fgetword (reloc);
		relword (lp, t, r, &t, &r);
		fputword (t, b1);
		if (rflag)
                        fputword (r, b2);
	}
}

struct nlist *lookloc (lp, sn)
register struct local *lp;
register sn;
{
	register struct local *clp;

	for (clp=local; clp<lp; clp++)
		if (clp->locindex == sn)
			return (clp->locsymbol);
	if (trace) {
		fprintf (stderr, "*** %d ***\n", sn);
		for (clp=local; clp<lp; clp++)
			fprintf (stderr, "%d, ", clp->locindex);
		fprintf (stderr, "\n");
	}
	error (2, "bad symbol reference");
	/* NOTREACHED */
}

relword (lp, t, r, pt, pr)
struct local *lp;
register long t, r;
long *pt, *pr;
{
	register long a, ad;
	register short i;
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
	switch (r & REXT) {
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

finishout ()
{
	register long n;
	register struct nlist *p;

	copy (toutb);
	copy (doutb);
	if (rflag) {
		copy (troutb);
		copy (droutb);
	}
	if (! sflag) {
		if (! xflag)
                        copy (soutb);
		for (p=symtab; p<&symtab[symindex]; ++p)
			fputsym (p, outb);
		putc (0, outb);
		while (ssize++ % W)
			putc (0, outb);
	}
	fclose (outb);
}

copy (buf)
register FILE *buf;
{
	register c;

	rewind (buf);
	while ((c = getc (buf)) != EOF)
                putc (c, outb);
	fclose (buf);
}

mkfsym (s, wflag)
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

getfile (cp)
register char *cp;
{
	int c;
	struct stat x;

	text = 0;
	filname = cp;
	if (cp[0] == '-' && cp[1] == 'l') {
		if (cp[2] == '\0')
                        cp = "-la";
		filname = libname;
		for (c = 0; cp [c+2]; c++)
                        filname [c + LNAMLEN] = cp [c+2];
		filname [c + LNAMLEN] = '.';
		filname [c + LNAMLEN + 1] = 'a';
		filname [c + LNAMLEN + 2] = '\0';
		text = fopen (filname, "r");
		if (! text)
                        filname += 4;
	}
	if (! text && ! (text = fopen (filname, "r")))
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

enter (hp)
register struct nlist **hp;
{
	register struct nlist *sp;

	if (! *hp) {
		if (symindex >= NSYM)
			error (2, "symbol table overflow");
		symhash [symindex] = hp;
		*hp = lastsym = sp = &symtab[symindex++];
		sp->n_len = cursym.n_len;
		sp->n_name = cursym.n_name;
		sp->n_type = cursym.n_type;
		sp->n_value = cursym.n_value;
		return (1);
	} else {
		lastsym = *hp;
		return (0);
	}
}

symreloc()
{
	register short i;

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

/* VARARGS2 */

error (n, s, a, b, c)
char *s;
{
	if (! errlev)
                printf ("ld: ");
	if (filname)
                printf ("%s: ", filname);
	printf (s, a, b, c);
	printf ("\n");
	if (n > 1)
                delexit (0);
	errlev = n;
}

readhdr (loc)
long loc;
{
	fseek (text, loc, 0);
	if (! fgethdr (text, &filhdr))
		error (2, "bad format");
	if (filhdr.a_magic != OMAGIC)
		error (2, "bad magic");
	if (filhdr.a_text % W)
		error (2, "bad length of text");
	if (filhdr.a_data % W)
		error (2, "bad length of data");
	if (filhdr.a_bss % W)
		error (2, "bad length of bss");
	ctrel = - BADDR;
	cdrel = - BADDR - filhdr.a_text;
	cbrel = - BADDR - (filhdr.a_text + filhdr.a_data);
}

reltype (stype)
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
