/*
 * Main definitions.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#ifdef DEBUG
#   define DEBUGCHECK checkfsd()   /* check fsd consistency for debugging */
#else
#   define DEBUGCHECK /* */
#endif

#define EDITED      2       /* Значение openwrite, если файл редактировался */

#define esc0        COCURS
#define esc1        '\\'

#define MOVECMD(x)  ((x) >= CCMOVEUP && (x) <= CCBACKTAB)
#define CTRLCHAR(x) ((((x) >= 0) && ((x) < ' ')) || ((lread1 >= 0177) && (lread1 <= 0240)))
#define LINELM      128     /* макс. длина строки на экране */
#define NLINESM     48      /* макс. число строк на экране  */
#define LBUFFER     256     /* *** НЕ МЕНЬШЕ  fsdmaxl  **** */
#define NEWLINE     012     /* newline  */
#define PARAMREDIT  40      /* редактируемая часть paramport */
#define PARAMRINFO  78      /* последняя колонка в paramport */
#define NPARAMLINES 1       /* число строк в PARAMPORT */
#define FILEMODE    0664    /* режим доступа к создаваемым файлам */
#define NTABS       30
#define BIGTAB      32767

#define BADF        -1
#define CONTF       -2

/* OUTPUT CODES */

#define COSTART     0
#define COUP        1
#define CODN        2
#define CORN        3
#define COHO        4
#define CORT        5
#define COLT        6
#define COCURS      7
#define COBELL      8
#define COFIN       9
#define COERASE     10
#define COMCOD      11      /* Число выходных кодов */

/* margin characters */
#define LMCH        '|'
#define RMCH        '|'
#define TMCH        '-'
#define BMCH        '-'
#define MRMCH       '>'
#define MLMCH       '<'
#define ELMCH       ';'
#define DOTCH       '+'

/* struct fsd -
 * Описатель сегмента файла. Описывает от 1 до 127 строк файла,
 * записанных подряд. Это минимальная компонента цепочки описателей
 */
#define FSDMAXL     127     /* Макс. число строк в fsd */

struct fsd {
        struct fsd *backptr;    /* Ссылка на пред. fsd   */
        struct fsd *fwdptr;     /* Ссылка на след. fsd   */
        int fsdnlines;          /* Число строк, которые описывает fsd */
                                /* 0 , если это конец цепи. */
        int fsdfile;            /* Дескриптор файла, 0, если это конец цепи */
        off_t seek;             /* Сдвиг в файле */
        char fsdbytes;  /* Переменное число байт - столько, сколько нужно
                        для того, чтобы описать очередные fsdnlines-1 строк:
                        Интерпретация очередного байта:
                        1-127   смещение следующей строки от предыдущей
                        0       здесь помещается пустая строка
                        -n      след. строка начинается с n*128+next байта
                                после начала предыдущей строки.
                        Имеется по меньшей мере fsdnlines-1 байтов, но может
                        быть и больше, если есть длинные строки
                        Отметим, что в принципе в одном fsd можно описать и
                        несмежные строки, но такая возможность не учтена
                        в функциях записи файла.               */
};

/* Урезанный вариант - без fsdbyte */
struct fsdsht {
    struct fsd *backptr, *fwdptr;
    int fsdnlines;
    int fsdfile;
    off_t seek;
};

#define SFSD        (sizeof (struct fsdsht))
#define MAXFILES    14

struct fsd *openfsds[MAXFILES];
char *openfnames[MAXFILES],
     openwrite[MAXFILES],       /* 1 - файл можно записывать */
     movebak[MAXFILES];         /* 1 - файл уже двигали в .bak */
int  nlines[MAXFILES];          /* Число непустых строк в файле */

/* workspace - структура, которая описывает файл */
struct workspace {
        struct fsd *curfsd;     /* ptr на текущий fsd */
        int ulhclno;            /* номер строки, верхней на экране */
        int ulhccno;            /* номер колонки, которая 0 на экране */
        int curlno;             /* текущий номер строки */
        int curflno;            /* номер строки, первой в curfsd */
        int wfile;              /* номер файла, 0 - если нет вообще */
        int ccol;               /* curorcol, когда не активен */
        int crow;               /* cursorline, когда неактивен */
};
#define SWKSP (sizeof (struct workspace))

struct workspace *curwksp, *pickwksp;
int curfile;

/*
 * viewport - описатель окна на экране терминала
 * все координаты на экране, а также ltext и ttext, измеряются по отношению
 *      к (0,0) = начало экрана. Остальные 6 границ приводятся по отношению
 *      к ltext и ttext.
 */
#define SVIEWPORT (sizeof (struct viewport))

