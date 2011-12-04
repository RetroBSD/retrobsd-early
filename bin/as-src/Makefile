#
# as      - Assembler
# aout    - Display information from a.out files
#
TOPSRC		= $(shell cd ../..; pwd)
#include $(TOPSRC)/target.mk
include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall
LDFLAGS         += -g
AS              = /usr/local/pic32-tools/bin/pic32-as

OBJS		= aout.o mips-dis.o

all:            aout as

aout:           ${OBJS}
		${CC} ${LDFLAGS} -o aout ${OBJS} ${LIBS}

clean:
		rm -f *.o *~ as aout

#as:             as.o fputsym.o fputhdr.o fgeth.o fputh.o
#		$(CC) $(LDFLAGS) as.o -o as fputsym.o fputhdr.o fgeth.o fputh.o

#dis:            dis.o fgethdr.o fgeth.o
#		$(CC) $(LDFLAGS) dis.o -o dis fgethdr.o fgeth.o

test:
		$(AS) -al example.s
