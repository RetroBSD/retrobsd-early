#
# Public Domain.  1996/11/16 - Steven Schultz
#
TOPSRC          = $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		= -O
SRCS		= update.c
OBJS		= update.o
MAN		= update.0
MANSRC		= update.8

all:		update update.0

update:		${OBJS}
		${CC} ${LDFLAGS} -o $@ ${OBJS} ${LIBS}
		${SIZE} $@

update.0:	${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *~ ${MAN} update tags

depend:		${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:	update
		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
		install -s -o root -g bin -m 755 update ${DESTDIR}/usr/sbin/update

lint:		${SRCS}
		lint -hax ${SRCS}

tags:		${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
