#include <stdio.h>

void
_strout(count, string, adjust, file, fillch)
        register char *string;
        register int count;
        int adjust;
        register FILE *file;
{
	while (adjust < 0) {
		if (*string=='-' && fillch=='0') {
			putc(*string++, file);
			count--;
		}
		putc(fillch, file);
		adjust++;
	}
	while (--count>=0)
		putc(*string++, file);
	while (adjust) {
		putc(fillch, file);
		adjust--;
	}
}
