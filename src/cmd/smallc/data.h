/*      File data.h: 2.2 (84/11/27,16:26:11) */

/* storage words */

extern SYMBOL symbol_table[NUMBER_OF_GLOBALS + NUMBER_OF_LOCALS]; //extern  char    symtab[];
extern int global_table_index, rglobal_table_index; //extern  char    *glbptr, *rglbptr, *locptr;
extern int local_table_index;
extern  WHILE ws[]; //int     ws[];
extern  int     while_table_index; //*wsptr;
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

extern  FILE    *input, *output;
extern  int     inclsp;

extern  char    quote[];
extern  int     current_symbol_table_idx; //extern  char    *cptr;
extern  int     *iptr;
extern  int     fexitlab;
extern  int     errfile;
extern  int     errs;

extern char initials_table[INITIALS_SIZE];      // 5kB space for initialisation data
extern char *initials_table_ptr;
