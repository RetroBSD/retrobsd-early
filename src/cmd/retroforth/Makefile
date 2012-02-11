TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -Wall

SRCS		= retro-11.2/vm/complete/retro.c
OBJS		= retro.o
BIN             = retroforth
LIBS            += -ltermcap

all:            $(BIN)

$(BIN):         ${SRCS}
		${CC} ${LDFLAGS} -o $(BIN).elf $(SRCS) ${LIBS}
		${SIZE} $(BIN).elf
		${OBJDUMP} -S $(BIN).elf > $(BIN).dis
		${ELF2AOUT} $(BIN).elf $@ && rm $(BIN).elf

clean:
		rm -f *.o *.elf $(BIN) *.elf *.dis *~ retro.c

install:        all
		install $(BIN) $(DESTDIR)/bin/
		cp retro-11.2/retroImage $(DESTDIR)/lib/
