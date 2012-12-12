/*
 * Global definitions.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef DEBUG
#include <stdio.h>
#endif

/*
 * DEBUGCHECK: check consistency of segment chains.
 */
#ifdef DEBUG
#   define DEBUGCHECK checksegm()
#else
#   define DEBUGCHECK /* */
#endif

#define MOVECMD(x)  ((x) >= CCMOVEUP && (x) <= CCBACKTAB)
#define CTRLCHAR(x) (((x) >= 0 && (x) < ' ') || ((x) >= 0177 && (x) < 0240))
#define MAXCOLS     128     /* max. width of screen */
#define MAXLINES    48      /* max. height of screen */
#define LBUFFER     256     /* lower limit for the current line buffer */
#define PARAMREDIT  40      /* input field of paramport */
#define PARAMRINFO  78      /* last column of paramport */
#define NPARAMLINES 1       /* number of lines in paramport */
#define FILEMODE    0664    /* access mode for newly created files */
#define MAXFILES    14      /* max. files under edit */
#define MAXPORTLIST 10      /* max.windows */
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
#define COMCOD      11      /* Number of output codes */

/* margin characters */
#define LMCH        '|'
#define RMCH        '|'
#define TMCH        '-'
#define BMCH        '-'
#define MRMCH       '>'
#define MLMCH       '<'
#define ELMCH       ';'
#define DOTCH       '+'

/*
 * Descriptor of file segment.  Contains from 1 to 127 lines of file,
 * written sequentially.  An elementary component of descriptor chain.
 */
#define FSDMAXL     127     /* Max. number of lines in descriptor */

typedef struct segment {
    struct segment *prev;   /* Previous descriptor in chain */
    struct segment *next;   /* Next descriptor in chain */
    int nlines;             /* Count of lines in descriptor,
                             * or 0 when end of chain */
    int fdesc;              /* File descriptor, or 0 for end of chain */
    off_t seek;             /* Byte offset in the file */
    char data;              /* Varying amount of data, needed
                             * to store the next nlines-1 lines. */
    /*
     * Interpretation of next byte:
     * 1-127   offset of this line from a previous line
     * 0       empty line
     * -n      line starts at offset n*128+next bytes
     *         from beginning of previous line.
     * There are at least nlines-1 bytes allocated or more,
     * in case of very long lines.
     */
} segment_t;

#define sizeof_segment_t    offsetof (segment_t, data)

typedef struct {
    segment_t *chain;       /* Chain of segments */
    char *name;             /* File name */
    char writable;          /* Do we have a write permission */
#define EDITED  2           /* Value when file was modified */

    char backup_done;       /* Already copied to .bak */
    int nlines;             /* Number of non-empty lines in file */
} file_t;

file_t file[MAXFILES];     /* Table of files */
int curfile;

/*
 * workspace - структура, которая описывает файл
 */
typedef struct {
    segment_t *cursegm;     /* ptr на текущий segm */
    int ulhclno;            /* номер строки, верхней на экране */
    int ulhccno;            /* номер колонки, которая 0 на экране */
    int curlno;             /* текущий номер строки */
    int curflno;            /* номер строки, первой в cursegm */
    int wfile;              /* номер файла, 0 - если нет вообще */
    int ccol;               /* curorcol, когда не активен */
    int crow;               /* cursorline, когда неактивен */
} workspace_t;

workspace_t *curwksp, *pickwksp;

/*
 * viewport - описатель окна на экране терминала
 * все координаты на экране, а также ltext и ttext, измеряются по отношению
 *      к (0,0) = начало экрана. Остальные 6 границ приводятся по отношению
 *      к ltext и ttext.
 */
typedef struct {
    workspace_t *wksp;      /* Ссылка на workspace         */
    workspace_t *altwksp;   /* Альтернативное workspace */
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
} viewport_t;

viewport_t *portlist[MAXPORTLIST];
int nportlist;

viewport_t *curport;        /* текущее окно */
viewport_t wholescreen;     /* весь экран   */
viewport_t paramport;       /* окно для параметров */

/*
 * Copy-and-paste clipboard.
 */
typedef struct {
    int linenum;            /* Номер первой строки в "#" */
    int nrows;              /* Число строк               */
    int ncolumns;           /* Число колонок             */
} clipboard_t;

clipboard_t *pickbuf, *deletebuf;

/*
 * Управляющие символы
 */
