DESTDIR=
SEPFLAG= -i
CFILES=	done.c init.c io.c main.c save.c subr.c vocab.c wizard.c
OBJS=	done.o init.o io.o main.o save.o subr.o vocab.o wizard.o
HFILES = hdr.h
SRCS = $(CFILES) $(HFILES) glorkz
CFLAGS=	-O
LDFLAGS=-n	# should be -i on small machines, -n on a vax

advent:	a.out
	rm -f advent
	echo y | ./a.out

a.out: ${OBJS}
	${CC} ${SEPFLAG} ${LDFLAGS} ${CFLAGS} ${OBJS}

install: advent
	install -s advent ${DESTDIR}/games/adventure
#	chmod 711 /games/lib
#	chmod 700 /games/lib/adventure
#	chmod 4711 /games/adventure
#	chown games /games/adventure /games/lib/adventure

clean:
	rm -f ${OBJS} a.out advent core errs