struct viewport {
        struct workspace *wksp; /* Ссылка на workspace         */
        struct workspace *altwksp;      /* Альтернативное workspace */
        int prevport;           /* Номер пред. окна   */
        int ltext;              /* Границы  текста (по отн. к 0,0) */
        int rtext;              /* Длина в ширину                  */
        int ttext;              /* Верхняя граница                 */
        int btext;              /* Высота окна                     */
        int lmarg;              /* границы окна == ..text или +1 */
        int rmarg;
        int tmarg;
        int bmarg;
        int ledit;              /* область редактирования в окне */
        int redit;
        int tedit;
        int bedit;
        char *firstcol;         /* Номера первых колонок, !=' '  */
        char *lastcol;          /*  -//-  последних -//-         */
        char *lmchars;          /* символы - левые ограничители  */
        char *rmchars;          /* символы - правые ограничители */
};

#define MAXPORTLIST 10
struct viewport *portlist[MAXPORTLIST],
                *curport,       /* текущее окно */
                wholescreen,    /* весь экран   */
                paramport;      /* окно для параметров */
int nportlist;

/* savebuf - структура, которая описывает буфер вставок */

#define SSAVEBUF (sizeof (struct savebuf))

struct savebuf {
        int linenum;    /* Номер первой строки в "#" */
        int nrows;      /* Число строк               */
        int ncolumns;   /* Число колонок             */
};
struct savebuf *pickbuf, *deletebuf;

/*
 * Управляющие символы
 */
#define CCCTRLQUOTE     0       /* knockdown next char  ^P          */
#define CCMOVEUP        1       /* move up 1 line               up  */
#define CCMOVEDOWN      2       /* move down 1 line             down */
#define CCRETURN        3       /* return               ^M          */
#define CCHOME          4       /* home cursor          ^X h    home */
#define CCMOVERIGHT     5       /* move right                   right */
#define CCMOVELEFT      6       /* move left                    left */
#define CCTAB           7       /* tab                  ^I          */
#define CCBACKTAB       010      /* tab left             ^X t        */
#define CCPICK          011     /* pick                         f6 */
#define CCMAKEPORT      012     /* make a viewport              f4 */
#define CCOPEN          013     /* insert                       f7 */
#define CCSETFILE       014     /* set file                     f5 */
#define CCCHPORT        015     /* change port                  f3 */
#define CCPLPAGE        016     /* minus a page                 page down */
#define CCGOTO          017     /* goto linenumber              f10 */
#define CCDOCMD         020     /* execute a filter     ^X x        */
#define CCMIPAGE        021     /* plus a page                  page up */
#define CCPLSRCH        022     /* plus search          ^F          */
#define CCRPORT         023     /* port right           ^X f        */
#define CCPLLINE        024     /* minus a line         ^X p        */
#define CCDELCH         025     /* character delete     ^D      delete */
#define CCSAVEFILE      026     /* make new file                f2  */
#define CCMILINE        027     /* plus a line          ^X n        */
#define CCMISRCH        030     /* minus search         ^B          */
#define CCLPORT         031     /* port left            ^X b        */
#define CCPUT           032     /* put                          f11 */
#define CCTABS          033     /* set tabs             ^X t        */
#define CCINSMODE       034     /* insert mode          ^X i    insert */
#define CCBACKSPACE     035     /* backspace and erase  ^H          */
#define CCCLOSE         036     /* delete               ^Y      f8  */
#define CCENTER         037     /* enter parameter      ^A      f1  */
#define CCQUIT          0177    /* terminate session    ^X ^C       */
#define CCREDRAW        0236    /* redraw screen        ^L          */
#define CCEND           0237    /* cursor to end        ^X e    end */
#define CCINTRUP        0240    /* interrupt (ttyfile) */
#define CCMAC           0200    /* macro marker */

int cursorline;         /* physical screen position of */
int cursorcol;          /*  cursor from (0,0)=ulhc of text in port */

extern char cntlmotions[];

extern int tabstops[];
char blanks[LINELM];
extern const char in0tab[]; /* input control codes */

extern int lread1;      /* Текущий входной символ, -1 - дай еще! */
char intrflag;          /* 1 - был сигнал INTERUP */

/* Умолчания */
extern int defplline,defplpage,defmiline,defmipage,deflport,defrport,
        definsert, defdelete, defpick;
extern char deffile[];

int errsw;              /* 1 - в окне параметров сообщение об ошибке */
int gosw;               /* -- атавизм */

/*
 * Глобальные параметры для param():
 * paraml - длина параметра;
 * char *paramv - сам параметр,
 * paramtype - тип параметра,
 * paramc0, paramr0, paramc1< paramr1 -
 * размеры области при "cursor defined"
 */
int paraml;
char *paramv, paramtype;
int paramc0, paramr0, paramc1, paramr1;

/*
 * cline  - массив для строки, lcline - макс. длина,
 * ncline - текущая длина, icline - следующее приращение длины
 * fcline - флаг (было изменение), clineno - номер строки в файле
 */
char *cline;
int lcline;
int ncline;
int icline;
char fcline;
int clineno;

/*
 * Описатели файлов:
 * tempfile, tempfl - дескриптор и смещение во временном файле
 * ttyfile - дескриптор файда протокола
 * inputfile - дескриптор файла ввода команд из протокола
 */
