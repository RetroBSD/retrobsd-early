/*
 * Main loop of editor.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"
#include <sys/wait.h>

char use0flg;   /* используется для блокировки преобразования первого имени файла при "lcase" и "red - */
int clrsw;      /* 1 - чистить окно параметров */
int csrsw;      /* 1 - показать текущую позицию меткой  */
int imodesw = 1; /* 1 - insert mode  */
int oldccol;    /* используется для отметки положения курсора */
int oldcline;   /* - - / / - - */

/*
 * Add a tab stop.
 */
static void settab(tabcol)
    int tabcol;
{
    register int i, j;

    if (tabstops[NTABS-1] == BIGTAB) {
        error("Too many tabstops; can't set more.");
        return;
    }
    for (i=0; i<NTABS; i++) {
        if (tabstops[i] == tabcol)
            return;
        if (tabstops[i] > tabcol) {
            for (j=NTABS-1; j>i; j--)
                tabstops[j] = tabstops[j-1];
            tabstops[i] = tabcol;
            return;
        }
    }
    error("Error in settab!");
}

/*
 * Clear a tab stop.
 */
static void clrtab(tabcol)
    int tabcol;
{
    register int i, j;

    for (i=0; i<NTABS; i++)
        if (tabstops[i] == tabcol) {
            for (j=i; j<NTABS-1; j++)
                tabstops[j] = tabstops[j+1];
            tabstops[NTABS-1] = 0;
            return;
        }
}

/*
 * Выполнить команду "exec"
 * Ответ 1, если не было ошибок
 */
static int callexec()
{
    register int i;
    char **execargs;
    register char *cp, **e;
    int j, k, m, pipef[2];
    char pwbuf[100];
#define NARGS 20

    /*
     * 1. Разбираем размер области.
     */
    i = curwksp->ulhclno + cursorline;
    m = 1;
    cp = paramv;
    if (*cp == '-' || (*cp >= '0' && *cp <= '9')) {
        cp = s2i(paramv, &m);
        if (cp == 0)
            goto noargerr;
    }
    m = -m;           /* По умолчанию - 1 параграф */
    if (*cp == 'l') {
        cp++;
        m = -m;
    }  /* nl == -n */
    /*
     * 2. Готовим строку аргументов.
     */
    e = execargs = (char **)salloc(NARGS*(sizeof (char *)));
    while (*cp == ' ') cp++;
    while (*cp != 0) {
        *e++ = cp;              /* адрес аргумента */
        if ((e - execargs) >= NARGS)
            goto noargerr; /* Слишком много */
        if (*cp == '"') {
            cp++;
            e[-1]++;
            while (*cp++ !=  '"')
                if (*cp == 0)
                    goto noargerr;
            cp[-1] = 0;
        }
        else if (*cp++ == '\'') {
            e[-1]++;
            while (*cp++ !=  '\'')
                if (*cp == 0)
                    goto noargerr;
            cp[-1] = 0;
        }
        else {
            while (*cp != ' ' && *cp != ',' && *cp)
                cp++;
        }
        while (*cp == ' ' || *cp == ',')
            *cp++ = 0;
    }
    *e = 0;
    /*
     * 3. Запускаем команду через pipe
     * (red) | (команда;red)
     * Вторая копия red занимается тем, что дочитывает
     * остаток информации из трубы.
     */
    if (pipe(pipef) == -1)
        goto nopiperr;
    if ((j = fork()) == -1)
        goto nopiperr;
    if (j == 0) {               /* команда;red */
        close(0);               /* Замыкаем в станд. ввод */
        if (dup(pipef[0]) < 0)
            exit(-1);
        close(1);               /* Вывод в рабочий файл */
        open(tmpname, 1);
        lseek(1, tempfl, 0);
        j = 2;
        /* Закрываем все, что осталось открыто */
        while ((k = dup(1)) >= 0)
            if (k > j)
                j = k;
        while (j >= 2)
            close(j--);
        if ((i = fork()) == -1)
            goto nopiperr;
        if (i != 0) {                       /* ;red   */
            while (wait(&m) != i);          /* Ждем, затем читаем */
            while (read(0, pwbuf, 100));    /* Пока не надоест */
            exit(m >> 8);                   /* И возвр. статус      */
        }
        execr(execargs);
        /* exit теперь в EXECR, -1 если ошибка */
    }
    /* Отец: */
    telluser("Executing ...",0);
    free((char *)execargs);
    doreplace(i, m, j, pipef);
    return(1);
nopiperr:
    error("Can not fork or write pipe.");
    return(0);
noargerr:
    error("Invalid argument.");
    return(0);
}

