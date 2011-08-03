#
# Public Domain.  1/15/1995 - Steven Schultz
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= sysctl.c
OBJS		= sysctl.o

all:            ../sysctl

../sysctl:      ${OBJS}
		${CC} ${LDFLAGS} -o sysctl.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S sysctl.elf > sysctl.dis
		${SIZE} sysctl.elf
		${ELF2AOUT} sysctl.elf $@

clean:
		rm -f *.o *.elf ${MAN} ../sysctl *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
		install -s -o root -g bin -m 755 sysctl ${DESTDIR}/bin/sysctl

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
