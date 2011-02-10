#include <stdlib.h>
#include <unistd.h>

int errno;

void
exit (code)
	int code;
{
	_cleanup();
	_exit (code);
}
