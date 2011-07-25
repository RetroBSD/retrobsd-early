TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          = -O
SRCS            = chown.c
OBJS            = chown.o
MAN             = chown.0
MANSRC          = chown.8

all:            ../chown

../chown:       ${OBJS}
		${CC} ${LDFLAGS} -o chown.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S chown.elf > chown.dis
		${SIZE} chown.elf
		${ELF2AOUT} chown.elf $@

chown.0:        ${MANSRC}
		/usr/man/manroff ${MANSRC} > ${MAN}

clean:
		rm -f *.o ../chown chown.elf chown.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        chown
		install -s -o root -g bin -m 755 ../chown ${DESTDIR}/usr/sbin/chown
#		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
