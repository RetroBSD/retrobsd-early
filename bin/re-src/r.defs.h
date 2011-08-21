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

#ifndef DEBUG
#   define printf printf1
#   define DEBUGCHECK /* */
#else
#   define DEBUGCHECK checkfsd()   /* check fsd consistency for debugging */
#endif

#define EDITED      2       /* Значение openwrite, если файл редактировался */

#define esc0        COCURS
#define esc1        '\\'

#define CONTROLCHAR (lread1 < 040)
#define CTRLCHAR    (((lread1>=0)&&(lread1<040)) || ((lread1 >= 0177)&& (lread1<=0240)))
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

#define VMOTCODE    4       /* коды 1 - 4 уводят курсор из текущей строки */
#define UP          1       /* Up     */
#define DN          2       /* Down   */
#define RN          3       /* Return */
#define HO          4       /* Home   */
#define RT          5       /* Right  */
#define LT          6       /* Left   */
#define TB          7       /* Tab    */
#define BT          8       /* Backtab*/

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
        int fsdbytes;  /* Переменное число байт - столько, сколько нужно
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
#define CCBACKTAB          BT      /* tab left             */
#define CCHOME             HO      /* home cursor          */
#define CCMOVEDOWN         DN      /* move down 1 line     */
#define CCMOVELEFT         LT      /* backspace            */
#define CCMOVERIGHT        RT      /* forward move         */
#define CCMOVEUP           UP      /* move up 1 lin        */
#define CCRETURN           RN      /* return               */
#define CCTAB              TB      /* tab                  */

#define CCCTRLQUOTE        000     /* knockdown next char  */
#define CCPICK             011     /* pick                 */
#define CCMAKEPORT         012     /* make a viewport      */
#define CCOPEN             013     /* insert               */
#define CCSETFILE          014     /* set file             */
#define CCCHPORT           015     /* change port          */
#define CCPLPAGE           016     /* minus a page         */
#define CCGOTO             017     /* goto linenumber      */
#define CCDOCMD            020     /* execute a filter     */
#define CCMIPAGE           021     /* plus a page          */
#define CCPLSRCH           022     /* plus search          */
#define CCRPORT            023     /* port right           */
#define CCPLLINE           024     /* minus a line         */
#define CCDELCH            025     /* character delete     */
#define CCSAVEFILE         026     /* make new file        */
#define CCMILINE           027     /* plus a line          */
#define CCMISRCH           030     /* minus search         */
#define CCLPORT            031     /* port left            */
#define CCPUT              032     /* put                  */
#define CCTABS             033     /* set tabs             */
#define CCINSMODE          034     /* insert mode          */
#define CCBACKSPACE        035     /* backspace and erase  */
#define CCCLOSE            036     /* delete               */
#define CCENTER            037     /* enter parameter      */
#define CCQUIT            0177     /* terminate editor run */
#define CCINTRUP          0240     /* interuption (for ttyfile)     */
#define CCMAC             0200     /* macro marka                   */

int cursorline;         /* physical screen position of */
int cursorcol;          /*  cursor from (0,0)=ulhc of text in port */

extern char cntlmotions[];

extern int tabstops[];
char blanks[LINELM];

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
char *(*agoto)();               /* for termcap definitions */

unsigned nbytes;

int read1 (void);               /* read a symbol from terminal */
void getlin (int);              /* get a line from current file */
void putline (int);             /* put a line to current file */
void movecursor (int);          /* cursor movement operation */
void poscursor (int, int);      /* position a cursor in current window */
void putup (int, int);          /* show lines of file */
void movep (int);               /* move a window to right */
void movew (int);               /* move a window down */
void excline (int);             /* extend cline array */
int putcha (int);               /* output symbol or op */
void putch (int, int);          /* put a symbol at current position */
void putstr (char *, int);       /* put a string, limited by column */
int endit (void);               /* end a session and write all */
void switchfile (void);         /* switch to alternative file */
void chgport (int);             /* switch to another window */
void search (int);              /* search a text in file */
void put (struct savebuf *, int, int); /* put a buffer into file */
void gtfcn (int);               /* go to line by number */
int savefile (char *, int);     /* save a file */
void makeport (char *);         /* make new window */
void error (char *);            /* display error message */
char *param (int);              /* get a parameter */
int dechars (char *, int);      /* conversion of line from internal to external form */
void cleanup (void);            /* cleanup before exit */
void fatal (char *);            /* print error message and exit */
int editfile (char *, int, int, int, int); /* open a file for editing */
void splitline (int, int);      /* split a line at given position */
int msrbuf (struct savebuf *, char *, int); /* store or get a buffer by name */
void combineline (int, int);    /* merge a line with next one */
int msvtag (char *name);        /* save a file position by name */
int mdeftag (char *);           /* define a file area by name */
int defkey (void);              /* redefine a keyboard key */
void rescreen (void);           /* redraw a screen */
int defmac (char *);            /* define a macro sequence */
int mgotag (char *);            /* return a cursor back to named position */
void execr (char **);           /* run command with given parameters */
void telluser (char *, int);    /* display a message in arg area */
void doreplace(int, int, int, int*); /* replace lines via pipe */
void removeport (void);         /* remove last window */
void switchport (struct viewport *); /* switch to given window */
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
int wposit (struct workspace *, int); /* Set workspace position */
void cgoto (int, int, int, int); /* scroll window to show a given area */
char *append (char *, char *);  /* append strings */
char *tgoto (char *, int, int); /* cursor addressing */
struct fsd *file2fsd (int);     /* create a file descriptor list for a file */

/*
 * Таблица клавиатуры (команда / строка символов)
 */
struct ex_int {
    int incc;
    char *excc;
};
extern struct ex_int inctab[];