/*
 * Main editor loop.
 */
void mainloop()
{
    int i, m, first = 1;
    register int j;
    int clsave = 0, ccsave = 0, k;
    /* Хитрости с экономией памяти */
    int thiscol, thisrow;
    char ich[8], *cp;
    /* Для команд с тремя вариантами аргументов */
    void (*lnfun)(int, int);
    void (*spfun)(int, int, int, int);
    struct viewport *oport;

    /*
     * Обработка одного символа или команды
     */
    if (cursorline == 0)
        oldcline = 1;
    if (cursorcol == 0)
        oldccol  = 1;
#ifndef lint
    goto funcdone;
#endif
    for (;;) {
        csrsw = clrsw = 0;
        read1();
        if (errsw) {
            errsw = 0;
            clrsw = 1;
            goto errclear;
        }
        /*
         * Редактирование в строке
         */
        if (! CTRLCHAR(lread1) || lread1 == CCCTRLQUOTE ||
            lread1 == CCBACKSPACE || lread1 == CCDELCH)
        {
            /* Отмена в 1 колонке */
            if (lread1 == CCBACKSPACE && cursorcol == 0)
            {
                lread1 = -1;
                goto contin;
            }
            if (openwrite[curfile] == 0)
                goto nowriterr;

            /* Строки у нас нет? Дай! */
            if (clineno != (i = curwksp->ulhclno+cursorline))
                getlin(i);

            /* исключение символа */
            if (lread1==CCDELCH || (imodesw && lread1==CCBACKSPACE))
            {
                thiscol = cursorcol + curwksp->ulhccno;
                thisrow = cursorline;
                if (lread1 == CCBACKSPACE)
                    thiscol--;
                if (ncline < thiscol + 2) {
                    if (lread1 == CCBACKSPACE)
                        movecursor(CCMOVELEFT);
                    lread1 = -1;
                    goto contin;
                }
                for (i=thiscol; i<ncline-2; i++)
                    cline[i] = cline[i+1];
                ncline--;
                thiscol -= curwksp->ulhccno;
                putup(-(1+thiscol),cursorline);
                poscursor(thiscol,thisrow);
                fcline = 1;
                lread1 = -1;
                goto contin;
            }
            /* Проверка на границу окна */
            if (cursorcol > curport->rtext) {
                if (fcline) {
                    putline(0);
                    movep(defrport);
                    goto contin;
                }
                goto margerr;
            }
            fcline = 1;
            j = (lread1 == CCBACKSPACE);
            if (j) {
                movecursor(CCMOVELEFT);
                lread1 = ' ';
            }
            i = cursorcol + curwksp->ulhccno;
            if (i >= (lcline - 2))
                excline(i + 2);
            if (i >= ncline-1) {
                for (k=ncline-1; k<=i; k++)
                    cline[k] = ' ';
                cline[i+1] = NEWLINE;
                ncline = i+2;
            }
            else if (imodesw) {
                thiscol = cursorcol + curwksp->ulhccno;
                thisrow = cursorline;
                if (ncline >= lcline)
                    excline(ncline+1);
                for (i=ncline; i>thiscol; i--)
                    cline[i] = cline[i-1];
                ncline++;
                thiscol -= curwksp->ulhccno;
                putup(-(1 + thiscol), cursorline);
                poscursor(thiscol, thisrow);
            }
            /* Выставим границу */
            if (cursorcol >= curport->rtext)
                curport->redit = curport->rtext + 1;

            /* Замена символа */
            if (lread1 == CCCTRLQUOTE)
                lread1 = esc0;
            if (cursorcol == curport->rtext - 10)
                putcha(COBELL);
            cline[i] = lread1;
            putch(lread1, 1);

            /* Если переехали границу */
            curport->redit = curport->rtext;
            if (j)
                movecursor(CCMOVELEFT);
            lread1 = -1;
            goto contin;
        }
        /* Сдвиг вниз, если последняя строка  */
        if (lread1 == CCRETURN) {
            putline(0);
            if (cursorline == curport->btext)
                movew(defplline);
            i = curwksp->ulhccno;
            if (i !=0)
                movep(-i);
            movecursor(lread1);
            lread1 = -1;
            goto errclear;
        }
        /*
         * Если команда перемещения
         */
        if (lread1 <= CCBACKTAB) {
            movecursor(lread1);
            if (lread1 == CCMOVEUP || lread1 == CCMOVEDOWN ||
                lread1 == CCRETURN || lread1 == CCHOME)
            {
                /* Row changed: save current line. */
                putline(0);
                goto newnumber;
            }
            lread1 = -1;
            goto contin;
        }
        /* Если граница поля */
        if (cursorcol > curport->rtext)
            poscursor(curport->rtext, cursorline);
        putline(0);
        if (lread1 == CCQUIT) {
            if (endit() == 0)
                goto funcdone;
            return;
        }
        switch (lread1) {
        case CCENTER:
            goto gotarg;

        case CCLPORT:
            movep(- deflport);
            goto funcdone;

        case CCSETFILE:
            switchfile();
            goto funcdone;

        case CCCHPORT:
            chgport(-1);
            goto funcdone;

        case CCOPEN:
            if (openwrite[curfile] == 0)
                goto nowriterr;
            openlines(curwksp->ulhclno + cursorline, definsert);
            goto funcdone;

        case CCMISRCH:
            search(-1);
            goto funcdone;

        case CCCLOSE:
            if (openwrite[curfile]==0)
                goto nowriterr;
            closelines(curwksp->ulhclno + cursorline, defdelete);
            goto funcdone;

        case CCPUT:
            if (openwrite[curfile] == 0)
                goto nowriterr;
            if (pickbuf->nrows == 0)
                goto nopickerr;
            put(pickbuf, curwksp->ulhclno + cursorline,
                curwksp->ulhccno + cursorcol);
            goto funcdone;

        case CCPICK:
            picklines(curwksp->ulhclno + cursorline, defpick);
            goto funcdone;

        case CCINSMODE:
            imodesw = 1 - imodesw;  /* change it */
            goto funcdone;

        case CCGOTO:
            gtfcn(0);
            goto funcdone;

        case CCMIPAGE:
            movew(- defmipage * (1 + curport->btext));
            goto funcdone;

        case CCPLSRCH:
            search(1);
            goto funcdone;

        case CCRPORT:
            movep(defrport);
            goto funcdone;

        case CCPLLINE:
            movew(defplline);
            goto funcdone;

        case CCDELCH:
            goto notimperr;

        case CCSAVEFILE:
            savefile(NULL,curfile);
            goto funcdone;

        case CCMILINE:
            movew(-defmiline);
            goto funcdone;

        case CCDOCMD:
            goto notstrerr;

        case CCPLPAGE:
            movew(defplpage * (1 + curport->btext));
            goto funcdone;

        case CCMAKEPORT:
            makeport(deffile);
            goto funcdone;

        case CCTABS:
            settab(curwksp->ulhccno + cursorcol);
            goto funcdone;

        case CCREDRAW:                  /* Redraw screen */
        case CCEND:                     /* TODO */
            rescreen();
            lread1 = -1;
            goto funcdone;

        /* case CCMOVELEFT: */
        /* case CCTAB:      */
        /* case CCMOVEDOWN: */
        /* case CCHOME:     */
        /* case CCRETURN:   */
        /* case CCMOVEUP:   */
        default:
            goto badkeyerr;
        }
        /* Повтор ввода аргумента */
reparg:
        read1();
        if (CTRLCHAR(lread1))
            goto yesarg;
        goto noargerr;
        /*
         * Дай аргумент!
         */
gotarg:
        param(0);
yesarg:
        if (lread1 == CCQUIT) {
            if (paraml > 0 &&
                (dechars(paramv, paraml), *paramv) == 'a')
            {
                if (*(paramv+1) != 'd')
                    return;
                cleanup();
                inputfile = -1; /* to force a dump */
                fatal("ABORTED");
            }
            if (endit() == 0)
                goto funcdone;
            return;
        }
        switch (lread1) {
        case CCENTER:
            goto funcdone;

        case CCLPORT:
            if (paramtype <= 0)
                goto notstrerr;
            if (s2i(paramv, &i))
                goto notinterr;
            movep(-i);
            goto funcdone;

        case CCSETFILE:
            if (paramtype <= 0)
                goto notstrerr;
            if (paramv == 0)
                goto noargerr;
            if (use0flg || ! inputfile)
                dechars(paramv, paraml);
            use0flg = 1;
            editfile(paramv, 0, 0, 1, 1);
            goto funcdone;

        case CCCHPORT:
            if (paramtype <= 0)
                goto notstrerr;
            if (s2i(paramv, &i))
                goto notinterr;
            if (i <= 0)
                goto notposerr;
            chgport(i - 1);
            goto funcdone;

        case CCOPEN:
            if (openwrite[curfile] == 0)
                goto nowriterr;
            if (paramtype != 0) {
                lnfun = openlines;
                spfun = openspaces;
                goto spdir;
            }
            splitline(curwksp->ulhclno + paramr0,
                paramc0 + curwksp->ulhccno);
            goto funcdone;

        case CCMISRCH:
        case CCPLSRCH:
            if (paramtype <= 0)
                goto notstrerr;
            if (paramv == 0)
                goto noargerr;
            if (searchkey)
                free(searchkey);
            searchkey = paramv;
            paraml = 0;
            search(lread1 == CCPLSRCH ? 1 : -1);
            goto funcdone;

        case CCCLOSE:
            if (openwrite[curfile] == 0)
                goto nowriterr;
            if (paramtype != 0) {
                if (paramtype > 0 && paramv && paramv[0] == '>') {
                    msrbuf(deletebuf, paramv+1, 0);
                    goto funcdone;
                }
                lnfun = closelines;
                spfun = closespaces;
                goto spdir;
            }
            combineline(curwksp->ulhclno + paramr0,
                paramc0 + curwksp->ulhccno);
            goto funcdone;

        case CCPUT:
            if (paramtype > 0 && paramv && paramv[0] == '$') {
                if (msrbuf(pickbuf, paramv+1, 1))
                    goto errclear;
                goto funcdone;
            }
            if (paramtype != 0)
                goto notstrerr;
            if (openwrite[curfile] == 0)
                goto nowriterr;
            if (deletebuf->nrows == 0)
                goto nodelerr;
            put(deletebuf, curwksp->ulhclno + cursorline,
                curwksp->ulhccno + cursorcol);
            goto funcdone;

        case CCMOVELEFT:
        case CCTAB:
        case CCMOVEDOWN:
        case CCHOME:
        case CCMOVEUP:
        case CCMOVERIGHT:
        case CCBACKTAB:
            if (s2i(paramv, &i))
                goto notinterr;
            if (i <= 0)
                goto notposerr;
            m = ((lread1 <= CCBACKTAB) ? lread1 : 0);
            while (--i >= 0)
                movecursor(m);
            goto funcdone;

        case CCRETURN:
            if (paramtype <= 0 || ! paramv)
                goto notimperr;
            dechars(paramv, paraml);
            switch (paramv[0]) {
            case '>':
                msvtag(paramv + 1);
                break;
            case '$':
                if(mdeftag(paramv + 1)){
                    lread1 = -1;
                    goto reparg;
                }
                break;
            case 'w':
                if(paramv[1] == ' ' && paramv[2] == '+')
                    openwrite[curwksp->wfile] = 1;
                else
                    openwrite[curwksp->wfile] = 0;
                break;
            case 'k':
                defkey();
                break;
            case 'r':           /* Redraw screen */
                rescreen();
                break;
            case 'd':
                if (paramv[1] == ' ')
                    defmac(&paramv[2]);
                break;
            case 'q':
                lread1 = CCQUIT;
                if (paramv[1] == 'a') {
                    return;
                }
                goto contin;
            default:
                goto noargerr;
            }
            goto funcdone;

        case CCPICK:
            if (paramtype == 0)
                goto notimperr;
            if (paramtype > 0 && paramv && paramv[0] == '>') {
                msrbuf(pickbuf, paramv+1, 0);
                goto funcdone;
            }
            lnfun = picklines;
            spfun = pickspaces;
            goto spdir;

        case CCINSMODE:
            imodesw = ! imodesw;
            goto funcdone;

        case CCGOTO:
            if (paramtype == 0)
                gtfcn(nlines[curfile]);
            else if (paramtype > 0) {
                if(paramv && paramv[0] == '$') {
                    mgotag(paramv + 1);
                    goto funcdone;
                }
                if (s2i(paramv, &i))
                    goto notinterr;
                gtfcn(i - 1);
            } else
                goto noargerr;
            goto funcdone;

        case CCMIPAGE:
            if (paramtype <= 0)
                goto notstrerr;
            if (s2i(paramv, &i))
                goto notinterr;
            movew(- i * (1 + curport->btext));
            goto funcdone;

        case CCRPORT:
            if (paramtype <= 0)
                goto notstrerr;
            if (s2i(paramv, &i))
                goto notinterr;
            movep(i);
            goto funcdone;

        case CCPLLINE:
            if (paramtype < 0)
                goto notstrerr;
            else if (paramtype == 0)
                movew(cursorline);
            else if (paramtype > 0) {
                if (s2i(paramv, &i))
                    goto notinterr;
                movew(i);
            }
            goto funcdone;

        case CCDELCH:
            goto notimperr;

        case CCSAVEFILE:
            if (paramtype <= 0)
                goto notstrerr;
            if (paramv == 0)
                goto noargerr;
            dechars(paramv, paraml);
            savefile(paramv, curfile);
            goto funcdone;

        case CCMILINE:
            if (paramtype < 0)
                goto notstrerr;
            else if (paramtype == 0)
                movew(cursorline - curport->btext);
            else if (paramtype > 0) {
                if (s2i(paramv, &i))
                    goto notinterr;
                movew(-i);
            }
            goto funcdone;

        case CCDOCMD:
            if (paramtype <= 0)
                goto notstrerr;
            dechars(paramv, paraml);
            if (openwrite[curfile] == 0)
                goto nowriterr;
            callexec();
            goto funcdone;

        case CCPLPAGE:
            if (paramtype <= 0)
                goto notstrerr;
            if (s2i(paramv, &i))
                goto notinterr;
            movew(i * (1 + curport->btext));
            goto funcdone;

        case CCMAKEPORT:
            if (paramtype == 0)
                removeport();
            else if (paramtype < 0)
                goto notstrerr;
            else {
                dechars(paramv, paraml);
                makeport(paramv);
            }
            goto funcdone;

        case CCTABS:
            clrtab(curwksp->ulhccno + cursorcol);
            goto funcdone;

        default:
            goto badkeyerr;
        }
spdir:
        if (paramtype > 0) {
            if(paramv[0] == '$') {
                if (mdeftag(paramv + 1))
                    goto spdir;
                goto funcdone;
            }
            if (s2i(paramv, &i))
                goto notinterr;
            if (i <= 0)
                goto notposerr;
            (*lnfun)(curwksp->ulhclno + cursorline, i);
        } else {
            if (paramc1 == paramc0) {
                (*lnfun)(curwksp->ulhclno + paramr0,
                    (paramr1 - paramr0) + 1);
            } else
                (*spfun)(curwksp->ulhclno + paramr0,
                    curwksp->ulhccno + paramc0,
                    (paramc1 - paramc0),
                    (paramr1 - paramr0) + 1);
        }
        goto funcdone;
badkeyerr:
        error("Illegal key or unnown macro");
        goto funcdone;
notstrerr:
        error("Argument must be a string.");
        goto funcdone;
noargerr:
        error("Invalid argument.");
        goto funcdone;
notinterr:
        error("Argument must be numerik.");
        goto funcdone;
notposerr:
        error("Argument must be positive.");
        goto funcdone;
nopickerr:
        error("Nothing in the pick buffer.");
        goto funcdone;
nodelerr:
        error ("Nothing in the close buffer.");
        goto funcdone;
notimperr:
        error("Feature not implemented yet.");
        goto funcdone;
margerr:
        error("Margin stusk; move cursor to free.");
        goto funcdone;
nowriterr:
        error("You cannot modify this file!");
        goto funcdone;
funcdone:
        clrsw = 1;
newnumber:
        lread1 = -1;        /* signify char read was used */
errclear:
        oport = curport;
        k = cursorline;
        j = cursorcol;
        switchport(&paramport);
        paramport.redit = PARAMRINFO;
        if (clrsw) {
            if (! errsw && ! first) {
                poscursor(0, 0);
                putstr(blanks, PARAMRINFO);
            }
            poscursor(PARAMREDIT + 2, 0);
            if (oport->wksp->wfile) {
                putstr("file ", PARAMRINFO);
                putstr(openfnames[oport->wksp->wfile], PARAMRINFO);
            }
            putstr(" line ", PARAMRINFO);
            clsave = cursorline;
            first = 0;
            ccsave = cursorcol;
        }
        poscursor(ccsave, clsave);
        i = oport->wksp->ulhclno + k + 1; /* Рисуем номер строки */
        cp = ich + 8;
        *--cp = '\0';
        do {
            (*--cp = '0' + (i % 10));
            i /= 10;
        } while (i);
        putstr(cp, PARAMRINFO);
        *cp = '\0';
        while (cp != ich)
            *--cp = ' ';
        putstr(ich, PARAMRINFO);
        switchport(oport);
        paramport.redit = PARAMREDIT;
        poscursor(j, k);
        if (csrsw) {
            putch(COCURS, 1);
            poscursor(j, k);
            dumpcbuf();
            sleep(1);
            putup(k, k);
            poscursor(j, k);
        }
        if (imodesw && clrsw && ! errsw)
            telluser("     ***** i n s e r t m o d e *****",0);
contin:
        ;
    }
}

