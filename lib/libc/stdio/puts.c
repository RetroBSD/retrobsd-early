#include <stdio.h>

int
puts(s)
	register const char *s;
{
	register c;

	while (c = *s++)
		putchar(c);
	return(putchar('\n'));
}
