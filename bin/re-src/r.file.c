/*
 * Редактор RED. ИАЭ им. И.В. Курчатова, ОС ДЕМОС
 * Работа с файлами.
 */
#include "r.defs.h"

/*
 * Write file from channel n to file.
 * When no file given use the name from openfnames[n].
 * When no permission to write, ask to write to ".".
 */
int savefile(file, n)
    char *file;
    int n;
{
    register char *f1;
    char *f0, *f2;
    register int i, j;
    int newf, nowrbak = 0;

    /* дай справочник */
    if (file) {
        f0 = file;
    } else {
        f0 = openfnames[n];
        nowrbak = 1;
    }
    for (f1=f2=f0; *f1; f1++)
        if (*f1 == '/')
            f2 = f1;
    if (f2 > f0) {
        *f2 = '\0';
        i = open(f0, 0);
        *f2 = '/';
    } else
        i = open (".", 0);

    if (i < 0) {
        error ("Directory does not exist.");
        return(0);
    }
    j = checkpriv(i);
    close (i);
    if (j != 2) {
        if (file) {
            error ("Can't write in specified directory");
            return(0);
        }
        if (f2 > f0) {
            telluser("Hit <save> to use '.'",0);
            lread1 = -1;
            nowrbak = 0;
            read1();
            if (lread1 != CCSAVEFILE)
                return(-1);
            i = open(".", 0);
            if (i < 0) {
                error ("Directory '.' does not exist!");
                return(0);
            }
            j = checkpriv(i);
            close (i);
            if (j != 2) {
                error ("Can't write in '.'");
                return(0);
            }
            f0 = f2 + 1; /* points to file name */
        } else {
            error ("Can't write in '.'");
            return(0);
        }
    }
    /* Готовимся к записи файла f0 */
    f1 = append (f0, "~");
    if (nowrbak && ! movebak[n]) {
        nowrbak = 0;
        movebak[n] = 1;
    }
    if (! nowrbak) {
        unlink(f1);
        link (f0, f1);
    }
    unlink (f0);
    newf = creat(f0, getpriv(n));
    if (newf < 0) {
        error ("Creat failed!");
        return(0);
    }
    /* chown(f0, userid); */

    /* Собственно запись. */
    telluser("save: ", 0);
    telluser(f0, 6);
    return (fsdwrite(openfsds[n], 077777, newf) == -1 ? 0 : 1);
}

/*
 * Запись по цепочке описателей f в файл "newf"
 * Если nl # 0  - записывать только nl строк или
 * -nl абзацев текста (nl используется в случае
 * команды "exec".
 * Ответ - число записанных строк, или -1, если ошибка.
 */
int fsdwrite(ff, nl, newf)
    struct fsd *ff;
    int nl, newf;
{
    register struct fsd *f;
    register int *c, i;
    int j, k, bflag, tlines;

    if (lcline < LBUFFER)
        excline(LBUFFER);
    f = ff;
    bflag = 1;
    tlines = 0;
    while (f->fsdfile && nl) {
        if (f->fsdfile > 0) {
            i = 0;
            c = &f->fsdbytes;
            for (j=f->fsdnlines; j; j--) {
                if (nl < 0) {
                    /* Проверяем счетчик пустых строк */
                    if (bflag && *c != 1)
                        bflag = 0;
                    else if (bflag == 0 && *c == 1) {
                        bflag = 1;
                        if (++nl == 0)
                            break;
                    }
                }
                if (*c & 0200)
                    i += 128 * (*c++ & 0177);
                i += *c++;
                ++tlines;
                /* Проверяем счетчик строк */
                if (nl > 0 && --nl == 0)
                    break;
            }
            lseek(f->fsdfile, f->seek, SEEK_SET);
            while (i) {
                j = (i < LBUFFER) ? i : LBUFFER;
                read(f->fsdfile, cline, j);
                if (write(newf, cline, j) < 0) {
                    error("DANGER -- WRITE ERROR");
                    close(newf);
                    return(-1);
                }
                i -= j;
            }
        } else {
            j = f->fsdnlines;
            if (nl < 0) {
                if (bflag == 0 && ++nl == 0)
                    j = 0;
                bflag = 1;
            } else {
                if (j > nl)
                    j = nl;
                nl -= j;
            }
            k = j;
            while (k)
                cline[--k] = NEWLINE;
            if (j && write(newf, cline, j) < 0) {
                error("DANGER -- WRITE ERROR");
                close(newf);
                return(-1);
            }
            tlines += j;
        }
        f = f->fwdptr;
    }
    close(newf);
    return tlines;
}

