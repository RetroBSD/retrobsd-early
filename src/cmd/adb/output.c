#include "defs.h"

int     outfile = 1;
char    *printptr = printbuf;

static char *digitptr;

void
printc(c)
    int c;
{
    char d;
    char *q;
    int  posn, tabs, p;

    if (mkfault)
        return;

    *printptr = c;
    if (c == EOR) {
        tabs = 0;
        posn = 0;
        q = printbuf;
        for (p=0; p<printptr-printbuf; p++) {
            d = printbuf[p];
            if ((p & 7)==0 && posn) {
                tabs++;
                posn = 0;
            }
            if (d == SP) {
                posn++;
            } else {
                while (tabs > 0) {
                    *q++ = TB;
                    tabs--;
                }
                while (posn > 0) {
                    *q++ = SP;
                    posn--;
                }
                *q++ = d;
            }
        }
        *q++ = EOR;
        write(outfile, printbuf, q - printbuf);
        printptr = printbuf;

    } else if (c == TB) {
        *printptr++ = SP;
         while ((printptr - printbuf) & 7) {
            *printptr++ = SP;
        }
    } else if (c) {
        printptr++;
    }
    if (printptr >= &printbuf[MAXLIN-9]) {
        write(outfile, printbuf, printptr - printbuf);
        printptr = printbuf;
    }
}

void
flushbuf()
{
    if (printptr != printbuf) {
        printc(EOR);
    }
}

static int
convert(cp)
    register char **cp;
{
    register char c;
    int     n = 0;

    while ((c = *(*cp)++) >= '0' && c <= '9') {
        n = n*10 + c - '0';
    }
    (*cp)--;
    return n;
}

static void
printnum(n, fmat, base)
    register int n;
{
    register char k;
    register int *dptr;
    int digs[15];

    dptr = digs;
    if (n < 0 && fmat == 'd') {
        n = -n;
        *digitptr++ = '-';
    }
    while (n) {
        *dptr++ = ((u_int) n) % base;
        n = ((u_int) n ) / base;
    }
    if (dptr == digs) {
        *dptr++ = 0;
    }
    while (dptr != digs) {
        k = *--dptr;
        *digitptr++ = k + ((k <= 9) ? '0' : 'a'-10);
    }
}

static void
printoct(o, s)
    long    o;
    int     s;
{
    int     i;
    long    po = o;
    char    digs[12];

    if (s) {
        if (po < 0) {
            po = -po;
            *digitptr++ = '-';

        } else if (s > 0)
            *digitptr++='+';
    }
    for (i=0; i<=11; i++) {
        digs[i] = po & 7;
        po >>= 3;
    }
    digs[10] &= 03;
    digs[11] = 0;
    for (i=11; i>=0; i--) {
        if (digs[i])
            break;
    }
    for (i++; i>=0; i--) {
        *digitptr++ = digs[i] + '0';
     }
}

static void
printlong(lx, fmat, base)
    long    lx;
    int     fmat, base;
{
    int digs[20], *dptr;
    char k;
    unsigned long f, g;

    dptr = digs;
    f = lx;
    if (fmat == 'x')
        *digitptr++ = '#';
    else if (fmat == 'D' && lx < 0) {
        *digitptr++ = '-';
        f = -lx;
    }
    while (f) {
        g = f / base;
        *dptr++ = f - g * base;
        f = g;
    }
    if (dptr == digs) {
        *dptr++ = 0;
    }
    while (dptr != digs) {
        k = *--dptr;
        *digitptr++ = k + ((k <= 9) ? '0' : 'a'-10);
    }
}

static void
printdate(tvec)
    long    tvec;
{
    register int i;
    register char *timeptr;

    timeptr = ctime(&tvec);
    for (i=20; i<24; i++)
        *digitptr++ = timeptr[i];
    for (i=3; i<19; i++)
        *digitptr++ = timeptr[i];
}

