TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS            = find.c bigram.c code.c

all:            ../find ../../libexec/bigram ../../libexec/code

../find:        find.o
		${CC} ${LDFLAGS} -o find.elf $< ${LIBS}
		${OBJDUMP} -S find.elf > find.dis
		${SIZE} find.elf
		${ELF2AOUT} find.elf $@

../../libexec/bigram: bigram.o
		${CC} ${LDFLAGS} -o bigram.elf $< ${LIBS}
		${OBJDUMP} -S bigram.elf > bigram.dis
		${SIZE} bigram.elf
		${ELF2AOUT} bigram.elf $@

../../libexec/code: code.o
		${CC} ${LDFLAGS} -o code.elf $< ${LIBS}
		${OBJDUMP} -S code.elf > code.dis
		${SIZE} code.elf
		${ELF2AOUT} code.elf $@

clean:
		rm -f *.o *.elf ../find ../../libexec/bigram ../../libexec/code *.elf *.dis tags *~

install:        ${PROG} ${LIBS}
		install -s -m 755 ${PROG} ${DESTDIR}/usr/bin/${PROG}
		-mkdir ${DESTDIR}/var/db ${DESTDIR}/usr/libexec/find
		chmod 755 ${DESTDIR}/usr/libexec/find ${DESTDIR}/var/db
		install -s -m 755 bigram ${DESTDIR}/usr/libexec/find/bigram
		install -s -m 755 code ${DESTDIR}/usr/libexec/find/code
		install -c -m 755 updatedb.csh ${DESTDIR}/usr/libexec/find/updatedb

depend:
		chmod u+w Makefile
		mkdep ${CFLAGS} ${SRCS}
