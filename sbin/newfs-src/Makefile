#
# Public Domain.  1996/11/16 - Steven Schultz
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

SRCS		= newfs.c
OBJS		= newfs.o
MAN		= newfs.0
MANSRC		= newfs.8

all:		../newfs newfs.0

../newfs:	${OBJS}
		${CC} ${LDFLAGS} -o newfs.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S newfs.elf > newfs.dis
		${SIZE} newfs.elf
		${ELF2AOUT} newfs.elf $@

newfs.0:	${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf *.dis *~ ${MAN} ../newfs tags

depend:		${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:	../newfs
		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
		install -s -o root -g bin -m 755 ../newfs ${DESTDIR}/sbin/newfs

lint:		${SRCS}
		lint -hax ${SRCS}

tags:		${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
