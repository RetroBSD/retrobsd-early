#
# Public Domain.  12/3/1994 - Steven Schultz
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		= -O
SRCS		= ls.c stat_flags.c
OBJS		= ls.o stat_flags.o

all:		../ls

../ls:		${OBJS}
		${CC} ${LDFLAGS} -o $@ ${OBJS} ${LIBS}
		${SIZE} $@

clean:
		rm -f *.o ../ls tags *~

depend:		${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:	../ls
		install -s -o root -g bin -m 755 ../ls ${DESTDIR}/bin/ls

lint:		${SRCS}
		lint -hax ${SRCS}

tags:		${SRCS}
		ctags ${SRCS}
