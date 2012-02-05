#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= more.c
OBJS		= more.o

all:            ../more

../more:        ${OBJS}
		${CC} ${LDFLAGS} -o more.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S more.elf > more.dis
		${SIZE} more.elf
		${ELF2AOUT} more.elf $@

clean:
		rm -f *.o *.elf ${MAN} ../more *.elf *.dis tags *~

install:        more more.help
		install -m 755 -s more ${DESTDIR}/usr/ucb/more
		rm -f ${DESTDIR}/usr/ucb/page
		ln ${DESTDIR}/usr/ucb/more ${DESTDIR}/usr/ucb/page
		install -m 444 -c more.help ${DESTDIR}/usr/share/misc/more.help
