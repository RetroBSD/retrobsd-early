/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "curses.ext"

/*
 * implement the mvscanw commands.  Due to the variable number of
 * arguments, they cannot be macros.  Another sigh....
 */
int
mvscanw(y, x, fmt, args)
        reg int		y, x;
        char		*fmt;
        int		args;
{
	return move(y, x) == OK ? _sscans(stdscr, fmt, &args) : ERR;
}

int
mvwscanw(win, y, x, fmt, args)
        reg WINDOW	*win;
        reg int		y, x;
        char		*fmt;
        int		args;
{
	return wmove(win, y, x) == OK ? _sscans(win, fmt, &args) : ERR;
}
