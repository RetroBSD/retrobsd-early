#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "defs.h"
#include "data.h"

main (argc, argv)
    int argc;
    char *argv[];
{
    char *p = NULL, *bp;
    ctext = 0;
    errs = 0;
    aflag = 1;
    int i=1;
    for (; i<argc; i++) {
        p = argv[i];
        if (*p == '-') {
            while (*++p) {
                switch (*p) {
                    case 't': case 'T': // output c source as asm comment
                        ctext = 1;
                        break;
                    case 'a': case 'A':
                        aflag = 0;
                        break;
                    default:
                        usage();
                }
            }
        } else {
            break;
        }
    }

    if (!p) {
        usage();
    }

    for (; i<argc; i++) {
        p = argv[i];
        errfile = 0;
        if (filename_typeof(p) == 'c') {
            glbptr = STARTGLB;
            locptr = STARTLOC;
            wsptr = ws;
            inclsp =
            swstp =
            litptr =
            stkp =
            errcnt =
            ncmp =
            lastst =
            quote[1] =
            0;
            input2 = NULL;
            quote[0] = '"';
            cmode = 1;
            glbflag = 1;
            nxtlab = 0;
            litlab = getlabel();
            addglb("memory", ARRAY, CCHAR, 0, EXTERN);
            addglb("stack", ARRAY, CCHAR, 0, EXTERN);
            rglbptr = glbptr;

	    /* FIXME currently not defined so comment out for the moment */
            /*addglb("etext", ARRAY, CCHAR, 0, EXTERN);*/
            /*addglb("edata", ARRAY, CCHAR, 0, EXTERN);*/

            /*
             *      compiler body
             */
            if (!openin(p))
                return;
            if (!openout())
                return;
            header();
            gtext();
            parse();
            fclose(input);
            gdata();
            dumplits();
            dumpglbs();
            errorsummary();
            trailer();
            fclose(output);
            pl("");
            errs = errs || errfile;
        } else {
            fputs("Don't understand file ", stderr);
            fputs(p, stderr);
            errs = 1;
        }
    }
    exit(errs != 0);
}

FEvers()
{
    outstr("\tFront End 2.7, 1984/11/28");
}

/**
 * prints usage
 * @return exits the execution
 */
usage()
{
    fputs("Usage:\n", stderr);
    fputs("    smallc [-t] [-a] file.c ...\n", stderr);
    fputs("\nOptions:\n", stderr);
    fputs("    -t    Output C source as asm comment\n", stderr);
    fputs("    -a    Do not use arg count register\n", stderr);
    exit(1);
}

/**
 * process all input text
 *
 * at this level, only static declarations, defines, includes,
 * and function definitions are legal.
 */
parse()
{
    while (!feof(input)) {
        if (amatch("extern", 6))
            dodcls(EXTERN);
        else if (amatch("static", 6))
            dodcls(STATIC);
        else if (dodcls(PUBLIC));
        else if (match("__asm__"))
            doasm();
        else
            newfunc();
        blanks();
    }
}

/**
 * parse top level declarations
 * @param stclass
 * @return
 */
dodcls (stclass)
    int stclass;
{
    blanks();
    if (amatch("char", 4))
        declglb(CCHAR, stclass);
    else if (amatch("int", 3))
        declglb(CINT, stclass);
    else if (stclass == PUBLIC)
        return (0);
    else
        declglb(CINT, stclass);
    ns();
    return (1);
}

/**
 * dump the literal pool
 */
dumplits()
{
    int j, k;

    if (litptr == 0)
        return;
    printlabel(litlab);
    col();
    k = 0;
    while (k < litptr) {
        defbyte();
        j = 8;
        while (j--) {
            onum(litq[k++] & 127);
            if ((j == 0) | (k >= litptr)) {
                nl();
                break;
            }
            outbyte(',');
        }
    }
}

/**
 * dump all static variables
 */
dumpglbs()
{
    int j;

    if (!glbflag)
        return;
    cptr = rglbptr;
    while (cptr < glbptr) {
        if (cptr[IDENT] != FUNCTION) {
            ppubext(cptr);
            if (cptr[STORAGE] != EXTERN) {
                prefix();
                outstr(cptr);
                col();
                defstorage();
                j = glint(cptr);
                if ((cptr[TYPE] == CINT) ||
                        (cptr[IDENT] == POINTER))
                    j = j * intsize();
                onum(j);
                nl();
            }
        } else {
            fpubext(cptr);
        }
        cptr = cptr + SYMSIZ;
    }
}

/*
 *      report errors
 */
errorsummary()
{
    if (ncmp)
        error("missing closing bracket");
    nl();
    comment();
    outdec(errcnt);
    if (errcnt) errfile = YES;
    outstr(" error(s) in compilation");
    nl();
    comment();
    ot("literal pool:");
    outdec(litptr);
    nl();
    comment();
    ot("global pool:");
    outdec(glbptr - rglbptr);
    nl();
    comment();
    pl(errcnt ? "Error(s)" : "No errors");
}

/**
 * test for C or similar filename, e.g. xxxxx.x, tests the dot at end-1 postion
 * @param s the filename
 * @return the last char if it contains dot, space otherwise
 */
filename_typeof (s)
    char *s;
{
    s += strlen(s) - 2;
    if (*s == '.')
        return (*(s + 1));
    return (' ');
}

/**
 * "asm" pseudo-statement
 * enters mode where assembly language statements are passed
 * intact through parser
 */
doasm ()
{
        cmode = 0;
        for (;;) {
                readline ();
                if (match ("__endasm__"))
                        break;
                if (feof (input))
                        break;
                outstr (line);
                nl ();
        }
        kill ();
        cmode = 1;
}