/*
 * Search forward/backward in file.
 * delta = 1 / -1
 * Setup a window when needed, and show a found text.
 * Search for "searchkey".
 */
void search(delta)
    int delta;
{
    register char *at, *sk, *fk;
    int ln, lkey, col, lin, slin, i;

    paraml = 0;
    if (searchkey == 0 || *searchkey == 0) {
        error("Nothing to search for.");
        return;
    }
    col = cursorcol;
    slin = lin = cursorline;
    if (delta == 1)
        telluser("+", 0);
    else
        telluser("-", 0);
    telluser("search: ", 1);
    telluser(searchkey, 9);
    putch(COCURS, 1);
    poscursor(col, lin);
    dumpcbuf();
    lkey = 0;
    sk = searchkey;
    while (*sk++)
        lkey++;
    ln = lin + curwksp->ulhclno;
    getlin (ln);
    putline(0);
    at = cline + col + curwksp->ulhccno;
    for (;;) {
        at += delta;
        while (at < cline || at >  cline + ncline - lkey) {
            /* Прервать, если было прерывание с терминала */
            i = interrupt();
            if (i || (ln += delta) < 0 ||
                (wposit(curwksp, ln) && delta == 1))
            {
                putup(lin, lin);
                poscursor(col, lin);
                error(i ? "Interrupt." : "Search key not found.");
                csrsw = 0;
                symac = 0;
                return;
            }
            getlin(ln);
            putline(0);
            at = cline;
            if (delta < 0)
                at += ncline - lkey;
        }
        sk = searchkey;
        fk = at;
        while (*sk == *fk++ && *++sk);
        if (*sk == 0) {
            cgoto(ln, at - cline, slin, 0);
            csrsw = 1;  /* put up a bullit briefly */
            return;
        }
    }
}
