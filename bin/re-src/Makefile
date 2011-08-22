TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -DDEBUG #-Wall #-Werror

OBJS            = r.cmd.o r.edit.o r.file.o r.gettc.o r.hlam.o \
                  r.mac.o r.main.o r.tele.o r.termc.o r.ttyio.o r.wind.o

all:            ../re ../../share/re.help

../re:          ${OBJS}
		${CC} ${LDFLAGS} -o re.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S re.elf > re.dis
		${SIZE} re.elf
		${ELF2AOUT} re.elf $@

../../share/re.help: help.txt
		cp -p $< $@

clean:
		rm -f *.o *.elf ../re *.elf *.dis *~