int tempfile;
off_t tempfl;
int ttyfile;
int inputfile;

char *searchkey, *symac;

int userid, groupid;

char *tmpname;                  /* name of file, for do command */

int LINEL, NLINES;              /* size of the screen */
extern char *curspos, *cvtout[];

unsigned nbytes;

int read1 (void);               /* read command from terminal */
int read2 (void);               /* read raw input character */
void getlin (int);              /* get a line from current file */
void putline (int);             /* put a line to current file */
void movecursor (int);          /* cursor movement operation */
void poscursor (int, int);      /* position a cursor in current window */
void pcursor (int, int);        /* move screen cursor */
void putup (int, int);          /* show lines of file */
void movep (int);               /* move a window to right */
void movew (int);               /* move a window down */
void excline (int);             /* extend cline array */
int putcha (int);               /* output symbol or op */
void putch (int, int);          /* put a symbol at current position */
void putstr (char *, int);      /* put a string, limited by column */
void putblanks (int);           /* output a line of spaces */
int endit (void);               /* end a session and write all */
void switchfile (void);         /* switch to alternative file */
void chgport (int);             /* switch to another window */
void search (int);              /* search a text in file */
void put (struct savebuf *, int, int); /* put a buffer into file */
int msrbuf (struct savebuf *, char *, int); /* store or get a buffer by name */
void gtfcn (int);               /* go to line by number */
int savefile (char *, int);     /* save a file */
void makeport (char *);         /* make new window */
void error (char *);            /* display error message */
char *param (int);              /* get a parameter */
int chars (int);                /* read next line from file */
int dechars (char *, int);      /* conversion of line from internal to external form */
void cleanup (void);            /* cleanup before exit */
void fatal (char *);            /* print error message and exit */
int editfile (char *, int, int, int, int); /* open a file for editing */
void splitline (int, int);      /* split a line at given position */
void combineline (int, int);    /* merge a line with next one */
int msvtag (char *name);        /* save a file position by name */
int mdeftag (char *);           /* define a file area by name */
int defkey (void);              /* redefine a keyboard key */
int addkey(int, char *);        /* add new command key to the code table */
void rescreen (void);           /* redraw a screen */
int defmac (char *);            /* define a macro sequence */
char *rmacl (int);              /* get macro sequence by name */
int mgotag (char *);            /* return a cursor back to named position */
void execr (char **);           /* run command with given parameters */
void telluser (char *, int);    /* display a message in arg area */
void doreplace(int, int, int, int*); /* replace lines via pipe */
void removeport (void);         /* remove last window */
void switchport (struct viewport *); /* switch to given window */
void drawport(struct viewport *, int); /* draw borders for a window */
void setupviewport (struct viewport *, int, int, int, int, int);
                                /* create new window */
void dumpcbuf (void);           /* flush output buffer */
char *s2i (char *, int *);      /* convert a string to number */
char *salloc (int);             /* allocate zeroed memory */
void openlines (int, int);      /* insert lines */
void closelines (int, int);     /* delete lines from file */
void picklines (int, int);      /* get lines from file to pick workspace */
void openspaces (int, int, int, int); /* insert spaces */
void closespaces (int, int, int, int); /* delete rectangular area */
void pickspaces (int, int, int, int); /* get rectangular area to pick buffer */
int interrupt (void);           /* we have been interrupted? */
int wseek (struct workspace *, int); /* set file position by line number */
int wposit (struct workspace *, int); /* set workspace position */
void redisplay (struct workspace *, int, int, int, int);
                                /* redisplay windows of the file */
void cgoto (int, int, int, int); /* scroll window to show a given area */
char *append (char *, char *);  /* append strings */
char *tgoto (char *, int, int); /* cursor addressing */
struct fsd *file2fsd (int);     /* create a file descriptor list for a file */
void puts1 (char *);            /* write a string to stdout */
void switchwksp (void);         /* switch to alternative workspace */
void checkfsd (void);           /* check fsd correctness */
int fsdwrite (struct fsd *, int, int); /* write descriptor chain to file */
void printfsd (struct fsd *);   /* debug output of fsd chains */
int checkpriv (int);            /* check access rights */
int getpriv (int);              /* get file access modes */
void tcread (void);             /* load termcap descriptions */
char *tgetstr(char *, char *, char **); /* get string option from termcap */
int tgetnum (char *, char *);   /* get a numeric termcap option */
int tgetflag (char *, char *);  /* get a flag termcap option */
char *getnm (int);              /* get user id as printable text */
void ttstartup (void);          /* setup terminal modes */
void ttcleanup (void);          /* restore terminal modes */
int get1w (int);                /* read word */
int get1c (int);                /* read byte */
void put1w (int, int);          /* write word */
void put1c (char, int);         /* write byte */
void mainloop (void);           /* main editor loop */

/*
 * Таблица клавиатуры (команда / строка символов)
 */
struct ex_int {
    int incc;
    char *excc;
};
extern struct ex_int inctab[];
