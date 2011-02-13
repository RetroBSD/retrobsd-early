#
# Public Domain.  1996/11/16 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1996/11/16
#
TOPSRC		= $(shell cd ../..; pwd)
#include $(TOPSRC)/target.mk
include $(TOPSRC)/cross.mk

CFLAGS		+= -Wall

SRCS		= mkfs.c
OBJS		= mkfs.o
MAN		= mkfs.0
MANSRC		= mkfs.8

all:		mkfs mkfs.0

mkfs:		${OBJS}
		${CC} ${LDFLAGS} -o $@ ${OBJS} ${LIBS}
		${SIZE} $@

mkfs.0:		${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *~ ${MAN} mkfs tags

depend:		${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:	mkfs
		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
		install -s -o root -g bin -m 755 mkfs ${DESTDIR}/sbin/mkfs

lint:		${SRCS}
		lint -hax ${SRCS}

tags:		${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
