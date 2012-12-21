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
    i = curwksp->topline + cursorline;
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
    /* Parent. */
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
 * Insert a symbol to current line at a current position.
 */
static void cline_insert_char(keysym)
{
    int i, c;

    /* Check the window margin. */
    if (cursorcol > curwin->text_maxcol) {
        if (cline_modified)
            putline();
        wksp_offset(defroffset);
        i = curwksp->topline + cursorline;
        if (clineno != i)
            getlin(i);
    }
    cline_modified = 1;
    c = cursorcol + curwksp->offset;
    if (c >= cline_max - 2) {
        /* Expand the line. */
        excline(c + 2);
    }
    if (c >= cline_len - 1) {
        /* Append spaces to the line. */
        for (i=cline_len-1; i<=c; i++)
            cline[i] = ' ';
        cline[c+1] = '\n';
        cline_len = c + 2;
    }
    else if (insert_mode) {
        /* Push the rest of the line. */
        if (cline_len >= cline_max)
            excline(cline_len + 1);
        for (i=cline_len; i>c; i--)
            cline[i] = cline[i-1];
        cline_len++;
        drawlines(- c + curwksp->offset - 1, cursorline);
        poscursor(c - curwksp->offset, cursorline);
    }

    /* Store the symbol. */
    if (keysym == CCCTRLQUOTE)
        keysym = COCURS;
    cline[c] = keysym;
    putch(keysym, 1);
}

/*
 * Draw the bottom line: message, file name, current line number.
 */
static void drawstatus()
{
    window_t *owin;
    char numstr[16], *cp;
    int i, ccol, cline;

    /* Switch to param window. */
    owin = curwin;
    cline = cursorline;
    ccol = cursorcol;
    win_switch(&paramwin);
    paramwin.text_maxcol = NCOLS;
    if (clr_arg_area) {
        if (! message_displayed) {
            poscursor(0, 0);
            //putcha(COERLN);
            for (i=PARAMWIDTH; i>0; i--)
                putch('~', 0);
        }
        if (owin->wksp->wfile) {
            i = strlen(file[owin->wksp->wfile].name);
            if (i < PARAMWIDTH-4) {
                poscursor(PARAMWIDTH-i-2, 0);
                putch('"', 0);
                putstr(file[owin->wksp->wfile].name, NCOLS-2);
                putch('"', 0);
            }
        }
        poscursor(PARAMWIDTH, 0);
        putstr(insert_mode ? "~~Ins~~Line:~~~~~~~" :
                             "~~~~~~~Line:~~~~~~~", NCOLS);
    }
    /* Display the current line number. */
    i = owin->wksp->topline + cline + 1;
    cp = numstr + sizeof(numstr);
    *--cp = '\0';
    *--cp = '~';
    *--cp = '~';
    *--cp = '~';
    *--cp = '~';
    *--cp = '~';
    do {
        (*--cp = '0' + (i % 10));
        i /= 10;
    } while (i);
    cp[6] = '\0';
    poscursor(PARAMWIDTH+12, 0);
    putstr(cp, NCOLS-2);

    /* Switch back to editor window. */
    win_switch(owin);
    paramwin.text_maxcol = PARAMWIDTH;
    poscursor(ccol, cline);
    if (highlight_position) {
        putch(COCURS, 1);
        poscursor(ccol, cline);
        dumpcbuf();
        sleep(1);
        drawlines(cline, cline);
        poscursor(ccol, cline);
    }
}

/*
 * Main editor loop.
 */
