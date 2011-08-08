TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

all:            hello cplus

hello:          hello.o
		${CC} ${LDFLAGS} -o hello.elf hello.o ${LIBS}
		${OBJDUMP} -S hello.elf > hello.dis
		${SIZE} hello.elf
		${ELF2AOUT} hello.elf $@

cplus:          cplus.o
		${CXX} ${LDFLAGS} -nostdlib -o cplus.elf cplus.o ${LIBS}
		${OBJDUMP} -S cplus.elf > cplus.dis
		${SIZE} cplus.elf
		${ELF2AOUT} cplus.elf $@

clean:
		rm -f *.o *.elf ${MAN} hello cplus *.elf *.dis tags *~
