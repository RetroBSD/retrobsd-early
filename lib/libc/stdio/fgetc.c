#include <stdio.h>

fgetc(fp)
register FILE *fp;
{
	return(getc(fp));
}
