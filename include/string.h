/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)string.h	5.1.3 (2.11BSD) 1996/3/20
 */

#include <sys/types.h>

#ifndef	NULL
#define	NULL	0
#endif

//extern	char	*strcat(), *strncat(), *strcpy(), *strncpy(), *index();
char	*strcat(char * __restrict, const char * __restrict);
char	*strncat(char * __restrict, const char * __restrict, size_t);
char	*strcpy(char * __restrict, const char * __restrict);
char	*strncpy(char * __restrict, const char * __restrict, size_t);

//extern	char	*rindex(), *strstr(), *syserrlst();
char	*strstr(const char *, const char *);

//extern	int	strcmp(), strncmp(), strcasecmp(), strncasecmp(), strlen();
int	 strcmp(const char *, const char *);
int	 strncmp(const char *, const char *, size_t);
size_t	 strlen(const char *);

//extern	int	memcmp();
int	 memcmp(const void *, const void *, size_t);

//extern	char	*memccpy(), *memchr(), *memcpy(), *memset(), *strchr();
void	*memccpy(void *, const void *, int, size_t);
void	*memchr(const void *, int, size_t);
void	*memcpy(void * __restrict, const void * __restrict, size_t);
void	*memset(void *, int, size_t);
char	*strchr(const char *, int);

//extern	char	*strdup(), *strpbrk(), *strrchr(), *strsep(), *strtok();
char	*strdup(const char *);
char	*strpbrk(const char *, const char *);
char	*strrchr(const char *, int);
char	*strsep(char **, const char *);
char	*strtok(char * __restrict, const char * __restrict);

//extern	size_t	strcspn(), strspn();
size_t	 strcspn(const char *, const char *);
size_t	 strspn(const char *, const char *);

//extern	char	*strerror();
const char *strerror(int);