/*
 * Открыть файл file для редактирования, начиная со строки
 * line и колонки col.
 * Файл открывается в текущем окне.
 * Если файла нет, а mkflg равен 1, то запрашивается
 * разрешение создать файл.
 * Код ответа -1, если файл не открыли и не создали.
 * Если putflg равен 1, файл тут же выводится в окно.
 */
int editfile(file, line, col, mkflg, puflg)
    char *file;
    int line, col, mkflg, puflg;
{
    int i, j;
    register int fn;
    register char *c,*d;

    fn = -1;
    for (i=0; i<MAXFILES; ++i) {
        if (openfnames[i] != 0) {
            c = file;
            d = openfnames[i];
            while (*c++ == *d) {
                if (*(d++) == 0) {
                    fn = i;
                    break;
                }
            }
        }
    }
    if (fn < 0) {
        fn = open(file, 0);  /* Файл существует? */
        if (fn >= 0) {
            if (fn >= MAXFILES) {
                error("Too many files -- editor limit!");
                close(fn);
                return(0);
            }
            j = checkpriv(fn);
            if (j == 0) {
                error("File read protected.");
                close(fn);
                return(0);
            }
            openwrite[fn] = (j == 2) ? 1 : 0;
            telluser("Use: ",0);
            telluser(file, 5);
        } else {
            if (! mkflg)
                return (-1);
            telluser("Hit <use> (ctrl-d) to make: ",0);
            telluser(file, 28);
            lread1 = -1;
            read1();
            if (lread1 != CCSETFILE&&lread1 != 'Y' &&lread1 != 'y')
                return(-1);
            /* Находим справочник */
            for (c=d=file; *c; c++)
                if (*c == '/')
                    d = c;
            if (d > file) {
                *d = '\0';
                i = open(file,0);
            } else
                i = open(".",0);

            if (i < 0) {
                error("Specified directory does not exist.");
                return(0);
            }
            if (checkpriv(i) != 2) {
                error("Can't write in:");
                telluser (file,21);
                return(0);
            }
            close(i);
            if (d > file)
                *d = '/';

            /* Создаем файл */
            fn = creat(file,FILEMODE);
            close(fn);
            fn = open(file, 0);
            if (fn < 0) {
                error("Create failed!");
                return(0);
            }
            if (fn >= MAXFILES) {
                close(fn);
                error("Too many files -- Editor limit!");
                return(0);
            }
            openwrite[fn] = 1;
            chown(file, userid, groupid);
        }
        paraml = 0;   /* so its kept around */
        openfnames[fn] = file;
    }
    /* Выталкиваем буфер, так как здесь долгая операция */
    dumpcbuf();
    switchwksp();
    if (openfsds[fn] == (struct fsd *)0)
        openfsds[fn] = file2fsd(fn);
    curwksp->curfsd = openfsds[fn];
    curfile = curwksp->wfile = fn;
    curwksp->curlno = curwksp->curflno = 0;
    curwksp->ulhclno = line;
    curwksp->ulhccno = col;
    if (puflg) {
        putup(0, curport->btext);
        poscursor(0, defplline);
    }
    return(1);
}

/*
 * End a session and write all.
 * Return 0 on write error.
 */
int endit()
{
    register int i, ko = 1;

    for (i=0; i<MAXFILES; i++)
        if (openfsds[i] && openwrite[i] == EDITED)
            if (savefile(NULL, i) == 0)
                ko = 0;
    return(ko);
}
