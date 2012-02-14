#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1.2	(2.11BSD)	1999/10/25
#

DESTDIR=
CFLAGS=	-O -DBSD -DDEST=\"${DESTDIR}/usr/games/lib/atc/\"
SEPFLAG= -i
YFLAGS=	-d
LIBS=	-ll -lm -lcurses -ltermcap
SRCS=	extern.c grammar.c graphics.c input.c lex.c list.c log.c \
	main.c tunable.c update.c
OBJS=	extern.o grammar.o input.o lex.o list.o log.o main.o tunable.o \
	graphics.o update.o

all: atc

atc: ${OBJS}
	${CC} ${SEPFLAG} ${OBJS} ${LIBS} -o $@

clean: FRC
	rm -f ${OBJS} core atc grammar.c y.tab.h y.tab.c lex.yy.c lex.c

depend: lex.c grammar.c FRC
	mkdep ${CFLAGS} ${SRCS}

install: FRC
	-[ -d ${DESTDIR}/usr/games/lib/atc ] || mkdir ${DESTDIR}/usr/games/lib/atc
	chmod 700 ${DESTDIR}/usr/games/lib/atc
	chown games.bin ${DESTDIR}/usr/games/lib/atc
	(cd games; install -c -o games -g bin -m 600 * ${DESTDIR}/usr/games/lib/atc)
	install -s -o games -g bin -m 4700 atc ${DESTDIR}/usr/games/hide/atc
	(cd ${DESTDIR}/usr/games; rm -f atc; ln -s dm atc; chown games.bin atc)

lint: lex.c grammar.c FRC
	lint ${CFLAGS} ${SRCS}

tags: lex.c grammar.c FRC
	ctags ${SRCS}

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
