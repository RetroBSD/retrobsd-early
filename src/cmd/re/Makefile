#
# Visual text editor for Unix.
# Based on sources of RAND Editor.
# Part of Demos Operating System Project, Russia 1982-1991.
#
# Alex P. Roudnev, Moscow, KIAE, 1984
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -DDEBUG -Wall -Werror

OBJS            = r.cmd.o r.edit.o r.file.o r.gettc.o r.hlam.o \
                  r.mac.o r.main.o r.tele.o r.termc.o r.ttyio.o r.wind.o

all:            re

re:          	${OBJS}
		${CC} ${LDFLAGS} -o re.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S re.elf > re.dis
		${SIZE} re.elf
		${ELF2AOUT} re.elf $@

${OBJ}:         r.defs.h

clean:
		rm -f *.o *.elf re *.elf *.dis *~

install:        all help.txt
		install re $(DESTDIR)/bin/re
		cp -p help.txt $(DESTDIR)/share/re.help
