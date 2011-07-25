TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

SRCS            = reboot.c
OBJS            = reboot.o
MAN             = reboot.0
MANSRC          = reboot.8

all:            ../reboot reboot.0

../reboot:      ${OBJS}
		${CC} ${LDFLAGS} -o reboot.elf ${OBJS} -lc
		${OBJDUMP} -S reboot.elf > reboot.dis
		${SIZE} reboot.elf
		${ELF2AOUT} reboot.elf $@

reboot.0:       ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *~ *.elf *.dis ${MAN} ../reboot tags

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        reboot
		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
		-rm -f ${DESTDIR}/usr/man/cat8/fastboot.0 ${DESTDIR}/usr/man/cat8/halt.0
		ln ${DESTDIR}/usr/man/cat8/reboot.0 ${DESTDIR}/usr/man/cat8/fastboot.0
		ln ${DESTDIR}/usr/man/cat8/reboot.0 ${DESTDIR}/usr/man/cat8/halt.0
		install -s -o root -g bin -m 755 reboot ${DESTDIR}/sbin/reboot
		-rm ${DESTDIR}/sbin/fastboot ${DESTDIR}/sbin/halt
		ln ${DESTDIR}/sbin/reboot ${DESTDIR}/sbin/halt
		ln ${DESTDIR}/sbin/reboot ${DESTDIR}/sbin/fastboot

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
