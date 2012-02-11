#
# To run this program on Linux, use:
#   sudo sysctl -w kernel.randomize_va_space=0
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall -Os

OBJS            = done.o init.o io.o main.o save.o subr.o vocab.o wizard.o
MAN		= adventure.0
MANSRC		= adventure.6

all:            adventure $(MAN)

adventure:      ${OBJS}
		${CC} ${LDFLAGS} -o adventure.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S adventure.elf > adventure.dis
		${SIZE} adventure.elf
		${ELF2AOUT} adventure.elf $@ #&& rm adventure.elf

${MAN}:         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o *.0 *.elf ${MAN} adventure *.elf *.dis tags *~

install:        all
		install adventure $(DESTDIR)/games/
		cp glorkz $(DESTDIR)/games/lib/
		cp adventure.dat $(DESTDIR)/games/lib/
		cp adventure.sav $(DESTDIR)/games/lib/
		cp ${MAN} $(DESTDIR)/share/man/cat6/

$(OBJS):        hdr.h
