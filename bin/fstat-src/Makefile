#
# Public Domain.  1996/11/16 - Steven Schultz
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= fstat.c
OBJS		= fstat.o
MAN		= fstat.0
MANSRC		= fstat.1

all:            ../fstat fstat.0

../fstat:       ${OBJS}
		${CC} ${LDFLAGS} -o fstat.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S fstat.elf > fstat.dis
		${SIZE} fstat.elf
		${ELF2AOUT} fstat.elf $@

fstat.0:        ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} ../fstat *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        fstat
		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
		install -s -o bin -g kmem -m 2755 fstat ${DESTDIR}/usr/bin/fstat

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
