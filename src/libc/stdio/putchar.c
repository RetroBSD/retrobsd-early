/*
 * A subroutine version of the macro putchar
 */
#define	USE_STDIO_MACROS
#include <stdio.h>

#undef putchar

putchar(c)
register c;
{
	putc(c, stdout);
}
