/*
 * Opening and saving files.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"

/*
 * Write file from channel n to file.
 * When no file given use the name from file[n].name.
 * When no permission to write, ask to write to ".".
 */
int savefile(filename, n)
    char *filename;
    int n;
{
    register char *f1;
    char *f0, *f2;
    register int i, j;
    int newf, nowrbak = 0;

    /* дай справочник */
    if (filename) {
        f0 = filename;
    } else {
        f0 = file[n].name;
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
        if (filename) {
            error ("Can't write in specified directory");
            return(0);
        }
        if (f2 > f0) {
            telluser("Hit ^N to use '.'",0);
            keysym = -1;
            nowrbak = 0;
            getkeysym();
            if (keysym != CCSETFILE)
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
    if (nowrbak && ! file[n].backup_done) {
        nowrbak = 0;
        file[n].backup_done = 1;
    }
    if (! nowrbak) {
        unlink(f1);
        if (link (f0, f1) < 0) {
            error ("Link failed!");
            return(0);
        }
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
    return segmwrite(file[n].chain, 077777, newf) == -1 ? 0 : 1;
}

/*
 * Запись по цепочке описателей f в файл "newf"
 * Если nl # 0  - записывать только nl строк или
 * -nl абзацев текста (nl используется в случае
 * команды "exec".
 * Ответ - число записанных строк, или -1, если ошибка.
 */
int segmwrite(ff, nl, newf)
    segment_t *ff;
    int nl, newf;
{
    register segment_t *f;
    register char *c;
    register int i;
    int j, k, bflag, tlines;

    if (cline_max < LBUFFER)
        excline(LBUFFER);
    f = ff;
    bflag = 1;
    tlines = 0;
    while (f->fdesc && nl) {
        if (f->fdesc > 0) {
            i = 0;
            c = &f->data;
            for (j=f->nlines; j; j--) {
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
            lseek(f->fdesc, f->seek, 0);
            while (i) {
                j = (i < LBUFFER) ? i : LBUFFER;
                if (read(f->fdesc, cline, j) < 0)
                    /* ignore errors */;
                if (write(newf, cline, j) < 0) {
                    error("DANGER -- WRITE ERROR");
                    close(newf);
                    return(-1);
                }
                i -= j;
            }
        } else {
            j = f->nlines;
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
                cline[--k] = '\n';
            if (j && write(newf, cline, j) < 0) {
                error("DANGER -- WRITE ERROR");
                close(newf);
                return(-1);
            }
            tlines += j;
        }
        f = f->next;
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
int editfile(filename, line, col, mkflg, puflg)
    char *filename;
    int line, col, mkflg, puflg;
{
    int i, j;
    register int fn;
    register char *c,*d;

    fn = -1;
    for (i=0; i<MAXFILES; ++i) {
        if (file[i].name != 0) {
            c = filename;
            d = file[i].name;
            while (*c++ == *d) {
                if (*(d++) == 0) {
                    fn = i;
                    break;
                }
            }
        }
    }
    if (fn < 0) {
        fn = open(filename, 0);  /* Файл существует? */
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
            file[fn].writable = (j == 2) ? 1 : 0;
            telluser("Use: ",0);
            telluser(filename, 5);
        } else {
            if (! mkflg)
                return (-1);
            telluser("Hit ^N to create new file: ",0);
            telluser(filename, 28);
            keysym = -1;
            getkeysym();
            if (keysym != CCSETFILE && keysym != 'Y' && keysym != 'y')
                return(-1);
            /* Находим справочник */
            for (c=d=filename; *c; c++)
                if (*c == '/')
                    d = c;
            if (d > filename) {
                *d = '\0';
                i = open(filename, 0);
            } else
                i = open(".",0);

            if (i < 0) {
                error("Specified directory does not exist.");
                return(0);
            }
            if (checkpriv(i) != 2) {
                error("Can't write in:");
                telluser (filename, 21);
                return(0);
            }
            close(i);
            if (d > filename)
                *d = '/';

            /* Создаем файл */
            fn = creat(filename, FILEMODE);
            close(fn);
            fn = open(filename, 0);
            if (fn < 0) {
                error("Create failed!");
                return(0);
            }
            if (fn >= MAXFILES) {
                close(fn);
                error("Too many files -- Editor limit!");
                return(0);
            }
            file[fn].writable = 1;
            if (chown(filename, userid, groupid) < 0)
                /* ignore errors */;
        }
        paraml = 0;   /* so its kept around */
        file[fn].name = filename;
    }
    /* Выталкиваем буфер, так как здесь долгая операция */
    dumpcbuf();
    wksp_switch();
    if (file[fn].chain == (segment_t *)0)
        file[fn].chain = file2segm(fn);
    curwksp->cursegm = file[fn].chain;
    curfile = curwksp->wfile = fn;
    curwksp->line = curwksp->segmline = 0;
    curwksp->toprow = line;
    curwksp->topcol = col;
    if (puflg) {
        putup(0, curwin->text_height);
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
        if (file[i].chain && file[i].writable == EDITED)
            if (savefile(NULL, i) == 0)
                ko = 0;
    return(ko);
}
