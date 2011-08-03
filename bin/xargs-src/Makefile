#
# Public Domain.  1995/03/13 - Steven Schultz
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= xargs.c
OBJS		= xargs.o
MAN		= xargs.0
MANSRC		= xargs.1

all:            ../xargs xargs.0

../xargs:       ${OBJS}
		${CC} ${LDFLAGS} -o xargs.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S xargs.elf > xargs.dis
		${SIZE} xargs.elf
		${ELF2AOUT} xargs.elf $@

xargs.0:        ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} ../xargs *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install -s -o bin -g bin -m 755 xargs ${DESTDIR}/usr/bin
		install -c -o bin -g bin -m 444 xargs.0 ${DESTDIR}/usr/man/cat1

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}
