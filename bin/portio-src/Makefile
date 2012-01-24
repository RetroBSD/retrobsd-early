TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -Wall

SRCS		= portio.c
OBJS		= portio.o

all:            ../portio ../loldemo

../portio:      ${OBJS}
		${CC} ${LDFLAGS} -o portio.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S portio.elf > portio.dis
		${SIZE} portio.elf
		${ELF2AOUT} portio.elf $@

../loldemo:     loldemo.o
		${CC} ${LDFLAGS} -o loldemo.elf loldemo.o ${LIBS}
		${OBJDUMP} -S loldemo.elf > loldemo.dis
		${SIZE} loldemo.elf
		${ELF2AOUT} loldemo.elf $@

clean:
		rm -f *.o *.elf ../portio ../loldemo *.elf *.dis *~
