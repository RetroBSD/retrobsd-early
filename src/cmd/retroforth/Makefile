TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -Wall

SRCS		= retro.c
OBJS		= retro.o
BIN             = retroforth
LIBS            += -ltermcap

all:            install-lib

$(BIN).elf:     ${OBJS}
		${CC} ${LDFLAGS} -o $(BIN).elf $(OBJS) ${LIBS}

$(BIN).dis:     $(BIN).elf
		${OBJDUMP} -S $(BIN).elf > $(BIN).dis
		${SIZE} $(BIN).elf

../$(BIN):      $(BIN).dis
		${ELF2AOUT} $(BIN).elf $@

retro.c:
		echo "#include <stdint.h>" >retro.c
		echo "#include <termios.h>" >>retro.c
		head -n 69 retro-11.2/vm/complete/retro.c >>retro.c
		echo '#undef LOCAL' >>retro.c
		echo '#define LOCAL "/lib/retroImage"' >>retro.c
		echo '#undef IMAGE_SIZE' >>retro.c
		echo '#define IMAGE_SIZE 10000' >>retro.c
		tail -n +69 retro-11.2/vm/complete/retro.c >>retro.c
                

install-lib:    ../$(BIN)
		cp retro-11.2/retroImage $(TOPSRC)/lib

clean:
		rm -f *.o *.elf ../$(BIN) *.elf *.dis *~ retro.c $(TOPSRC)/lib/retroImage
