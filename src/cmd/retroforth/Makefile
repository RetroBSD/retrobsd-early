TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -Wall

SRCS		= retro.c
OBJS		= retro.o
BIN             = retroforth
LIBS            += -ltermcap

all:            $(BIN)

$(BIN):         ${OBJS}
		${CC} ${LDFLAGS} -o $(BIN).elf $(OBJS) ${LIBS}
		${SIZE} $(BIN).elf
		${OBJDUMP} -S $(BIN).elf > $(BIN).dis
		${ELF2AOUT} $(BIN).elf $@ && rm $(BIN).elf

retro.c:
		echo "#include <stdint.h>" >retro.c
		echo "#include <termios.h>" >>retro.c
		head -n 69 retro-11.2/vm/complete/retro.c >>retro.c
		echo '#undef LOCAL' >>retro.c
		echo '#define LOCAL "/lib/retroImage"' >>retro.c
		echo '#undef IMAGE_SIZE' >>retro.c
		echo '#define IMAGE_SIZE 10000' >>retro.c
		tail -n +69 retro-11.2/vm/complete/retro.c >>retro.c


clean:
		rm -f *.o *.elf $(BIN) *.elf *.dis *~ retro.c

install:        all
		install $(BIN) $(DESTDIR)/bin/
		cp retro-11.2/retroImage $(DESTDIR)/lib/