void mainloop()
{
    /* Для команд с тремя вариантами аргументов */
    void (*lnfun)(int, int);
    void (*spfun)(int, int, int, int);
    int i, m;

    /*
     * Обработка одного символа или команды
     */
    for (;;) {
        clr_arg_area = 1;
newnumber:
        keysym = -1;        /* mark the symbol as used */
errclear:
        drawstatus();
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
         * Edit the current line.
         */
        if (! CTRLCHAR(keysym) || keysym == CCCTRLQUOTE ||
            keysym == CCBACKSPACE || keysym == CCDELCH ||
            (keysym == CCTAB && insert_mode))
        {
            /* Backspace at column 0: join lines. */
            if (keysym == CCBACKSPACE && cursorcol + curwksp->offset == 0) {
                int lnum = curwksp->topline + cursorline;
                if (lnum > file[curfile].nlines) {
                    /* Beyond end of file - move up. */
                    movecursor(CCMOVEUP);
                    keysym = -1;
                    goto nextkey;
                }
                if (file[curfile].writable == 0 || lnum == 0) {
                    /* File not writable or at line 0. */
                    keysym = -1;
                    goto nextkey;
                }
                /* Append the current line to the previous one. */
                putline();
                getlin(lnum - 1);
                combineline(lnum - 1, cline_len - 1);
                continue;
            }
            if (file[curfile].writable == 0)
                goto nowriterr;

            /* No current line? Get it! */
            i = curwksp->topline + cursorline;
            if (clineno != i)
                getlin(i);

            /* Delete char after line end: join lines. */
            if (keysym == CCDELCH &&
                cursorcol + curwksp->offset >= cline_len - 1)
            {
                int lnum = curwksp->topline + cursorline;
                if (lnum >= file[curfile].nlines) {
                    /* Beyond end of file. */
                    keysym = -1;
                    goto nextkey;
                }
                if (file[curfile].writable == 0) {
                    /* File not writable. */
                    keysym = -1;
                    goto nowriterr;
                }
                /* Append the next line to the current one. */
                putline();
                combineline(lnum, cursorcol + curwksp->offset);
                continue;
            }

            /* Delete the symbol. */
            if (keysym == CCDELCH || keysym == CCBACKSPACE) {
                int thiscol = cursorcol + curwksp->offset;
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
                thiscol -= curwksp->offset;
                drawlines(-thiscol-1, cursorline);
                poscursor(thiscol, thisrow);
                cline_modified = 1;
                keysym = -1;
                goto nextkey;
            }
            if (keysym == CCTAB) {
                /* Expand tabs to 4 spaces. */
                do {
                    cline_insert_char(' ');
                } while ((cursorcol + curwksp->offset) & 3);
            } else
                cline_insert_char(keysym);
            keysym = -1;
            goto nextkey;
        }
        if (keysym == CCRETURN) {
            /* <Enter> - split the line. */
            if (file[curfile].writable == 0)
                goto nowriterr;
            putline();
            splitline(curwksp->topline + cursorline,
                cursorcol + curwksp->offset);
            movecursor(keysym);
            keysym = -1;
            goto errclear;
        }

        /*
         * Cursor movement.
         */
        if (keysym <= CCEND) {
            movecursor(keysym);
            if (keysym == CCMOVEUP || keysym == CCMOVEDOWN ||
                keysym == CCPLPAGE || keysym == CCMIPAGE ||
                keysym == CCRETURN || keysym == CCHOME) {
                /* Row changed: save current line. */
                putline();
                goto newnumber;
            }
            keysym = -1;
            goto nextkey;
        }

        /* Window margin. */
        if (cursorcol > curwin->text_maxcol)
            poscursor(curwin->text_maxcol, cursorline);
        putline();
        switch (keysym) {
        case CCQUIT:
            if (endit())
                return;
            continue;

        case CCPARAM:
            goto gotarg;

        case CCLOFFSET:
            wksp_offset(- defloffset);
            continue;

        case CCSETFILE:
            switchfile();
            continue;

        case CCINSLIN:
            if (file[curfile].writable == 0)
                goto nowriterr;
            insertlines(curwksp->topline + cursorline, 1);
            continue;

        case CCMISRCH:
            search(-1);
            continue;

        case CCDELLIN:
            if (file[curfile].writable == 0)
                goto nowriterr;
            deletelines(curwksp->topline + cursorline, 1);
            continue;

        case CCPASTE:
            if (file[curfile].writable == 0)
                goto nowriterr;
            if (pickbuf->nrows == 0)
                goto nopickerr;
            put(pickbuf, curwksp->topline + cursorline,
                curwksp->offset + cursorcol);
            continue;

        case CCCOPY:
            picklines(curwksp->topline + cursorline, 1);
            continue;

        case CCINSMODE:
            insert_mode = 1 - insert_mode;  /* change it */
            continue;

        case CCGOTO:
            gtfcn(0);
            continue;
        case CCPLSRCH:
            search(1);
            continue;

        case CCROFFSET:
            wksp_offset(defroffset);
            continue;

        case CCDELCH:
            goto notimperr;

        case CCSAVEFILE:
            savefile(NULL, curfile);
            continue;

        case CCDOCMD:
            goto notstrerr;

        case CCREDRAW:                  /* Redraw screen */
            redisplay();
            keysym = -1;
            continue;
#if MULTIWIN
        case CCMAKEWIN:
            win_open(deffile);
            continue;
        case CCCHWINDOW:
            win_goto(-1);
            continue;

#endif
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
        param();
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

        case CCPARAM:
            continue;

        case CCLOFFSET:
            if (param_type <= 0)
                goto notstrerr;
            if (s2i(param_str, &i))
                goto notinterr;
            wksp_offset(-i);
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

        case CCINSLIN:
            if (file[curfile].writable == 0)
                goto nowriterr;
            if (param_type != 0) {
                lnfun = insertlines;
                spfun = openspaces;
                goto spdir;
            }
            splitline(param_r0, param_c0);
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

        case CCDELLIN:
            if (file[curfile].writable == 0)
                goto nowriterr;
            if (param_type != 0) {
                if (param_type > 0 && param_str && param_str[0] == '>') {
                    msrbuf(deletebuf, param_str+1, 0);
                    continue;
                }
                lnfun = deletelines;
                spfun = closespaces;
                goto spdir;
            }
            combineline(param_r0, param_c0);
            continue;

        case CCPASTE:
            if (param_type > 0 && param_str && param_str[0] == '$') {
                if (msrbuf(deletebuf, param_str+1, 1))
                    goto errclear;
                continue;
            }
            if (param_type != 0)
                goto notstrerr;
            if (file[curfile].writable == 0)
                goto nowriterr;
            if (deletebuf->nrows == 0)
                goto nodelerr;
            put(deletebuf, curwksp->topline + cursorline,
                curwksp->offset + cursorcol);
            continue;

        case CCMOVELEFT:
        case CCTAB:
        case CCMOVEDOWN:
        case CCHOME:
        case CCEND:
        case CCMOVEUP:
        case CCMOVERIGHT:
            if (s2i(param_str, &i))
                goto notinterr;
            if (i <= 0)
                goto notposerr;
            m = ((keysym <= CCEND) ? keysym : 0);
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
            case 'r':           /* Redraw screen */
                redisplay();
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

        case CCCOPY:
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

        case CCROFFSET:
            if (param_type <= 0)
                goto notstrerr;
            if (s2i(param_str, &i))
                goto notinterr;
            wksp_offset(i);
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

        case CCDOCMD:
            if (param_type <= 0)
                goto notstrerr;
            dechars(param_str, param_len);
            if (file[curfile].writable == 0)
                goto nowriterr;
            callexec();
            continue;
#if MULTIWIN
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

        case CCCHWINDOW:
            if (param_type <= 0)
                goto notstrerr;
            if (s2i(param_str, &i))
                goto notinterr;
            if (i <= 0)
                goto notposerr;
            win_goto(i - 1);
            continue;
#endif
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
            (*lnfun)(curwksp->topline + cursorline, i);
        } else {
            if (param_c1 == param_c0) {
                (*lnfun)(param_r0, (param_r1 - param_r0) + 1);
            } else
                (*spfun)(param_r0, param_c0,
                    (param_c1 - param_c0),
                    (param_r1 - param_r0) + 1);
        }
        continue;
badkeyerr:
        error("Unknown command.");
        continue;
notstrerr:
        error("Need a string parameter.");
        continue;
noargerr:
        error("Invalid argument.");
        continue;
notinterr:
        error("Need a numeric parameter.");
        continue;
notposerr:
        error("Need a positive parameter.");
        continue;
nopickerr:
        error("Clipboard is empty.");
        continue;
nodelerr:
        error ("Delete buffer is empty.");
        continue;
notimperr:
        error("Feature not implemented yet.");
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
    ln = lin + curwksp->topline;
    getlin (ln);
    putline();
    at = cline + col + curwksp->offset;
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
