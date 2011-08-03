#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

OBJS		= main.o init.o subr.o gettytab.o get_date.o

all:            ../../libexec/getty

../../libexec/getty: ${OBJS}
		${CC} ${LDFLAGS} -o getty.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S getty.elf > getty.dis
		${SIZE} getty.elf
		${ELF2AOUT} getty.elf $@

clean:
		rm -f *.o *.elf ${MAN} ../getty *.elf *.dis tags *~

install:        all
		install -s -m 555 -o bin -g bin getty ${DESTDIR}/usr/libexec/getty

main.o: gettytab.h
init.o: gettytab.h
subr.o: gettytab.h
