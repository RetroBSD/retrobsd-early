#
# 1997/6/25 - Steven Schultz
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= id.c
OBJS		= id.o
MAN		= id.0 groups.0 whoami.0

all:            ../id ${MAN}
		install -c groups.sh ../groups
		install -c whoami.sh ../whoami

../id:          ${OBJS}
		${CC} ${LDFLAGS} -o id.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S id.elf > id.dis
		${SIZE} id.elf
		${ELF2AOUT} id.elf $@

.SUFFIXES:      .0 .1

.1.0:
		${MANROFF} $*.1 > $@

clean:
		rm -f *.o *.elf ${MAN} ../id ../groups ../whoami *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        id
		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1
		install -s -o root -g bin -m 755 id ${DESTDIR}/usr/bin/id
		install -c -o bin -g bin -m 755 groups.sh ${DESTDIR}/usr/bin/groups
		install -c -o bin -g bin -m 755 whoami.sh ${DESTDIR}/usr/bin/whoami

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
