#
# Copyright (c) 1988 The Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= passwd.c
OBJS		= passwd.o
MAN		= passwd.0
MANSRC		= passwd.1

all:            ../passwd passwd.0

../passwd:     ${OBJS}
		${CC} ${LDFLAGS} -o passwd.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S passwd.elf > passwd.dis
		${SIZE} passwd.elf
		${ELF2AOUT} passwd.elf $@

passwd.0:      ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} ../passwd *.elf *.dis tags *~

cleandir:       clean
		rm -f tags .depend

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        ${MAN} passwd
		install -s -o root -g bin -m 4755 passwd ${DESTDIR}/bin/passwd
		install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1

lint:           ${SRCS}
		lint ${CFLAGS} ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}
