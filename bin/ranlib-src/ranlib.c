/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Hugh Smith at The University of Guelph.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <paths.h>
#include <ar.h>
#include <ranlib.h>
#include <a.out.h>
#include <archive.h>

u_int options;				/* UNUSED -- keep open_archive happy */

CHDR chdr;                              /* converted header */
char *archive;                          /* archive name */
char *tname = "temporary file";		/* temporary file "name" */

typedef struct _rlib {
	struct _rlib *next;		/* next structure */
	off_t pos;			/* offset of defining archive file */
	char *sym;			/* symbol */
	int symlen;			/* strlen(sym) */
} RLIB;
RLIB *rhead, **pnext;

FILE *fp;
long symcnt;				/* symbol count */
long tsymlen;				/* total string length */

void error(name)
	char *name;
{
	(void)fprintf(stderr, "ranlib: %s: %s\n", name, strerror(errno));
	exit(1);
}

void *emalloc(len)
	int len;
{
	char *p;

	p = malloc((u_int)len);
	if (! p)
		error(archive);
	return((void *)p);
}

int sgets(buf, n, fp)
	char *buf;
	int n;
	register FILE *fp;
{
	register int i, c;

	n--;				/* room for null */
	for (i = 0; i < n; i++) {
		c = getc(fp);
		if (c == EOF || c == 0)
			break;
		*buf++ = c;
	}
	*buf = '\0';
	return(i + 1);
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

/*
 * Read a symbol table entry.
 * Return a number of bytes read, or -1 on EOF.
 * Format of symbol record:
 *  1 byte: length of name in bytes
 *  1 byte: type of symbol (N_UNDF, N_ABS, N_TEXT, etc)
 *  4 bytes: value
 *  N bytes: name
 */
int fgetsym (fi, name, value, type)
        register FILE *fi;
        register char *name;
        unsigned *value;
        unsigned short *type;
{
        register int len;
        unsigned nbytes;

        len = getc (fi);
        if (len <= 0)
                return -1;
        *type = getc (fi);
        *value = fgetword (fi);
        nbytes = len + 6;
        if (name) {
                while (len-- > 0)
                        *name++ = getc (fi);
                *name = '\0';
        } else
                fseek (fi, len, SEEK_CUR);
        return nbytes;
}

/*
 * Read the exec structure; ignore any files that don't look exactly right.
 */
void rexec(rfd, wfd)
	int rfd;
	int wfd;
{
	register RLIB *rp;
	long nsyms;
	register int nr, symlen;
	struct exec ebuf;
	off_t r_off, w_off;

	/* Get current offsets for original and tmp files. */
	r_off = lseek(rfd, (off_t)0, L_INCR);
	w_off = lseek(wfd, (off_t)0, L_INCR);

	/* Read in exec structure. */
	nr = read(rfd, (char *)&ebuf, sizeof(struct exec));
	if (nr != sizeof(struct exec))
		goto bad1;

	/* Check magic number and symbol count. */
	if (N_BADMAG(ebuf) || ebuf.a_syms == 0)
		goto bad1;

	/* Seek to symbol table. */
	if (fseek(fp, (off_t)N_SYMOFF(ebuf) + r_off, L_SET) == (off_t)-1)
		goto bad1;

	/* For each symbol read the nlist entry and save it as necessary. */
	nsyms = ebuf.a_syms;
	while (nsyms > 0) {
                unsigned value;
                unsigned short type;
	        char name [256];

                int c = fgetsym(fp, name, &value, &type);
                if (c <= 0) {
			if (feof(fp)) {
				/* Bad file format. */
                                errno = EINVAL;
                        }
			error(archive);
                }
                nsyms -= c;
		symlen = c - 6;

		/* Ignore if no name or local. */
		if (symlen <= 0 || ! (type & N_EXT))
			continue;

		/*
		 * If the symbol is an undefined external and the n_value
		 * field is non-zero, keep it.
		 */
		if ((type & N_TYPE) == N_UNDF && ! value)
			continue;

		rp = (RLIB *)emalloc(sizeof(RLIB));
		rp->next = NULL;
		rp->pos = w_off;
		rp->symlen = symlen;
		rp->sym = (char*) emalloc(symlen);
		bcopy(name, rp->sym, symlen);
		tsymlen += symlen;

		/* Build in forward order for "ar -m" command. */
		*pnext = rp;
		pnext = &rp->next;
		++symcnt;
	}
bad1:	(void)lseek(rfd, (off_t)r_off, L_SET);
}

/*
 * symobj --
 *	Write the symbol table into the archive, computing offsets as
 *	writing.
 */
void symobj()
{
	register RLIB *rp, *next;
	struct ranlib rn;
	char hb[sizeof(struct ar_hdr) + 1], pad;
	long ransize, size, stroff;
	gid_t getgid();
	uid_t getuid();

	/* Rewind the archive, leaving the magic number. */
	if (fseek(fp, (off_t)SARMAG, L_SET) == (off_t)-1)
		error(archive);

	/* Size of the ranlib archive file, pad if necessary. */
	ransize = sizeof(long) +
	    symcnt * sizeof(struct ranlib) + sizeof(long) + tsymlen;
	if (ransize & 01) {
		++ransize;
		pad = '\n';
	} else
		pad = '\0';

	/* Put out the ranlib archive file header. */
	(void)sprintf(hb, HDR2, RANLIBMAG, 0L, getuid(), getgid(),
	    0666 & ~umask(0), ransize, ARFMAG);
	if (!fwrite(hb, sizeof(struct ar_hdr), 1, fp))
		error(tname);

	/* First long is the size of the ranlib structure section. */
	size = symcnt * sizeof(struct ranlib);
	if (!fwrite((char *)&size, sizeof(size), 1, fp))
		error(tname);

	/* Offset of the first archive file. */
	size = SARMAG + sizeof(struct ar_hdr) + ransize;

	/*
	 * Write out the ranlib structures.  The offset into the string
	 * table is cumulative, the offset into the archive is the value
	 * set in rexec() plus the offset to the first archive file.
	 */
	for (rp = rhead, stroff = 0; rp; rp = rp->next) {
		rn.ran_name = (char*) stroff;
		stroff += rp->symlen;
		rn.ran_off = size + rp->pos;
		if (!fwrite((char *)&rn, sizeof(struct ranlib), 1, fp))
			error(archive);
	}

	/* Second long is the size of the string table. */
	if (!fwrite((char *)&tsymlen, sizeof(tsymlen), 1, fp))
		error(tname);

	/* Write out the string table. */
	for (rp = rhead; rp; ) {
		if (!fwrite(rp->sym, rp->symlen, 1, fp))
			error(tname);
		(void)free(rp->sym);
		next = rp->next;
		free(rp);
		rp = next;
	}

	if (pad && !fwrite(&pad, sizeof(pad), 1, fp))
		error(tname);

	(void)fflush(fp);
}

void settime(afd)
	int afd;
{
	struct ar_hdr *hdr;
	off_t size;
	char buf[50];

	size = SARMAG + sizeof(hdr->ar_name);
	if (lseek(afd, size, L_SET) == (off_t)-1)
		error(archive);
	(void)sprintf(buf, "%-12ld", time((time_t *)NULL) + RANLIBSKEW);
	if (write(afd, buf, sizeof(hdr->ar_date)) != sizeof(hdr->ar_date))
		error(archive);
}

int tmp()
{
	long set, oset;
	int fd;
	char path[MAXPATHLEN];

	bcopy(_PATH_RANTMP, path, sizeof(_PATH_RANTMP));

	set = sigmask(SIGHUP) | sigmask(SIGINT) |
             sigmask(SIGQUIT) | sigmask(SIGTERM);
	oset = sigsetmask(set);
	fd = mkstemp(path);
	if (fd < 0)
		error(tname);
        (void)unlink(path);
	(void)sigsetmask(oset);
	return(fd);
}

int build()
{
	CF cf;
	int afd, tfd;
	off_t size;

	afd = open_archive(O_RDWR);
	fp = fdopen(afd, "r+");
	tfd = tmp();

	SETCF(afd, archive, tfd, tname, RPAD|WPAD);

	/* Read through the archive, creating list of symbols. */
	pnext = &rhead;
	symcnt = tsymlen = 0;
	while(get_arobj(afd)) {
		if (!strcmp(chdr.name, RANLIBMAG)) {
			skip_arobj(afd);
			continue;
		}
		rexec(afd, tfd);
		put_arobj(&cf, (struct stat *)NULL);
	}
	*pnext = NULL;

	/* Create the symbol table. */
	symobj();

	/* Copy the saved objects into the archive. */
	size = lseek(tfd, (off_t)0, L_INCR);
	(void)lseek(tfd, (off_t)0, L_SET);
	SETCF(tfd, tname, afd, archive, RPAD|WPAD);
	copy_ar(&cf, size);
	(void)ftruncate(afd, lseek(afd, (off_t)0, L_INCR));
	(void)close(tfd);

	/* Set the time. */
	settime(afd);
	close_archive(afd);
	return(0);
}

int touch()
{
	int afd;

	afd = open_archive(O_RDWR);

	if (!get_arobj(afd) ||
	    strncmp(RANLIBMAG, chdr.name, sizeof(RANLIBMAG) - 1)) {
		(void)fprintf(stderr,
		    "ranlib: %s: no symbol table.\n", archive);
		return(1);
	}
	settime(afd);
	return(0);
}

void usage()
{
	(void)fprintf(stderr, "usage: ranlib [-t] archive ...\n");
	exit(1);
}

int main(argc, argv)
	int argc;
	char **argv;
{
	int ch, eval, tflag;

	tflag = 0;
	while ((ch = getopt(argc, argv, "t")) != EOF)
		switch(ch) {
		case 't':
			tflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (! *argv)
		usage();

	for (eval = 0; (archive = *argv++);)
		eval |= tflag ? touch() : build();
        return eval;
}

/*
 * For archive.c.
 */
char *rname(path)
	char *path;
{
	register char *ind;

	ind = rindex(path, '/');
	if (! ind)
	        return path;
	return ind + 1;
}

void badfmt()
{
	errno = EINVAL;
	error(archive);
}
