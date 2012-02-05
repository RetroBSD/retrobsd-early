#
# Public Domain.  12/3/1994 - Steven Schultz
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= chflags.c stat_flags.c
OBJS		= chflags.o stat_flags.o
MAN		= chflags.0
MANSRC		= chflags.1

all:            ../chflags chflags.0

../chflags:     ${OBJS}
		${CC} ${LDFLAGS} -o chflags.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S chflags.elf > chflags.dis
		${SIZE} chflags.elf
		${ELF2AOUT} chflags.elf $@

chflags.0:      ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} ../chflags *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        chflags
		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1
		install -s -o root -g bin -m 755 chflags ${DESTDIR}/bin/chflags

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

stat_flags.o:   ../ls-src/stat_flags.c
		${CC} ${CFLAGS} -c $< -o $@

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
