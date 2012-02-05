#
# Public Domain.  1995/05/02 - Steven Schultz
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -DNOFPU

SRCS		= printf.c
OBJS		= printf.o
MAN		= printf.0
MANSRC		= printf.1

all:            ../printf printf.0

../printf:      ${OBJS}
		${CC} ${LDFLAGS} -o printf.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S printf.elf > printf.dis
		${SIZE} printf.elf
		${ELF2AOUT} printf.elf $@

printf.0:       ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} ../printf *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1
		install -s -o root -g bin -m 755 printf ${DESTDIR}/usr/bin/printf

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
