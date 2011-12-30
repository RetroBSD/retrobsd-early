/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifndef FILE

#include <sys/types.h>

#define	BUFSIZ	1024
extern	struct	_iobuf {
	int	_cnt;
	char	*_ptr;		/* should be unsigned char */
	char	*_base;		/* ditto */
	int	_bufsiz;
	short	_flag;
	char	_file;		/* should be short */
} _iob[];

#define	_IOREAD	01
#define	_IOWRT	02
#define	_IONBF	04
#define	_IOMYBUF	010
#define	_IOEOF	020
#define	_IOERR	040
#define	_IOSTRG	0100
#define	_IOLBF	0200
#define	_IORW	0400

/*
 * The following definition is for ANSI C, which took them
 * from System V, which brilliantly took internal interface macros and
 * made them official arguments to setvbuf(), without renaming them.
 * Hence, these ugly _IOxxx names are *supposed* to appear in user code.
*/
#define	_IOFBF	0	/* setvbuf should set fully buffered */
			/* _IONBF and _IOLBF are used from the flags above */

#ifndef	NULL
#define	NULL	0
#endif

#define	FILE	struct _iobuf
#define	EOF	(-1)

#define	stdin	(&_iob[0])
#define	stdout	(&_iob[1])
#define	stderr	(&_iob[2])

#ifndef lint
#define	getc(p)		(--(p)->_cnt>=0? (int)(*(unsigned char *)(p)->_ptr++):_filbuf(p))
#define putc(x, p)	(--(p)->_cnt >= 0 ?\
	(int)(*(unsigned char *)(p)->_ptr++ = (x)) :\
	(((p)->_flag & _IOLBF) && -(p)->_cnt < (p)->_bufsiz ?\
		((*(p)->_ptr = (x)) != '\n' ?\
			(int)(*(unsigned char *)(p)->_ptr++) :\
			_flsbuf(*(unsigned char *)(p)->_ptr, p)) :\
		_flsbuf((unsigned char)(x), p)))
#endif /* not lint */

#define	getchar()	getc(stdin)
#define	putchar(x)	putc(x,stdout)
#define	feof(p)		(((p)->_flag&_IOEOF)!=0)
#define	ferror(p)	(((p)->_flag&_IOERR)!=0)
#define	fileno(p)	((p)->_file)
#define	clearerr(p)	((p)->_flag &= ~(_IOERR|_IOEOF))

FILE	*fopen (const char *, const char *);
FILE	*fdopen (int, const char *);
FILE	*freopen (const char *, const char *, FILE *);
FILE	*popen (const char *, const char *);
int     fclose (FILE *);
long	ftell (FILE *);
int     fflush (FILE *);
int     fgetc (FILE *);
int     ungetc (int, FILE *);
int	fputc (int, FILE *);
int	fputs (const char *, FILE *);
int	puts (const char *);
char	*fgets (char *, int, FILE *);
char	*gets (char *);
FILE	*_findiop (void);
int     _filbuf (FILE *);
int     _flsbuf (unsigned char, FILE *);
void    setbuffer (FILE *, char *, size_t);
void    setlinebuf (FILE *);
int     fseek (FILE *, long, int);
void    rewind (FILE *stream);

size_t	fread (void *, size_t, size_t, FILE *);
size_t	fwrite (const void *, size_t, size_t, FILE *);

int	 fprintf (FILE *, const char *, ...);
int	 printf (const char *, ...);
int	 sprintf (char *, const char *, ...);
int	 snprintf (char *, size_t, const char *, ...);

int	 fscanf (FILE *, const char *, ...);
int	 scanf (const char *, ...);
int	 sscanf (const char *, const char *, ...);

#ifndef _VA_LIST_
#define va_list		__builtin_va_list	/* For GCC */
#endif

int	 vfprintf (FILE *, const char *, va_list);
int	 vprintf (const char *, va_list);
int	 vsprintf (char *, const char *, va_list);
int	 vsnprintf (char *, size_t, const char *, va_list);

int	 vfscanf (FILE *, const char *, va_list);
int	 vscanf (const char *, va_list);
int	 vsscanf (const char *, const char *, va_list);

int	_doprnt (const char *, va_list, FILE *);
int	_doscan (FILE *, const char *, va_list);

#ifndef _VA_LIST_
#undef va_list
#endif

void    perror (const char *);

#endif /* _FILE */
