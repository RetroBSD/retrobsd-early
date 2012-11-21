TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall -g
LDFLAGS		+= -g

SRCS		= strip.c
OBJS            = strip.o

all:            mips-strip

mips-strip:     ${OBJS}
		${CC} ${LDFLAGS} -o $@ ${OBJS} ${LIBS}

clean:
		rm -f *.o mips-strip a.out tags *~
