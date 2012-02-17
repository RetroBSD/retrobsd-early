#include "defs.h"
#include <sys/file.h>

MAP     txtmap;
MAP     datmap;
int     wtflag;
int     kernel;
int     fcor;
int     fsym;
long    maxfile;
long    maxstor;
long    txtsiz;
long    datsiz;
long    bss;
long    datbas;
long    stksiz;
char    *errflg;
int     magic;
long    entrypt;
int     argcount;
int     signo;
u_int   corhdr[USIZE/sizeof(u_int)];
u_int   *uar0 = UAR0;
char    *symfil  = "a.out";
char    *corfil  = "core";
off_t   symoff, stroff;

extern long var[];

setsym()
{
    struct exec hdr;

    bzero(&txtmap, sizeof (txtmap));
    fsym = getfile(symfil, 1);
    txtmap.ufd = fsym;

    if (read(fsym, &hdr, sizeof (hdr)) >= (int)sizeof (hdr)) {
        magic= hdr.a_magic;
        txtsiz = hdr.a_text;
        datsiz = hdr.a_data;
        bss = hdr.a_bss;
        entrypt = hdr.a_entry;

        txtmap.f1 = N_TXTOFF(hdr);
        symoff = N_SYMOFF(hdr);
        // TODO: stroff = N_STROFF(hdr);

        switch (magic) {
        case 0407:
            txtmap.e1 = txtmap.e2 = txtsiz+datsiz;
            txtmap.f2 = txtmap.f1;
            break;
        case 0405:
        case 0411:
            txtmap.e1 = txtsiz;
            txtmap.e2 = datsiz;
            txtmap.f2 = txtsiz + txtmap.f1;
            break;
        default:
            magic = 0;
            txtsiz = 0;
            datsiz = 0;
            bss = 0;
            entrypt = 0;
        }
        datbas = txtmap.b2;
        symINI(&hdr);
    }
    if (magic==0) { txtmap.e1=maxfile; }
}

setcor()
{
    fcor = getfile(corfil, 2);
    datmap.ufd = fcor;
    if (read(fcor, corhdr, sizeof corhdr)==sizeof corhdr
    ) {    if (!kernel
        ) {    txtsiz = ((U*)corhdr)->u_tsize << 6;
            datsiz = ((U*)corhdr)->u_dsize << 6;
            stksiz = ((U*)corhdr)->u_ssize << 6;
            datmap.f1 = USIZE;
            datmap.b2 = maxstor-stksiz;
            datmap.e2 = maxstor;
        } else {    datsiz = roundn(datsiz + bss, 64L);
            stksiz = (long) USIZE;
            datmap.f1 = 0;
            datmap.b2 = 0140000L;
            datmap.e2 = 0140000L + USIZE;
        }
        switch (magic) {
        case 0407:
            datmap.b1 = 0;
            datmap.e1 = txtsiz+datsiz;
            if (kernel
) {    datmap.f2 = (long)corhdr[KA6] *
                    0100L;
} else {    datmap.f2 = USIZE + txtsiz + datsiz;
            }
            break;

        case 0405:
        case 0411:
            datmap.b1 = 0;
            datmap.e1 = datsiz;
            if (kernel
) { datmap.f2 = (long)corhdr[KA6] *
                0100L;
} else { datmap.f2 = datsiz + USIZE;
            }
            break;

        default:
            magic = 0;
            datmap.b1 = 0;
            datmap.e1 = maxfile;
            datmap.f1 = 0;
            datmap.b2 = 0;
            datmap.e2 = 0;
            datmap.f2 = 0;
        }
        datbas = datmap.b1;
#if 0
        if (!kernel && magic) {
            /*
             * Note that we can no longer compare the magic
             * number of the core against that of the object
             * since the user structure no longer contains
             * the exec header ...
             */
            register u_int *ar0;
            ar0 = (u_int *)(((U *)corhdr)->u_ar0);
            if (ar0 > (u_int *)0140000
                && ar0 < (u_int *)(0140000 + USIZE)
                && !((unsigned)ar0&01))
                uar0 = (u_int *)&corhdr[ar0-(u_int *)0140000];
        }
#endif
    } else {
        datmap.e1 = maxfile;
    }
}

getfile(filnam, cnt)
    char    *filnam;
    int     cnt;
{
    register int f;

    if (strcmp("-", filnam)
    ) {    f = open(filnam, wtflag);
        if (f < 0 && argcount>cnt
        ) {    if (wtflag
            ) {    f = open(filnam, O_CREAT | O_TRUNC | wtflag, 644);
            }
            if (f < 0
            ) { print("cannot open `%s'\n", filnam);
            }
        }
    } else {    f = -1;
    }
    return(f);
}
