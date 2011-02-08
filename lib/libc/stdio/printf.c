#include <stdio.h>

int
printf(fmt, args)
	char *fmt;
{
	_doprnt(fmt, &args, stdout);
	return ferror(stdout) ? EOF : 0;
}
