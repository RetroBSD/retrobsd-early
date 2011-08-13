/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 */
#include "defs.h"

int
setbrk (incr)
{
	REG BYTPTR a = sbrk (incr);
	brkend = a + incr;
	return (int) a;
}
