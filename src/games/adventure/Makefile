TOPSRC		= $(shell cd ../../..; pwd)
#include $(TOPSRC)/target.mk
include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall

OBJS            = done.o init.o io.o main.o save.o subr.o vocab.o wizard.o
MAN		= adventure.0
MANSRC		= adventure.6

all:            advent $(MAN)

advent:         ${OBJS}
		${CC} ${LDFLAGS} -o advent.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S advent.elf > advent.dis
		${SIZE} advent.elf
		${ELF2AOUT} advent.elf $@ && rm advent.elf

${MAN}:         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o *.0 *.elf ${MAN} advent *.elf *.dis tags *~

install:        all
		install advent $(DESTDIR)/games/adventure
		cp ${MAN} $(DESTDIR)/share/man/cat6/
