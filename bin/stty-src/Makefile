#
# Public Domain.  1997/3/27 - Steven Schultz
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		= -O -Werror

SRCS		= stty.c
OBJS		= stty.o
MAN		= stty.0
MANSRC		= stty.1

all:		../stty stty.0

../stty:	${OBJS}
		${CC} ${LDFLAGS} -o stty.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S stty.elf > stty.dis
		${SIZE} stty.elf
		${ELF2AOUT} stty.elf $@

stty.0:		${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} ../stty tags *~

depend:		${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:	../stty
		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1
		install -s -o root -g bin -m 755 ../stty ${DESTDIR}/bin/stty

lint:		${SRCS}
		lint -hax ${SRCS}

tags:		${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
