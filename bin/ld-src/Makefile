#
# Version 2.2 January 24, 1998
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall -g
LDFLAGS		+= -g

SRCS		= ld.c
OBJS            = ld.o
MAN		= ld.0
MANSRC		= ld.1

all:            ../ld $(MAN)

../ld:          ${OBJS}
		${CC} ${LDFLAGS} -o ld.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S ld.elf > ld.dis
		${SIZE} ld.elf
		${ELF2AOUT} ld.elf $@

ld.0:           ld.1
		${MANROFF} $< > $@

clean:
		rm -f *.o *.0 *.elf ${MAN} ../ld *.elf *.dis a.out tags *~

#install:        all
#		install -c -s -o bin -g bin -m 755 ld ${DESTDIR}/bin/ld
#		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1
