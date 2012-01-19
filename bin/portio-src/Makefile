TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -Wall

SRCS		= portio.c
OBJS		= portio.o

all:            ../portio

../portio:      ${OBJS}
		${CC} ${LDFLAGS} -o portio.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S portio.elf > portio.dis
		${SIZE} portio.elf
		${ELF2AOUT} portio.elf $@

clean:
		rm -f *.o *.elf ../portio *.elf *.dis *~
