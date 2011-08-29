/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */
#include "defs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>

#define MAXDIR  64
struct direct           *getdir();

static struct direct    dirbuf[MAXDIR];
static int              nxtdir = -1;
static int              maxdir = 0;
static char             entry[MAXNAMLEN+1];

#define XXX 0200

static int
addg(as1, as2, as3)
char    *as1, *as2, *as3;
{
	register char   *s1, *s2;
	register int    c;

	s2 = locstak() + BYTESPERWORD;
	s1 = as1;
	while (c = /* @@@ *s1++ */ cii(*s1++))
	{
		if (/* @@@ (c &= STRIP)*/ (c=smask(c)) == 0)
		{
			*s2++ = '/';
			break;
		}
		*s2++ = c;
	}
	s1 = as2;
	while (*s2 = *s1++)
		s2++;
	if (s1 = as3)
	{
		*s2++ = '/';
		while (*s2++ = *++s1);
	}
	makearg(endstak(s2));
}

/*
 * globals (file name generation)
 *
 * "*" in params matches r.e ".*"
 * "?" in params matches r.e. "."
 * "[...]" in params matches character class
 * "[...a-z...]" in params matches a through z.
 *
 */
expand(as, rcnt)
	char    *as;
{
	int     count;
	BOOL    dir = 0;
	char    *rescan = NIL;
	register char   *s, *cs;
	struct argnod   *schain = gchain;
	struct stat statb;
	BOOL    slash;
	DIR* dirp;
	char    *useAsDir;

	if (trapnote & SIGSET)
		return(0);
	s = cs = as;

	/*
	 * check for meta chars
	 */
	{
		register BOOL open;

		slash = 0;
		open = 0;
		do
		{
			switch (*cs++)
			{
			case 0:
				if (rcnt && slash)
					break;
				else
					return(0);

			case '/':
				slash++;
				open = 0;
				continue;

			case '[':
				open++;
				continue;

			case ']':
				if (open == 0)
					continue;

			case '?':
			case '*':
				if (rcnt > slash)
					continue;
				else
					cs--;
				break;


			default:
				continue;
			}
			break;
		} while (TRUE);
	}

	for (;;)
	{
		if (cs == s)
		{
			s = nullstr;
			break;
		}
		else if (*--cs == '/')
		{
			*cs = 0;
			if (s == cs)
				s = "/";
			break;
		}
	}

	useAsDir = (*s ? s : ".");
	if (stat(useAsDir, &statb) != -1 &&
            (statb.st_mode & S_IFMT) == S_IFDIR &&
            (dirp = opendir(useAsDir)) != 0)
		dir++;

	count = 0;
	if (*cs == 0)
		*cs++ = XXX;
	if (dir)                /* check for rescan */
	{
		register char *rs;
		struct direct *dp;

		rs = cs;
		do
		{
			if (*rs == '/')
			{
				rescan = rs;
				*rs = 0;
				gchain = NIL;
			}
		} while (*rs++);

		while ((dp = readdir(dirp)) && (trapnote & SIGSET) == 0)
		{
			*(movstrn(dp->d_name, entry, MAXNAMLEN)) = 0;

			if (entry[0] == '.' && *cs != '.')
#ifndef BOURNE
				continue;
#else
			{
				if (entry[1] == 0)
					continue;
				if (entry[1] == '.' && entry[2] == 0)
					continue;
			}
#endif

			if (gmatch(entry, cs))
			{
				addg(s, entry, rescan);
				count++;
			}
		}
		closedir(dirp);

		if (rescan)
		{
			register struct argnod  *rchain;

			rchain = gchain;
			gchain = schain;
			if (count)
			{
				count = 0;
				while (rchain)
				{
					count += expand(rchain->argval, slash + 1);
					rchain = rchain->argnxt;
				}
			}
			*rescan = '/';
		}
	}

	{
		register char   c;

		s = as;
		while (c = *s)
			/* @@@ *s++ = (c & STRIP ? c : '/'); */
			*s++ = smask(c) ? c: '/';
	}
	return(count);
}

gmatch(s, p)
register char   *s, *p;
{
	register int    scc;
	char            c;

	if (scc = /* @@@ *s++ */ cii( *s++))
	{
		if ( /* @@@ (scc &= STRIP) */  smask(scc) == 0)
			scc=XXX;
	}
	switch (c = /* @@@ *p++ */ cii( *p++) )
	{
	case '[':
		{
			BOOL ok;
			int lc;
			int notflag = 0;

			ok = 0;
			lc = 077777;
			if (*p == '!')
			{
				notflag = 1;
				p++;
			}
			while (c = /* @@@ *p++ */ cii(*p++))
			{
				if (c == ']')
					return(ok ? gmatch(s, p) : 0);
				else if (c == MINUS)
				{
					if (notflag)
					{
						if (scc < lc || scc > *p++)
							ok++;
						else
							return(0);
					}
					else
					{
						if (lc <= scc && scc <= *p++)
							ok++;
					}
				}
				else
				{
					/* @@@ lc = c & STRIP; */
					lc = ismask(c);

					if (notflag)
					{
						if (scc && scc != lc)
							ok++;
						else
							return(0);
					}
					else
					{
						if (scc == lc)
							ok++;
					}
				}
			}
			return(0);
		}

	default:
		if (/* @@@ (c & STRIP)*/ ismask(c) != scc)
			return(0);

	case '?':
		return(scc ? gmatch(s, p) : 0);

	case '*':
		while (*p == '*')
			p++;

		if (*p == 0)
			return(1);
		--s;
		while (*s)
		{
			if (gmatch(s++, p))
				return(1);
		}
		return(0);

	case 0:
		return(scc == 0);
	}
}

makearg(args)
	register struct argnod *args;
{
	args->argnxt = gchain;
	gchain = args;
}
