#	Makefile for test
#
#	1.1 	Robin Birch	11 march 1995
#	1.2 	Steven Schultz	27 october 1996
#
#	Version 1.2
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS            = test.c operators.c
OBJS            = test.o operators.o

all:            ../test

../test:        ${OBJS}
		${CC} ${LDFLAGS} -o test.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S test.elf > test.dis
		${SIZE} test.elf
		${ELF2AOUT} test.elf $@

clean:
		rm -f *.o *.elf ${MAN} ../test *.elf *.dis tags *~

install:        all
		install -s -m 755 -g bin -o bin test ${DESTDIR}/bin
		rm -f ${DESTDIR}/bin/[
		ln ${DESTDIR}/bin/test ${DESTDIR}/bin/[
		install -c -m 444 -g bin -o bin test.0 ${DESTDIR}/usr/man/cat1
