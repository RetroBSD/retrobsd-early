#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#   define assert(x) { if (! (x)) {\
	fprintf (stderr, "assertion failed: file \"%s\", line %d\n",\
	__FILE__, __LINE__); exit (-1); } }
#else
#   define assert(x)
#endif
#define INLINE static inline __attribute__((always_inline))

/*
 * Type tags.
 */
#define TPAIR           1               /* tag "pair" */
#define TSYMBOL         2               /* tag "symbol" */
#define TBOOL           3               /* tag "boolean" */
#define TCHAR           4               /* tag "character" */
#define TINTEGER        5               /* tag "integer" */
#define TREAL           6               /* tag "real" */
#define TSTRING         7               /* tag "string" */
#define TVECTOR         8               /* tag "vector" */
#define THARDW          9               /* tag "hard-wired function" */
#define TCLOSURE       10               /* tag "closure" */

#define NIL             ((lisp_t) -1)     /* empty list */
#define TOPLEVEL        ((lisp_t) -2)     /* top level context */

/*
 * Size of lisp_t should be bigger that pointer size.
 * Это используется для хранения указателя на строку в cdr.
 */
typedef size_t lisp_t;                  /* address of a cell */
typedef lisp_t (*func_t) (lisp_t, lisp_t);  /* pointer to hardwired function */

typedef struct {                        /* elementary cell */
	short type;                     /* type */
	union {
		struct {                /* pair */
			lisp_t a;       /* first element */
			lisp_t d;       /* second element */
		} pair;
		struct {                /* string */
			int length;     /* length */
			char *array;    /* array of bytes */
		} string;
		struct {                /* vector */
			int length;     /* length */
			lisp_t *array;  /* array of elements */
		} vector;
		char *symbol;           /* symbol */
		unsigned char chr;      /* character */
		long integer;           /* integer number, hardwired function */
		double real;            /* real number */
	} as;
} cell;

typedef struct {
	char *name;
	func_t func;
} functab;

extern lisp_t T, ZERO, ENV;

int eqv (lisp_t a, lisp_t b);                   /* сравнение объектов */
int equal (lisp_t a, lisp_t b);                 /* рекурсивное сравнение */
lisp_t evalblock (lisp_t expr, lisp_t ctx);     /* вычисление блока */
lisp_t evalclosure (lisp_t func, lisp_t expr);  /* вычисление замыкания */
lisp_t evalfunc (lisp_t func, lisp_t arg, lisp_t ctx); /* вычисление функции */
lisp_t eval (lisp_t expr, lisp_t *ctxp);        /* вычисление */
lisp_t getexpr ();                              /* чтение выражения */
void putexpr (lisp_t p, FILE *fd);              /* печать списка */
lisp_t copy (lisp_t a, lisp_t *t);              /* копирование списка */
lisp_t alloc (int type);                        /* выделение памяти под новую пару */
void fatal (char*);                             /* фатальная ошибка */
int istype (lisp_t p, int type);                /* проверить соответствие типа */

extern cell mem[];                              /* память списков */
extern unsigned memsz;                          /* размер памяти */
extern void *memcopy (void*, int);

INLINE lisp_t car (lisp_t p)            /* доступ к первому элементу */
{
	assert (p>=0 && p<memsz && mem[p].type==TPAIR);
	return mem[p].as.pair.a;
}

INLINE lisp_t cdr (lisp_t p)            /* доступ ко второму элементу */
{
	assert (p>=0 && p<memsz && mem[p].type==TPAIR);
	return mem[p].as.pair.d;
}

INLINE void setcar (lisp_t p, lisp_t v) /* доступ к первому элементу */
{
	assert (p>=0 && p<memsz && mem[p].type==TPAIR);
	mem[p].as.pair.a = v;
}

INLINE void setcdr (lisp_t p, lisp_t v) /* доступ ко второму элементу */
{
	assert (p>=0 && p<memsz && mem[p].type==TPAIR);
	mem[p].as.pair.d = v;
}

INLINE lisp_t cons (lisp_t a, lisp_t d) /* выделение памяти под новую пару */
{
	lisp_t p = alloc (TPAIR);
	setcar (p, a);
	setcdr (p, d);
	return p;
}

INLINE lisp_t symbol (char *name)       /* создание атома-символа */
{
	lisp_t p = alloc (TSYMBOL);
	mem[p].as.symbol = memcopy (name, strlen (name) + 1);
	return p;
}

INLINE long numval (lisp_t p)           /* выдать значение целого числа */
{
	assert (p>=0 && p<memsz && mem[p].type==TINTEGER);
	return mem[p].as.integer;
}

INLINE lisp_t number (long val)         /* создание атома-числа */
{
	lisp_t p = alloc (TINTEGER);
	mem[p].as.integer = val;
	return p;
}

INLINE lisp_t string (int len, char *array) /* создание строки */
{
	lisp_t p = alloc (TSTRING);
	mem[p].as.string.length = len;
	if (len > 0)
		mem[p].as.string.array = memcopy (array, len);
	return p;
}

INLINE long charval (lisp_t p)          /* выдать значение буквы */
{
	assert (p>=0 && p<memsz && mem[p].type==TCHAR);
	return mem[p].as.chr;
}

INLINE lisp_t character (int val)       /* создание атома-буквы */
{
	lisp_t p = alloc (TCHAR);
	mem[p].as.chr = val;
	return p;
}

INLINE lisp_t real (double val)         /* создание атома-вещественного числа */
{
	lisp_t p = alloc (TREAL);
	mem[p].as.real = val;
	return p;
}

INLINE lisp_t vector (int len, lisp_t *array) /* создание вектора */
{
	lisp_t p = alloc (TVECTOR);
	mem[p].as.vector.length = len;
	if (len > 0)
		mem[p].as.vector.array = memcopy (array, len * sizeof (lisp_t));
	return p;
}

INLINE lisp_t closure (lisp_t body, lisp_t ctx) /* создание замыкания */
{
	lisp_t p = alloc (TCLOSURE);
	mem[p].as.pair.a = body;
	mem[p].as.pair.d = ctx;
	return p;
}

INLINE lisp_t hardw (func_t func)       /* создание встроенной функции */
{
	lisp_t p = alloc (THARDW);
	mem[p].as.integer = (long) func;
	return p;
}

INLINE double realval (lisp_t p)        /* выдать значение веществ. числа */
{
	assert (p>=0 && p<memsz && mem[p].type==TREAL);
	return mem[p].as.real;
}

INLINE func_t hardwval (lisp_t p)       /* выдать адрес встроенной функции */
{
	assert (p>=0 && p<memsz && mem[p].type==THARDW);
	return (func_t) mem[p].as.integer;
}

INLINE lisp_t closurebody (lisp_t p)    /* выдать замыкание */
{
	assert (p>=0 && p<memsz && mem[p].type==TCLOSURE);
	return mem[p].as.pair.a;
}

INLINE lisp_t closurectx (lisp_t p)     /* выдать замыкание */
{
	assert (p>=0 && p<memsz && mem[p].type==TCLOSURE);
	return mem[p].as.pair.d;
}

INLINE char *symname (lisp_t p)         /* выдать строку - имя символа */
{
	assert (p>=0 && p<memsz && mem[p].type==TSYMBOL);
	return mem[p].as.symbol;
}
