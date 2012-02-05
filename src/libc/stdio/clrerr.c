#include <stdio.h>
#undef	clearerr

clearerr(iop)
	register FILE *iop;
{
	iop->_flag &= ~(_IOERR|_IOEOF);
}
