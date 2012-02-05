#
# Public Domain.  1995/03/13 - Steven Schultz
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS            = hostname.c
OBJS            = hostname.o
MAN             = hostname.0

all:            ../hostname ${MAN}

../hostname:    ${OBJS}
		${CC} ${LDFLAGS} -o hostname.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S hostname.elf > hostname.dis
		${SIZE} hostname.elf
		${ELF2AOUT} hostname.elf $@

.SUFFIXES:      .0 .1

.1.0:
		${MANROFF} $*.1 > $@

clean:
		rm -f *.o *.elf ${MAN} ../id ../groups ../whoami *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install -s -o bin -g bin -m 755 hostname ${DESTDIR}/bin
		install -c -o bin -g bin -m 444 hostname.0 ${DESTDIR}/usr/man/cat1

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}