void
print(char *fmat, ...)
{
    char    *fptr, *s;
    int     *vptr;
    long    *dptr;
    double  *rptr;
    int     width, prec;
    char    c, adj;
    int     x, decpt, n;
    long    lx;
    char    digits[64];

    fptr = fmat;
    vptr = 1 + (int*) &fmat;

    while ((c = *fptr++)) {
        if (c != '%') {
            printc(c);
        } else {
            if (*fptr == '-') {
                adj = 'l';
                fptr++;
            } else {
                adj = 'r';
            }
            width = convert(&fptr);
            if (*fptr == '*') {
                width = *vptr++;
                fptr++;
            }
            if (*fptr == '.') {
                fptr++;
                prec = convert(&fptr);
                if (*fptr == '*') {
                    prec = *vptr++;
                    fptr++;
                }
            } else
                prec = -1;

            digitptr = digits;
            rptr = (double*) vptr;
            dptr = (long*) vptr;
            lx = *dptr;
            x = *vptr++;
            s = 0;
            switch (c = *fptr++) {
            case 'd':
            case 'u':
                printnum(x, c, 10);
                break;
            case 'o':
                printoct((long) (unsigned) x, 0);
                break;
            case 'q':
                printlong((long) (unsigned) x, 'x', 16);
                break;
            case 'x':
                printlong((long) (unsigned) x, c, 16);
                break;
            case 'Y':
                printdate(lx);
                //vptr++;
                break;
            case 'D':
            case 'U':
                printlong(lx, c, 10);
                //vptr++;
                break;
            case 'O':
                printoct(lx, 0);
                //vptr++;
                break;
            case 'Q':
                printlong(lx, 'x', 16);
                //vptr++;
                break;
            case 'X':
                printlong(lx, 'x', 16);
                //vptr++;
                break;
            case 'c':
                printc(x);
                break;
            case 's':
                s = (char*) x;
                break;
            case 'f':
            case 'F':
                //vptr += 7;
                s = ecvt(*rptr, prec, &decpt, &n);
                *digitptr++ = (n?'-':'+');
                *digitptr++ = (decpt <= 0) ? '0' : *s++;
                if (decpt > 0) {
                    decpt--;
                }
                *digitptr++ = '.';
                while (*s && prec-- ) {
                    *digitptr++ = *s++;
                }
                while (*--digitptr=='0');
                digitptr += (digitptr - digits >= 3) ? 1 : 2;
                if (decpt) {
                    *digitptr++ = 'e';
                    printnum(decpt, 'd', 10);
                }
                s = 0;
                prec = -1;
                break;
            case 'm':
                vptr--;
                break;
            case 'M':
                width = x;
                break;
            case 'T':
            case 't':
                if (c == 'T') {
                    width = x;
                } else {
                    vptr--;
                }
                if (width) {
                    width -= (printptr - printbuf) % width;
                }
                break;
            default:
                printc(c);
                vptr--;
            }

            if (s == 0) {
                *digitptr = 0;
                s = digits;
            }
            n = strlen(s);
            n = (prec < n && prec >= 0) ? prec : n;
            width -= n;
            if (adj == 'r') {
                while (width-- > 0)
                    printc(SP);
            }
            while (n--)
                printc(*s++);
            while (width-- > 0)
                printc(SP);
            digitptr = digits;
        }
    }
}

#define MAXIFD  5
struct {
    int     fd;
    long    r9;
} istack[MAXIFD];

int ifiledepth;

void
iclose(stack, err)
{
    if (err) {
        if (infile) {
            close(infile);
            infile = 0;
        }
        while (--ifiledepth >= 0) {
            if (istack[ifiledepth].fd) {
                close(istack[ifiledepth].fd);
                infile = 0;
            }
        }
        ifiledepth = 0;
    } else if (stack == 0) {
        if (infile) {
            close(infile);
            infile = 0;
        }
    } else if (stack > 0) {
        if (ifiledepth >= MAXIFD) {
            error(TOODEEP);
        }
        istack[ifiledepth].fd = infile;
        istack[ifiledepth].r9 = var[9];
        ifiledepth++;
        infile = 0;
    } else {
        if (infile) {
            close(infile);
            infile = 0;
        }
        if (ifiledepth > 0) {
            infile = istack[--ifiledepth].fd;
            var[9] = istack[ifiledepth].r9;
        }
    }
}

void
oclose()
{
    if (outfile != 1) {
        flushbuf();
        close(outfile);
        outfile = 1;
    }
}

void
endline()
{
    if ((printptr - printbuf) >= maxpos)
        printc('\n');
}
