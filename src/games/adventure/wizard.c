/*
 * Re-coding of advent in C: privileged operations
 */
#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include "hdr.h"

char *magic;

void
datime(d, t)
int *d, *t;
{
        time_t now;
        struct tm *tm;

	time(&now);
	tm = localtime(&now);
	*d = tm->tm_yday +              /* day since 1977 (mod leap)    */
	        365 * (tm->tm_year - 77);
	/* bug: this will overflow in the year 2066 AD                  */
	/* it will be attributed to Wm the C's millenial celebration    */
	*t = tm->tm_hour * 60 +         /* and minutes since midnite    */
                tm->tm_min;             /* pretty painless              */
}

void
poof()
{       magic="dwarf";
	latncy=45;
}

int
wizard()                /* not as complex as advent/10 (for now)        */
{
	char *word,*x;
	if (!yesm(16,0,7)) return(FALSE);
	mspeak(17);
	getin(&word,&x);
	if (!weq(word,magic))
	{       mspeak(20);
		return(FALSE);
	}
	mspeak(19);
	return(TRUE);
}

int
start(n)
{       int d,t,delay;
	datime(&d,&t);
	delay=(d-saved)*1440+(t-savet); /* good for about a month       */
	if (delay>=latncy || delay < 0 || setup >= 0)
	{       saved= -1;
		return(FALSE);
	}
	printf("This adventure was suspended a mere %d minutes ago.",delay);
	if (delay<=latncy/3)
	{       mspeak(2);
		exit(0);
	}
	mspeak(8);
	if (! wizard())
	{       mspeak(9);
		exit(0);
	}
	saved = -1;
	return(FALSE);
}

void
ciao(cmdfile)
char *cmdfile;
{       register char *c;
	register int outfd, size;
	char fname[80], buf[512];
	extern unsigned filesize;

	lseek(datfd,(long)filesize,0);
	for (;;)
	{       printf("What would you like to call the saved version?\n");
		for (c=fname;; c++)
			if ((*c=getchar())=='\n') break;
		*c=0;
		if (save(cmdfile,fname)>=0) break;
		printf("I can't use that one.\n");
		return;
	}
	outfd=open(fname,1);
	lseek(outfd,0L,2);                /* end of executable file       */
	while ((size=read(datfd,buf,512))>0)
		write(outfd,buf,size);  /* copy the message data        */
	printf("                    ^\n");
	printf("That should do it.  Gis revido.\n");
	exit(0);
}

int
ran(range)                              /* uses unix rng                */
int range;                              /* can't div by 32768 because   */
{
	long i;
	i = rand() % range;
	return(i);
}
