#include "defs.h"

int     mkfault;
char    line[LINSIZ];
int     infile;
char    *lp;
char    lastc = EOR;
int     eof;

/* input routines */

eol(c)
    char    c;
{
    return (c==EOR || c==';');
}

rdc()
{
    do {
        readchar();
    } while (lastc == SP || lastc == TB );
    return lastc;
}

readchar()
{
    if (eof) {
        lastc = '\0';
    } else {
        if (lp == 0) {
            lp = line;
            do {
                eof = (read(infile, lp, 1) == 0);
                if (mkfault)
                    error((char *)0);
            } while (eof == 0 && *lp++ != EOR);
            *lp = 0;
            lp = line;
        }
        if (lastc = *lp)
            lp++;
    }
    return lastc;
}

nextchar()
{
    if (eol(rdc())) {
        lp--;
        return 0;
    }
    return lastc;
}

quotchar()
{
    if (readchar() == '\\')
        return readchar();
    if (lastc == '\'')
        return 0;
    return lastc;
}

getformat(deformat)
    char    *deformat;
{
    register char   *fptr;
    register int    quote;

    fptr = deformat;
    quote = FALSE;
    while ((quote ? readchar()!=EOR : !eol(readchar()))) {
        if ((*fptr++ = lastc) == '"') {
            quote = ~quote;
        }
    }
    lp--;
    if (fptr != deformat) {
        *fptr++ = '\0';
    }
}
