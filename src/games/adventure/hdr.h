/*
 * ADVENTURE -- Jim Gillogly, Jul 1977
 *
 * This program is a re-write of ADVENT, written in FORTRAN mostly by
 * Don Woods of SAIL.  In most places it is as nearly identical to the
 * original as possible given the language and word-size differences.
 * A few places, such as the message arrays and travel arrays were changed
 * to reflect the smaller core size and word size.  The labels of the
 * original are reflected in this version, so that the comments of the
 * fortran are still applicable here.
 *
 * The data file distributed with the fortran source is assumed to be called
 * "glorkz" in the directory where the program is first run.
 */

/* hdr.h: included by c advent files */

extern int setup;                       /* changed by savec & init      */
int datfd;                              /* message file descriptor      */
int delhit;
int yea;

#define TAB     011
#define LF      012
#define FLUSHLINE while (getchar()!='\n')
#define FLUSHLF   while (next()!=LF)

int loc,newloc,oldloc,oldlc2,wzdark,SHORT,gaveup,kq,k,k2;
char *wd1,*wd2;                         /* the complete words           */
int verb,obj,spk;
extern int blklin;
int saved,savet,mxscor,latncy;

#define MAXSTR  20                      /* max length of user's words   */

#define HTSIZE  512                     /* max number of vocab words    */
struct hashtab                          /* hash table for vocabulary    */
{       int val;                        /* word type &index (ktab)      */
	char *atab;                     /* pointer to actual string     */
} voc[HTSIZE];

#define DATFILE "glorkz"                /* all the original msgs        */
#define TMPFILE "tmp.foo.baz"           /* just the text msgs           */


struct text
{       int seekadr;                    /* DATFILE must be < 2**16      */
	int txtlen;                     /* length of msg starting here  */
};

#define RTXSIZ  205
struct text rtext[RTXSIZ];              /* random text messages         */

#define MAGSIZ  35
struct text mtext[MAGSIZ];              /* magic messages               */

int clsses;
#define CLSMAX  12
struct text ctext[CLSMAX];              /* classes of adventurer        */
int cval[CLSMAX];

struct text ptext[101];                 /* object descriptions          */

#define LOCSIZ  141                     /* number of locations          */
struct text ltext[LOCSIZ];              /* long loc description         */
struct text stext[LOCSIZ];              /* short loc descriptions       */

struct travlist                         /* direcs & conditions of travel*/
{       struct travlist *next;          /* ptr to next list entry       */
	int conditions;                 /* m in writeup (newloc / 1000) */
	int tloc;                       /* n in writeup (newloc % 1000) */
	int tverb;                      /* the verb that takes you there*/
} *travel[LOCSIZ],*tkk;                 /* travel is closer to keys(...)*/

int atloc[LOCSIZ];

int  plac[101];                         /* initial object placement     */
int  fixd[101],fixed[101];              /* location fixed?              */

int actspk[35];                         /* rtext msg for verb <n>       */

int cond[LOCSIZ];                       /* various condition bits       */

extern int setbit[16];                  /* bit defn masks 1,2,4,...     */

int hntmax;
int hints[20][5];                       /* info on hints                */
int hinted[20],hintlc[20];

int place[101], prop[101], plink[201];
int abb[LOCSIZ];

int maxtrs,tally,tally2;                /* treasure values              */

#define FALSE   0
#define TRUE    1

int keys,lamp,grate,cage,rod,rod2,steps,/* mnemonics                    */
	bird,door,pillow,snake,fissur,tablet,clam,oyster,magzin,
	dwarf,knife,food,bottle,water,oil,plant,plant2,axe,mirror,dragon,
	chasm,troll,troll2,bear,messag,vend,batter,
	nugget,coins,chest,eggs,tridnt,vase,emrald,pyram,pearl,rug,chain,
	spices,
	back,look,cave,null,entrnc,dprssn,
	say,lock,throw,find,invent;

int chloc,chloc2,dseen[7],dloc[7],      /* dwarf stuff                  */
	odloc[7],dflag,daltlc;

int tk[21],stick,dtotal,attack;
int turns,lmwarn,iwest,knfloc,detail,   /* various flags & counters     */
	abbnum,maxdie,numdie,holdng,dkill,foobar,bonus,clock1,clock2,
	saved,closng,panic,closed,scorng;

int demo,newloc,limit;

void init (char *);
void startup (void);
void trapdel (int);
void rdata (void);
void mspeak (int);
void rspeak (int);
void speak (struct text *);
int toting (int);
int yes (int, int, int);
int yesm (int, int, int);
void drop (int, int);
int vocab (char *, int, int);
void poof (void);
int confirm (char *);
int save (char *, char *);
int start (int);
int length (char *);
void bug (int);
int getcmd (char *);
int fdwarf (void);
int die (int);
int forced (int);
int dark (int);
int pct (int);
void pspeak (int, int);
void checkhints (void);
void getin (char **, char **);
void copystr (char *, char *);
void done (int);
int closing (void);
int caveclose (void);
int here (int);
int at (int);
int liqloc (int);
int weq (char *, char *);
int march (void);
void dstroy (int);
int score (void);
void move (int, int);
void datime (int *, int *);
void ciao (char *);
int trtake (void);
int trdrop (void);
int trsay (void);
int tropen (void);
int trkill (void);
int trtoss (void);
int trfill (void);
int trfeed (void);
int liq (int);
int ran (int);
void carry (int, int);
void juggle (int);
int put (int, int, int);
