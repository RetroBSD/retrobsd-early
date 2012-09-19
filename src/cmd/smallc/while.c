#include <stdio.h>
#include "defs.h"
#include "data.h"

addwhile (ptr)
        int     ptr[];
{
        int     k;

        if (wsptr == WSMAX) {
                error ("too many active whiles");
                return;
        }
        k = 0;
        while (k < WSSIZ)
                *wsptr++ = ptr[k++];
}

delwhile ()
{
        if (readwhile ())
                wsptr = wsptr - WSSIZ;
}

readwhile ()
{
        if (wsptr == ws) {
                error ("no active do/for/while/switch");
                return (0);
        }
        return CAST_INT (wsptr - WSSIZ);
}

findwhile ()
{
        int     *ptr;

        for (ptr = wsptr; ptr != ws;) {
                ptr = ptr - WSSIZ;
                if (ptr[WSTYP] != WSSWITCH)
                        return (CAST_INT ptr);
        }
        error ("no active do/for/while");
        return (0);
}

readswitch ()
{
        int     *ptr;

        ptr = CAST_INT_PTR readwhile ();
        if (ptr)
                if (ptr[WSTYP] == WSSWITCH)
                        return (CAST_INT ptr);
        return (0);
}

addcase (val)
        int     val;
{
        int     lab;

        if (swstp == SWSTSZ)
                error ("too many case labels");
        else {
                swstcase[swstp] = val;
                swstlab[swstp++] = lab = getlabel ();
                printlabel (lab);
                col ();
                nl ();
        }
}
