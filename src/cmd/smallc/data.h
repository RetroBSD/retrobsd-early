/* storage words */

extern  char    symtab[];
extern  char    *glbptr, *rglbptr, *locptr;
extern  int     ws[];
extern  int     *wsptr;
extern  int     swstcase[];
extern  int     swstlab[];
extern  int     swstp;
extern  char    litq[];
extern  int     litptr;
extern  char    line[];
extern  int     lptr;

/* miscellaneous storage */

extern  int     nxtlab,
                litlab,
                stkp,
                argstk,
                ncmp,
                errcnt,
                glbflag,
                ctext,
                cmode,
                lastst;

extern  FILE    *input, *input2, *output;
extern  FILE    *inclstk[];
extern  int     inclsp;
extern  char    fname[];

extern  char    quote[];
extern  char    *cptr;
extern  int     *iptr;
extern  int     fexitlab;
extern  int     errfile;
extern  int     sflag;
extern  int     cflag;
extern  int     errs;
extern  int     aflag;
