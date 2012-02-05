#
# Public Domain.  1996/11/16 - Steven Schultz
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= renice.c
OBJS		= renice.o
MAN		= renice.0
MANSRC		= renice.8

all:            ../renice renice.0

../renice:      ${OBJS}
		${CC} ${LDFLAGS} -o renice.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S renice.elf > renice.dis
		${SIZE} renice.elf
		${ELF2AOUT} renice.elf $@

renice.0:       ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} ../renice *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        renice
		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
		install -s -o root -g bin -m 755 renice ${DESTDIR}/usr/bin/renice

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