#define CCCTRLQUOTE 0       /* knockdown next char  ^P              */
#define CCMOVEUP    1       /* move up 1 line               up      */
#define CCMOVEDOWN  2       /* move down 1 line             down    */
#define CCRETURN    3       /* return               ^M              */
#define CCHOME      4       /* home cursor          ^X h    home    */
#define CCMOVERIGHT 5       /* move right                   right   */
#define CCMOVELEFT  6       /* move left                    left    */
#define CCTAB       7       /* tab                  ^I              */
#define CCBACKTAB   010     /* tab left             ^X t            */
#define CCPICK      011     /* pick                         f6      */
#define CCMAKEPORT  012     /* make a viewport              f4      */
#define CCOPEN      013     /* insert                       f7      */
#define CCSETFILE   014     /* set file                     f5      */
#define CCCHPORT    015     /* change port                  f3      */
#define CCPLPAGE    016     /* minus a page                 page down */
#define CCGOTO      017     /* goto linenumber              f10     */
#define CCDOCMD     020     /* execute a filter     ^X x            */
#define CCMIPAGE    021     /* plus a page                  page up */
#define CCPLSRCH    022     /* plus search          ^F              */
#define CCRPORT     023     /* port right           ^X f            */
#define CCPLLINE    024     /* minus a line         ^X p            */
#define CCDELCH     025     /* character delete     ^D      delete  */
#define CCSAVEFILE  026     /* make new file                f2      */
#define CCMILINE    027     /* plus a line          ^X n            */
#define CCMISRCH    030     /* minus search         ^B              */
#define CCLPORT     031     /* port left            ^X b            */
#define CCPUT       032     /* put                          f11     */
#define CCTABS      033     /* set tabs             ^X t            */
#define CCINSMODE   034     /* insert mode          ^X i    insert  */
#define CCBACKSPACE 035     /* backspace and erase  ^H              */
#define CCCLOSE     036     /* delete               ^Y      f8      */
#define CCENTER     037     /* enter parameter      ^A      f1      */
#define CCQUIT      0177    /* terminate session    ^X ^C           */
#define CCREDRAW    0236    /* redraw screen        ^L              */
#define CCEND       0237    /* cursor to end        ^X e    end     */
#define CCINTRUP    0240    /* interrupt (ttyfile) */
#define CCMAC       0200    /* macro marker */

int cursorline;             /* physical screen position of */
int cursorcol;              /* cursor from (0,0)=ulhc of text in port */

extern char cntlmotions[];

extern int tabstops[];
char blanks[MAXCOLS];
extern const char in0tab[]; /* input control codes */

extern int keysym;          /* Текущий входной символ, -1 - дай еще! */
char intrflag;              /* 1 - был сигнал INTR */
int highlight_position;     /* Highlight the current cursor position */

/* Умолчания */
extern int defplline,defplpage,defmiline,defmipage,deflport,defrport,
        definsert, defdelete, defpick;
extern char deffile[];

int message_displayed;      /* Arg area contains an error message */

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
 * Current line.
 */
char *cline;                /* массив для строки */
int cline_max;              /* макс. длина */
int cline_len;              /* текущая длина */
int cline_incr;             /* следующее приращение длины */
char cline_modified;        /* флаг 'было изменение' */
int clineno;                /* номер строки в файле */

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

int NCOLS, NLINES;              /* size of the screen */
extern char *curspos, *cvtout[];

/*
 * Таблица клавиатуры (команда / строка символов)
 */
typedef struct {
    int incc;
    char *excc;
} keycode_t;

extern keycode_t keytab[];

int getkeysym (void);           /* read command from terminal */
int rawinput (void);            /* read raw input character */
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
void put (clipboard_t *, int, int); /* put a buffer into file */
int msrbuf (clipboard_t *, char *, int); /* store or get a buffer by name */
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
void switchport (viewport_t *); /* switch to given window */
void drawport(viewport_t *, int); /* draw borders for a window */
void setupviewport (viewport_t *, int, int, int, int, int);
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
int wseek (workspace_t *, int); /* set file position by line number */
int wposit (workspace_t *, int); /* set workspace position */
void redisplay (workspace_t *, int, int, int, int);
                                /* redisplay windows of the file */
void cgoto (int, int, int, int); /* scroll window to show a given area */
char *append (char *, char *);  /* append strings */
char *tgoto (char *, int, int); /* cursor addressing */
segment_t *file2segm (int);     /* create a file descriptor list for a file */
void puts1 (char *);            /* write a string to stdout */
void switchwksp (void);         /* switch to alternative workspace */
void checksegm (void);          /* check segm correctness */
int segmwrite (segment_t *, int, int); /* write descriptor chain to file */
void printsegm (segment_t *);   /* debug output of segm chains */
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
void put1c (int, int);          /* write byte */
void mainloop (void);           /* main editor loop */
