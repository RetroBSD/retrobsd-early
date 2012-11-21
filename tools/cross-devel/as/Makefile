#
# as      - Assembler
# aout    - Display information from a.out files
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall -Os
LDFLAGS         += -g

AOUTOBJS        = aout.o mips-dis.o

all:            mips-as mips-aout

mips-as:        as.o
		${CC} ${LDFLAGS} -o $@ as.o ${LIBS}

mips-aout:      $(AOUTOBJS)
		${CC} ${LDFLAGS} -o $@ $(AOUTOBJS) ${LIBS}

clean:
		rm -f *.o *.0 mips-as mips-aout tags *~
