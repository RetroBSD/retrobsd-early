; Small C 8080;
;	Coder (2.4,84/11/27)
;	Front End (2.7,84/11/28)
	extrn ?gchar,?gint,?pchar,?pint,?bool
	extrn ?sxt
	extrn ?or,?and,?xor
	extrn ?eq,?ne,?gt,?le,?ge,?lt,?uge,?ult,?ugt,?ule
	extrn ?asr,?asl
	extrn ?sub,?neg,?com,?lneg,?mul,?div
	extrn ?case
	cseg
;#include <stdio.h>
;          ^
;******  Could not open include file  ******
;FILE *input, *input2, *output;
;     ^
;******  missing open paren  ******
FILE:
;FILE *input, *input2, *output;
;     ^
;******  illegal argument name  ******
;FILE *input, *input2, *output;
;             ^
;******  illegal argument name  ******
;FILE *input, *input2, *output;
;                      ^
;******  illegal argument name  ******
;FILE *input, *input2, *output;
;                             ^
;******  expected comma  ******
;FILE *input, *input2, *output;
;                             ^
;******  function requires compound statement  ******
?1:
	ret
;FILE *inclstk[3];
;    ^
;******  already defined  ******
;FILE
;FILE *inclstk[3];
;     ^
;******  missing open paren  ******
FILE:
;FILE *inclstk[3];
;     ^
;******  illegal argument name  ******
;FILE *inclstk[3];
;             ^
;******  expected comma  ******
;FILE *inclstk[3];
;             ^
;******  illegal argument name  ******
;FILE *inclstk[3];
;               ^
;******  expected comma  ******
;FILE *inclstk[3];
;               ^
;******  illegal argument name  ******
;FILE *inclstk[3];
;                ^
;******  expected comma  ******
;FILE *inclstk[3];
;                ^
;******  function requires compound statement  ******
?2:
	ret
	dseg
	extrn	etext
	extrn	edata
	public	symtab
symtab:	ds	2800
	public	glbptr
glbptr:	ds	2
	public	rglbptr
rglbptr:	ds	2
	public	locptr
locptr:	ds	2
	public	ws
ws:	ds	200
	public	wsptr
wsptr:	ds	2
	public	swstcase
swstcase:	ds	200
	public	swstlab
swstlab:	ds	200
	public	swstp
swstp:	ds	2
	public	litq
litq:	ds	2000
	public	litptr
litptr:	ds	2
	public	macq
macq:	ds	1000
	public	macptr
macptr:	ds	2
	public	line
line:	ds	150
	public	mline
mline:	ds	150
	public	lptr
lptr:	ds	2
	public	mptr
mptr:	ds	2
	public	nxtlab
nxtlab:	ds	2
	public	litlab
litlab:	ds	2
	public	stkp
stkp:	ds	2
	public	argstk
argstk:	ds	2
	public	ncmp
ncmp:	ds	2
	public	errcnt
errcnt:	ds	2
	public	glbflag
glbflag:	ds	2
	public	ctext
ctext:	ds	2
	public	cmode
cmode:	ds	2
	public	lastst
lastst:	ds	2
	public	FILE
	public	inclsp
inclsp:	ds	2
	public	fname
fname:	ds	20
	public	quote
quote:	ds	2
	public	cptr
cptr:	ds	2
	public	iptr
iptr:	ds	2
	public	fexitlab
fexitlab:	ds	2
	public	iflevel
iflevel:	ds	2
	public	skipleve
skipleve:	ds	2
	public	errfile
errfile:	ds	2
	public	sflag
sflag:	ds	2
	public	cflag
cflag:	ds	2
	public	errs
errs:	ds	2
	public	aflag
aflag:	ds	2

;16 error(s) in compilation
;	literal pool:0
;	global pool:602
;	Macro pool:851
	end
