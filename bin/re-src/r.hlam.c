/*
 * Редактор RED. ИАЭ им. И.В. Курчатова, ОС ДЕМОС
 *
 * Разные программы / всякий хлам
 *
 * $Header: /home/sergev/Project/vak-opensource/trunk/relcom/nred/RCS/r.hlam.c,v 3.1 1986/04/20 23:41:51 alex Exp $
 * $Log: r.hlam.c,v $
 * Revision 3.1  1986/04/20 23:41:51  alex
 */
#include "r.defs.h"
#include <sys/types.h>
#include <sys/stat.h>

/*
 * Allocate zeroed memory.
 * Always succeeds.
 */
char *salloc(n)
    int n;
{
    register char *c;

    c = calloc(n, 1);
    if (! c)
        fatal(NULL);
    return (c);
}

/*
 * checkpriv(file) - Проверить права. 0 - ни чтения, ни записи;
 *                   1 только чтение
 *                   2 и чтение, и запись
 */
checkpriv(fildes)
int fildes;
{
    register struct stat *buf;
    struct stat buffer;
    int anum, num;
    register int unum,gnum;
    if (userid == 0) return 2;   /* superuser accesses all */
    buf = &buffer;
    fstat(fildes,buf);
    unum = gnum = anum = 0;
    if (buf->st_uid == userid)
    {
        if (buf->st_mode & 0200) unum = 2;
        else if (buf->st_mode & 0400) unum = 1;
    }
    if (buf->st_gid == groupid)
    {
        if (buf->st_mode & 020) gnum = 2;
        else if (buf->st_mode & 040) gnum = 1;
    }
    if (buf->st_mode & 02) anum = 2;
    else if (buf->st_mode & 04) anum = 1;
    num = (unum > gnum ? unum : gnum);
    num = (num  > anum ? num  : anum);
    return(num);
}

/*
 * getpriv(fildes)
 * Дай режим
 */
getpriv(fildes)
int fildes;
{
        struct stat buffer;
        register struct stat *buf;
        buf = &buffer;
        fstat(fildes,buf);
        return (buf->st_mode & 0777);
}

/* printf1 - printf для рабочего режима
 */
printf1(s) char *s;
{
        register int i=0;
        register char *s1=s;
        while(*s1++) i++;
        if(i) write(1,s,i);
}

/* strcopy(a,b)
 * Копировать строку b в строку a
 */
strcopy(a,b)
char *a,*b;
{
        while(*a++ = *b++);
}

/* abs(n) -
 * Функция abs
 */
abs(number)
int number;
{
        return (number<0 ? -number : number);
}

/*
 * Append strings.
 * Allocates memory.
 */
char *append(name, ext)
    char *name, *ext;
{
    int lname;
    register char *c, *d, *newname;

    lname = 0;
    c = name;
    while (*c++)
        ++lname;
    c = ext;
    while (*c++)
        ++lname;
    newname = c = salloc(lname + 1);
    d = name;
    while (*d)
        *c++ = *d++;
    d = ext;
    while (*c++ = *d++);
    return newname;
}

/*
 * Convert a string to number.  Store result into *i.
 * Return pointer to first symbol after a number, or 0 if none.
 */
char *s2i(s, i)
    char *s;
    int *i;
{
    register char lc, c;
    register int val;
    int sign;
    char *ans;

    sign = 1;
    val = lc = 0;
    ans = 0;
    while ((c = *s++) != 0) {
        if (c >= '0' && c <= '9') val = 10*val + c - '0';
        else if (c == '-' && lc == 0) sign = -1;
        else {
            ans = --s;
            break;
        }
        lc = c;
    }
    *i = val * sign;
    return ans;
}

/*
 * getnm() -    Подпрограмма выдает номер пользователя в текстовом виде.
 */
#define LNAME 8
static char namewd[LNAME+1];
char *getnm(uid)
int uid;
{
        register int i,j;
        i = LNAME;
        namewd[LNAME]=0;
        while( i>1 && uid>0) {
                namewd[--i]= '0' + uid %10;
                uid /= 10;
        }
        return(&namewd[i]);
}

/*
 * get1c, get1w(fd) - дай байт/слово
 * put1c, put1w(w,fd) - положи байт/слово
 */
int get1w(fd)
int fd;
{
        int i;

        if (read(fd, &i, sizeof(int)) !=  sizeof(int))
            return -1;
        return i;
}

int get1c(fd)
int fd;
{
        char c;
        if(read(fd,&c,1)==1) return(c&0377);
        return(-1);
}

put1w(w,fd)
int fd,w;
{
        write(fd, &w, sizeof(int));
}

put1c(c,fd)
int fd;
char c;
{
        write(fd,&c,1);
}
