#include <stdlib.h>
#include <unistd.h>

void
exit(code)
	int code;
{
	_cleanup();
	_exit(code);
}
