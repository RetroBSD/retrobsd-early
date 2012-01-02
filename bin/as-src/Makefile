#
# as      - Assembler
# aout    - Display information from a.out files
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall -Os
LDFLAGS         += -g

all:            ../as ../aout

../as:          as.o
		${CC} ${LDFLAGS} -o as.elf as.o ${LIBS} #-lvmf
		${OBJDUMP} -S as.elf > as.dis
		${SIZE} as.elf
		${ELF2AOUT} as.elf $@

../aout:        aout.o mips-dis.o
		${CC} ${LDFLAGS} -o aout.elf aout.o mips-dis.o ${LIBS} #-lvmf
		${OBJDUMP} -S aout.elf > aout.dis
		${SIZE} aout.elf
		${ELF2AOUT} aout.elf $@

clean:
		rm -f *.o *.0 *.elf ../as ../aout *.elf *.dis tags *~

test:
		/usr/local/pic32-tools/bin/pic32-as -al example.s
