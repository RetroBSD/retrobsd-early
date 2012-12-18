/*
 * Main loop of editor.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"
#include <sys/wait.h>

static int insert_mode = 1; /* Insert mode */
static int clr_arg_area;    /* Need to erase the arg area */
static char use0flg;        /* используется для блокировки преобразования первого имени файла при "red -" */

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
    i = curwksp->toprow + cursorline;
    m = 1;
    cp = param_str;
    if (*cp == '-' || (*cp >= '0' && *cp <= '9')) {
        cp = s2i(param_str, &m);
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
        lseek(1, tempseek, 0);
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
    int i, m, k;
    register int j;
    int lnum_row = 0, lnum_col = 0;     /* position to draw a line number */
    char ich[8], *cp;
    /* Для команд с тремя вариантами аргументов */
    void (*lnfun)(int, int);
    void (*spfun)(int, int, int, int);
    window_t *owin;

    /*
     * Обработка одного символа или команды
     */
    for (;;) {
        clr_arg_area = 1;
newnumber:
        keysym = -1;        /* mark the symbol as used */
errclear:
        owin = curwin;
        k = cursorline;
        j = cursorcol;
        win_switch(&paramwin);
        paramwin.text_width = PARAMRINFO;
        if (clr_arg_area) {
            if (! message_displayed) {
                poscursor(0, 0);
                putstr(blanks, PARAMRINFO);
            }
            poscursor(PARAMREDIT + 2, 0);
            if (owin->wksp->wfile) {
                putstr("file ", PARAMRINFO);
                putstr(file[owin->wksp->wfile].name, PARAMRINFO);
            }
            putstr(" line ", PARAMRINFO);
            lnum_row = cursorline;
            lnum_col = cursorcol;
        }
        /* Display the current line number. */
        poscursor(lnum_col, lnum_row);
        i = owin->wksp->toprow + k + 1;
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
        win_switch(owin);
        paramwin.text_width = PARAMREDIT;
        poscursor(j, k);
        if (highlight_position) {
            putch(COCURS, 1);
            poscursor(j, k);
            dumpcbuf();
            sleep(1);
            drawlines(k, k);
            poscursor(j, k);
        }
        if (insert_mode && clr_arg_area && ! message_displayed)
            telluser("     *** I n s e r t   m o d e ***", 0);
nextkey:
        highlight_position = 0;
        clr_arg_area = 0;
        getkeysym();
        if (message_displayed) {
            /* Clear the message after any key pressed. */
            message_displayed = 0;
            clr_arg_area = 1;
            goto errclear;
        }

        /*
         * Редактирование в строке
         */
        if (! CTRLCHAR(keysym) || keysym == CCCTRLQUOTE ||
            keysym == CCBACKSPACE || keysym == CCDELCH)
        {
            /* Отмена в 1 колонке */
            if (keysym == CCBACKSPACE && cursorcol == 0) {
                keysym = -1;
                goto nextkey;
            }
            if (file[curfile].writable == 0)
                goto nowriterr;

            /* Строки у нас нет? Дай! */
            i = curwksp->toprow + cursorline;
            if (clineno != i)
                getlin(i);

            /* исключение символа */
            if (keysym == CCDELCH || (insert_mode && keysym == CCBACKSPACE)) {
                int thiscol = cursorcol + curwksp->topcol;
                int thisrow = cursorline;

                if (keysym == CCBACKSPACE)
                    thiscol--;
                if (cline_len < thiscol + 2) {
                    if (keysym == CCBACKSPACE)
                        movecursor(CCMOVELEFT);
                    keysym = -1;
                    goto nextkey;
                }
                for (i=thiscol; i<cline_len-2; i++)
                    cline[i] = cline[i+1];
                cline_len--;
                thiscol -= curwksp->topcol;
                drawlines(-thiscol-1, cursorline);
                poscursor(thiscol, thisrow);
                cline_modified = 1;
                keysym = -1;
                goto nextkey;
            }
            /* Проверка на границу окна */
            if (cursorcol > curwin->text_width) {
                if (cline_modified) {
                    putline();
                    movep(defrindent);
                    goto nextkey;
                }
                goto margerr;
            }
            cline_modified = 1;
            j = (keysym == CCBACKSPACE);
            if (j) {
                movecursor(CCMOVELEFT);
                keysym = ' ';
            }
            i = cursorcol + curwksp->topcol;
            if (i >= (cline_max - 2))
                excline(i + 2);
            if (i >= cline_len-1) {
                for (k=cline_len-1; k<=i; k++)
                    cline[k] = ' ';
                cline[i+1] = '\n';
                cline_len = i+2;
            }
            else if (insert_mode) {
                int thiscol = cursorcol + curwksp->topcol;
                int thisrow = cursorline;

                if (cline_len >= cline_max)
                    excline(cline_len+1);
                for (i=cline_len; i>thiscol; i--)
                    cline[i] = cline[i-1];
                cline_len++;
                thiscol -= curwksp->topcol;
                drawlines(-thiscol-1, cursorline);
                poscursor(thiscol, thisrow);
            }
#if 0
            /* Set the margin... I do not understand this (vak) */
            if (cursorcol >= curwin->text_width)
                curwin->edit_width = curwin->text_width + 1;
#endif
            /* Замена символа */
            if (keysym == CCCTRLQUOTE)
                keysym = COCURS;
            if (cursorcol == curwin->text_width - 10)
                putcha(COBELL);
            cline[i] = keysym;
            putch(keysym, 1);

#if 0
            /* Если переехали границу */
            curwin->edit_width = curwin->text_width;
#endif
            if (j)
                movecursor(CCMOVELEFT);
            keysym = -1;
            goto nextkey;
        }
        /* Сдвиг вниз, если последняя строка  */
        if (keysym == CCRETURN) {
            putline();
            if (cursorline == curwin->text_height)
                movew(defplline);
            i = curwksp->topcol;
            if (i !=0)
                movep(-i);
            movecursor(keysym);
            keysym = -1;
            goto errclear;
        }
        /*
         * Если команда перемещения
         */
        if (keysym <= CCBACKTAB) {
            movecursor(keysym);
            if (keysym == CCMOVEUP || keysym == CCMOVEDOWN ||
                keysym == CCRETURN || keysym == CCHOME)
            {
                /* Row changed: save current line. */
                putline();
                goto newnumber;
            }
            keysym = -1;
            goto nextkey;
        }
        /* Если граница поля */
        if (cursorcol > curwin->text_width)
            poscursor(curwin->text_width, cursorline);
        putline();
        switch (keysym) {
        case CCQUIT:
            if (endit())
                return;
            continue;

        case CCENTER:
            goto gotarg;

        case CCLINDENT:
            movep(- deflindent);
            continue;

        case CCSETFILE:
            switchfile();
            continue;

        case CCCHWINDOW:
            win_goto(-1);
            continue;

        case CCOPEN:
            if (file[curfile].writable == 0)
                goto nowriterr;
            openlines(curwksp->toprow + cursorline, definsert);
            continue;

        case CCMISRCH:
            search(-1);
            continue;

        case CCCLOSE:
            if (file[curfile].writable == 0)
                goto nowriterr;
            closelines(curwksp->toprow + cursorline, defdelete);
            continue;

        case CCPUT:
            if (file[curfile].writable == 0)
                goto nowriterr;
            if (pickbuf->nrows == 0)
                goto nopickerr;
            put(pickbuf, curwksp->toprow + cursorline,
                curwksp->topcol + cursorcol);
            continue;

        case CCPICK:
            picklines(curwksp->toprow + cursorline, defpick);
            continue;

        case CCINSMODE:
            insert_mode = 1 - insert_mode;  /* change it */
            continue;

        case CCGOTO:
            gtfcn(0);
            continue;

        case CCMIPAGE:
            movew(- defmipage * (1 + curwin->text_height));
            continue;

        case CCPLSRCH:
            search(1);
            continue;

        case CCRINDENT:
            movep(defrindent);
            continue;

        case CCPLLINE:
            movew(defplline);
            continue;

        case CCDELCH:
            goto notimperr;

        case CCSAVEFILE:
            savefile(NULL,curfile);
            continue;

        case CCMILINE:
            movew(-defmiline);
            continue;

        case CCDOCMD:
            goto notstrerr;

        case CCPLPAGE:
            movew(defplpage * (1 + curwin->text_height));
            continue;

        case CCMAKEWIN:
            win_open(deffile);
            continue;

        case CCTABS:
            settab(curwksp->topcol + cursorcol);
            continue;

        case CCREDRAW:                  /* Redraw screen */
        case CCEND:                     /* TODO */
            redisplay();
            keysym = -1;
            continue;

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
        getkeysym();
        if (CTRLCHAR(keysym))
            goto yesarg;
        goto noargerr;

        /*
         * Дай аргумент!
         */
gotarg:
        param(0);
yesarg:
        switch (keysym) {
        case CCQUIT:
            if (param_len > 0 &&
                (dechars(param_str, param_len), *param_str) == 'a')
            {
                if (param_str[1] != 'd')
                    return;
                cleanup();
                inputfile = -1; /* to force a dump */
                fatal("ABORTED");
            }
            if (endit())
                return;
            continue;

        case CCENTER:
            continue;

        case CCLINDENT:
            if (param_type <= 0)
                goto notstrerr;
            if (s2i(param_str, &i))
                goto notinterr;
            movep(-i);
            continue;

        case CCSETFILE:
            if (param_type <= 0)
                goto notstrerr;
            if (param_str == 0)
                goto noargerr;
            if (use0flg || ! inputfile)
                dechars(param_str, param_len);
            use0flg = 1;
            editfile(param_str, 0, 0, 1, 1);
            continue;

        case CCCHWINDOW:
            if (param_type <= 0)
                goto notstrerr;
            if (s2i(param_str, &i))
                goto notinterr;
            if (i <= 0)
                goto notposerr;
            win_goto(i - 1);
            continue;

        case CCOPEN:
            if (file[curfile].writable == 0)
                goto nowriterr;
            if (param_type != 0) {
                lnfun = openlines;
                spfun = openspaces;
                goto spdir;
            }
            splitline(curwksp->toprow + param_r0,
                param_c0 + curwksp->topcol);
            continue;

        case CCMISRCH:
        case CCPLSRCH:
            if (param_type <= 0)
                goto notstrerr;
            if (param_str == 0)
                goto noargerr;
            if (searchkey)
                free(searchkey);
            searchkey = param_str;
            param_len = 0;
            search(keysym == CCPLSRCH ? 1 : -1);
            continue;

        case CCCLOSE:
            if (file[curfile].writable == 0)
                goto nowriterr;
            if (param_type != 0) {
                if (param_type > 0 && param_str && param_str[0] == '>') {
                    msrbuf(deletebuf, param_str+1, 0);
                    continue;
                }
                lnfun = closelines;
                spfun = closespaces;
                goto spdir;
            }
            combineline(curwksp->toprow + param_r0,
                param_c0 + curwksp->topcol);
            continue;

        case CCPUT:
            if (param_type > 0 && param_str && param_str[0] == '$') {
                if (msrbuf(pickbuf, param_str+1, 1))
                    goto errclear;
                continue;
            }
            if (param_type != 0)
                goto notstrerr;
            if (file[curfile].writable == 0)
                goto nowriterr;
            if (deletebuf->nrows == 0)
                goto nodelerr;
            put(deletebuf, curwksp->toprow + cursorline,
                curwksp->topcol + cursorcol);
            continue;

        case CCMOVELEFT:
        case CCTAB:
        case CCMOVEDOWN:
        case CCHOME:
        case CCMOVEUP:
        case CCMOVERIGHT:
        case CCBACKTAB:
            if (s2i(param_str, &i))
                goto notinterr;
            if (i <= 0)
                goto notposerr;
            m = ((keysym <= CCBACKTAB) ? keysym : 0);
            while (--i >= 0)
                movecursor(m);
            continue;

        case CCRETURN:
            if (param_type <= 0 || ! param_str)
                goto notimperr;
            dechars(param_str, param_len);
            switch (param_str[0]) {
            case '>':
                msvtag(param_str + 1);
                break;
            case '$':
                if(mdeftag(param_str + 1)){
                    keysym = -1;
                    goto reparg;
                }
                break;
            case 'w':
                if(param_str[1] == ' ' && param_str[2] == '+')
                    file[curwksp->wfile].writable = 1;
                else
                    file[curwksp->wfile].writable = 0;
                break;
            case 'k':
                defkey();
                break;
            case 'r':           /* Redraw screen */
                redisplay();
                break;
            case 'd':
                if (param_str[1] == ' ')
                    defmac(&param_str[2]);
                break;
            case 'q':
                keysym = CCQUIT;
                if (param_str[1] == 'a') {
                    return;
                }
                goto nextkey;
            default:
                goto noargerr;
            }
            continue;

        case CCPICK:
            if (param_type == 0)
                goto notimperr;
            if (param_type > 0 && param_str && param_str[0] == '>') {
                msrbuf(pickbuf, param_str+1, 0);
                continue;
            }
            lnfun = picklines;
            spfun = pickspaces;
            goto spdir;

        case CCINSMODE:
            insert_mode = ! insert_mode;
            continue;

        case CCGOTO:
            if (param_type == 0)
                gtfcn(file[curfile].nlines);
            else if (param_type > 0) {
                if(param_str && param_str[0] == '$') {
                    mgotag(param_str + 1);
                    continue;
                }
                if (s2i(param_str, &i))
                    goto notinterr;
                gtfcn(i - 1);
            } else
                goto noargerr;
            continue;

        case CCMIPAGE:
            if (param_type <= 0)
                goto notstrerr;
            if (s2i(param_str, &i))
                goto notinterr;
            movew(- i * (1 + curwin->text_height));
            continue;

        case CCRINDENT:
            if (param_type <= 0)
                goto notstrerr;
            if (s2i(param_str, &i))
                goto notinterr;
            movep(i);
            continue;

        case CCPLLINE:
            if (param_type < 0)
                goto notstrerr;
            else if (param_type == 0)
                movew(cursorline);
            else if (param_type > 0) {
                if (s2i(param_str, &i))
                    goto notinterr;
                movew(i);
            }
            continue;

        case CCDELCH:
            goto notimperr;

        case CCSAVEFILE:
            if (param_type <= 0)
                goto notstrerr;
            if (param_str == 0)
                goto noargerr;
            dechars(param_str, param_len);
            savefile(param_str, curfile);
            continue;

        case CCMILINE:
            if (param_type < 0)
                goto notstrerr;
            else if (param_type == 0)
                movew(cursorline - curwin->text_height);
            else if (param_type > 0) {
                if (s2i(param_str, &i))
                    goto notinterr;
                movew(-i);
            }
            continue;

        case CCDOCMD:
            if (param_type <= 0)
                goto notstrerr;
            dechars(param_str, param_len);
            if (file[curfile].writable == 0)
                goto nowriterr;
            callexec();
            continue;

        case CCPLPAGE:
            if (param_type <= 0)
                goto notstrerr;
            if (s2i(param_str, &i))
                goto notinterr;
            movew(i * (1 + curwin->text_height));
            continue;

        case CCMAKEWIN:
            if (param_type == 0)
                win_remove();
            else if (param_type < 0)
                goto notstrerr;
            else {
                dechars(param_str, param_len);
                win_open(param_str);
            }
            continue;

        case CCTABS:
            clrtab(curwksp->topcol + cursorcol);
            continue;

        default:
            goto badkeyerr;
        }
spdir:
        if (param_type > 0) {
            if(param_str[0] == '$') {
                if (mdeftag(param_str + 1))
                    goto spdir;
                continue;
            }
            if (s2i(param_str, &i))
                goto notinterr;
            if (i <= 0)
                goto notposerr;
            (*lnfun)(curwksp->toprow + cursorline, i);
        } else {
            if (param_c1 == param_c0) {
                (*lnfun)(curwksp->toprow + param_r0,
                    (param_r1 - param_r0) + 1);
            } else
                (*spfun)(curwksp->toprow + param_r0,
                    curwksp->topcol + param_c0,
                    (param_c1 - param_c0),
                    (param_r1 - param_r0) + 1);
        }
        continue;
badkeyerr:
        error("Illegal key or unnown macro");
        continue;
notstrerr:
        error("Argument must be a string.");
        continue;
noargerr:
        error("Invalid argument.");
        continue;
notinterr:
        error("Argument must be numerik.");
        continue;
notposerr:
        error("Argument must be positive.");
        continue;
nopickerr:
        error("Nothing in the pick buffer.");
        continue;
nodelerr:
        error ("Nothing in the close buffer.");
        continue;
notimperr:
        error("Feature not implemented yet.");
        continue;
margerr:
        error("Margin stusk; move cursor to free.");
        continue;
nowriterr:
        error("You cannot modify this file!");
        continue;
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

    param_len = 0;
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
    ln = lin + curwksp->toprow;
    getlin (ln);
    putline();
    at = cline + col + curwksp->topcol;
    for (;;) {
        at += delta;
        while (at < cline || at >  cline + cline_len - lkey) {
            /* Прервать, если было прерывание с терминала */
            i = interrupt();
            if (i || (ln += delta) < 0 ||
                (wksp_position(curwksp, ln) && delta == 1))
            {
                drawlines(lin, lin);
                poscursor(col, lin);
                error(i ? "Interrupt." : "Search key not found.");
                highlight_position = 0;
                symac = 0;
                return;
            }
            getlin(ln);
            putline();
            at = cline;
            if (delta < 0)
                at += cline_len - lkey;
        }
        sk = searchkey;
        fk = at;
        while (*sk == *fk++ && *++sk);
        if (*sk == 0) {
            cgoto(ln, at - cline, slin, 0);
            highlight_position = 1;
            return;
        }
    }
}
