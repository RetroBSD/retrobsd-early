/*
 * Calloc - allocate and clear memory block
 */
#include <sys/types.h>
#include <stdlib.h>
#include <strings.h>

void *
calloc(num, size)
	size_t num, size;
{
	register char *p;

	size *= num;
	if (p = malloc(size))
		bzero(p, size);
	return (p);
}

cfree(p, num, size)
	char *p;
	unsigned num;
	unsigned size;
{
	free(p);
}
