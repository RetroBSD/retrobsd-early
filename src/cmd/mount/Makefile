#
# Public Domain.  1996/1/15 - Steven Schultz
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS            = mount.c getmntopts.c mount_ufs.c
OBJS            = mount.o getmntopts.o mount_ufs.o
MAN		= mount.0
MANSRC		= mount.8

all:            ../mount mount.0

../mount:       ${OBJS}
		${CC} ${LDFLAGS} -o mount.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S mount.elf > mount.dis
		${SIZE} mount.elf
		${ELF2AOUT} mount.elf $@

mount.0:        ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} ../mount *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
		install -s -o root -g bin -m 755 mount ${DESTDIR}/sbin/mount

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
